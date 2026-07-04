# 01 — Architecture

## Layered overview

```
┌───────────────────────────────────────────────────────────────────┐
│  VIEW LAYER (QML)                                                   │
│  Bundled default UI (qrc:/ui/...) OR user override (~/.config/fang) │
│  Custom Qt Quick components. No filesystem logic. Calls QML API.    │
└───────────────▲───────────────────────────────────────────────────┘
                │  QML API (versioned): singletons, models, roles,
                │  Q_INVOKABLE methods, signals, enums
┌───────────────┴───────────────────────────────────────────────────┐
│  CONTROLLER LAYER (C++, QObject, exposed to QML)                    │
│  Application, WindowController, PaneController, Navigation,         │
│  Selection, Clipboard, FileOperations, Search, Places,             │
│  ActionRegistry, Theme, Settings, Previews                          │
└───────────────▲───────────────────────────────────────────────────┘
                │  in-process C++ calls
┌───────────────┴───────────────────────────────────────────────────┐
│  SERVICE / MODEL LAYER (C++)                                        │
│  DirectoryModel (QAbstractItemModel), FileSystemWatcher,           │
│  FileOperationEngine (+ workers), TrashManager, UndoStack,         │
│  SearchEngine, ThumbnailProvider, MountMonitor, MimeResolver,      │
│  ConfigStore, ActionParser, IconResolver                           │
└───────────────▲───────────────────────────────────────────────────┘
                │  OS / libraries
┌───────────────┴───────────────────────────────────────────────────┐
│  PLATFORM: POSIX FS, inotify (QFileSystemWatcher), Qt Concurrent,   │
│  QMimeDatabase, XDG dirs, freedesktop Trash + Thumbnails specs,     │
│  udisks2 (D-Bus, optional), gio/gvfs mounts, xdg-mime/gio launch    │
└───────────────────────────────────────────────────────────────────┘
```

**Strict rule:** QML never touches the filesystem directly. All I/O flows through the
Controller/Service layers. This keeps the override contract stable and safe.

## Process model

Fang is **multi-process**. Each `fang [path]` invocation is an independent OS process
with its own event loop, models, watchers, and windows. There is **no** daemon, no D-Bus
activation, and no cross-process coordination. Consequences the implementation must
honor:

- Each process owns its own `FileSystemWatcher`s and thumbnail workers.
- Session state (open tabs/panes) is per-process and optionally restored on next launch
  from `~/.local/state/fang/session.toml` if session restore is enabled.
- A window may contain multiple tabs and split panes; those live in one process.
- No locking assumptions between Fang instances beyond normal filesystem semantics.

## Threading model

The GUI thread (Qt Quick scene graph + QML) must never block. Rules:

- **Directory listing:** `DirectoryModel` populates asynchronously. The initial scan of a
  directory runs on a worker (QtConcurrent / `QThreadPool`); results are delivered to the
  model in batches on the GUI thread via queued signals. Stat calls (size, mtime,
  permissions, mime) are performed off-thread and streamed in.
- **File operations:** each operation (copy/move/delete/trash) runs on a dedicated worker
  managed by `FileOperationEngine`. Progress, conflicts, and completion are reported to
  the GUI thread via signals. Conflicts pause the worker and wait for a resolution set
  from the UI.
- **Search:** runs on a worker; results stream to a `SearchResultsModel` incrementally
  and can be cancelled.
- **Thumbnails:** generated on a bounded thread pool; the GUI requests a thumbnail by
  path+mtime and receives it asynchronously (placeholder icon until ready).
- **Watching:** `QFileSystemWatcher` (inotify) signals are debounced/coalesced (e.g.
  100 ms) before the model refreshes, to avoid thrash on bulk changes.

Thread-safety: models and controllers are GUI-thread objects. Workers communicate only
via signals/slots with `Qt::QueuedConnection`. No shared mutable state is accessed from
two threads without a mutex; prefer message passing.

## Module responsibilities

### Controllers (QML-facing QObjects)

- **Application** — app-global singleton: version, config access, theme, quit,
  new-window, open-path, recent locations, global services (search, places, actions).
- **WindowController** — one per window: manages tabs and split panes, active pane,
  window-level actions (new tab, close tab, toggle split, focus pane).
- **PaneController** — one per pane: current location, view mode, sort, column set,
  the `DirectoryModel` instance, selection, per-pane navigation history.
- **Navigation** — back/forward/up/home/refresh, path parsing, breadcrumb segments,
  location bar editing/validation.
- **Selection** — selection set for a pane (anchor, current, multi, range, invert),
  exposes counts/aggregate size to the UI.
- **Clipboard** — cut/copy/paste using the system clipboard with freedesktop
  `x-special/gnome-copied-files` + `text/uri-list` semantics for interop.
- **FileOperations** — QML entry points that enqueue operations on the engine and expose
  live operation objects (progress, state, conflict prompts).
- **Search** — starts/stops searches, exposes `SearchResultsModel` and query params.
- **Places** — the sidebar model: XDG dirs, bookmarks (editable), devices (from
  `MountMonitor`), trash, and a network section (mounted gvfs paths only in v1).
- **ActionRegistry** — parses `actions.toml`, evaluates which actions apply to the
  current selection (by MIME/extension/count), and executes them.
- **Theme** — exposes the active palette/metrics to QML; loads bundled or overridden
  theme; handles opt-in system light/dark following.
- **Settings** — typed access to `config.toml`; change signals; persistence.
- **Previews** — bridges QML to `ThumbnailProvider` and to text/code peek.

### Services (pure C++)

- **DirectoryModel** — `QAbstractItemModel` (flat for icon/list, hierarchical for tree)
  with a fixed set of roles (see [04-qml-api.md](04-qml-api.md)). Handles sorting,
  hidden-file filtering, live filter, and incremental population.
- **FileSystemWatcher** — wraps `QFileSystemWatcher`, coalesces events, emits
  add/remove/change deltas to models.
- **FileOperationEngine** + **CopyWorker/MoveWorker/DeleteWorker** — the transactional
  op engine. Emits progress; supports pause/resume/cancel; produces undo records.
- **TrashManager** — freedesktop Trash spec: trash to the correct `.Trash` per mount,
  write `.trashinfo`, list, restore, empty.
- **UndoStack** — bounded multi-level undo/redo of move/rename/trash/copy(→delete) ops.
- **SearchEngine** — threaded recursive walk with name globbing + optional content grep.
- **ThumbnailProvider** — implements `QQuickImageProvider`; reads/writes
  `~/.cache/thumbnails/{normal,large}` per the freedesktop spec; pluggable generators.
- **MountMonitor** — enumerates mounts (QStorageInfo + `/proc/self/mountinfo`), watches
  for changes, exposes removable devices; optional mount/unmount via udisks2 D-Bus;
  surfaces gvfs mounts under `/run/user/$UID/gvfs`.
- **MimeResolver** — `QMimeDatabase` wrapper; content+name detection; default-app lookup
  and launch via `xdg-mime`/`gio` and `.desktop` parsing.
- **ConfigStore** — loads/saves TOML config, actions, bookmarks, session, with defaults
  and migration by version.
- **IconResolver** — `QIcon::fromTheme` against the configured XDG icon theme with
  fallbacks; provides icon names to QML.

## Data flow examples

**Navigating into a folder:** QML calls `pane.navigation.open(path)` → `PaneController`
sets location → `DirectoryModel` starts async scan → watcher armed → rows stream to the
view → thumbnails requested lazily as delegates become visible.

**Copying files:** QML calls `pane.fileOperations.copy(urls, destUrl)` →
`FileOperationEngine` creates an `Operation` (exposed to QML) and dispatches a
`CopyWorker` → progress signals update the QML progress UI → on name clash the worker
emits `conflict(...)`, UI shows the resolution dialog, calls
`operation.resolveConflict(mode, applyToAll)` → on completion an undo record is pushed.

**Applying a custom action:** QML shows `actionRegistry.actionsFor(selection)` in the
context menu → user picks one → `actionRegistry.run(actionId, selection)` expands
placeholders and launches the command detached.
