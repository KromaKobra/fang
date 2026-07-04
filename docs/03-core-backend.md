# 03 — Core Backend (C++)

This document specifies the C++ classes so the implementation makes no independent design
decisions. Signatures are indicative (Qt/C++20); exact parameter types may be refined but
behavior and responsibilities are fixed. All types live in `namespace fang`.

---

## 1. Application (QML singleton)

`Application` is the app-global entry point, registered as a QML singleton (`QML_ELEMENT`,
`QML_SINGLETON`, URI `Fang`).

Responsibilities: process bootstrap, config/theme access, window management, global
services, path opening, session save/restore.

Key members:
- `int apiVersion() const` → `1`. Exposed as `Q_PROPERTY`.
- `QString appVersion() const`.
- `Settings* settings()`, `Theme* theme()`, `Places* places()`, `Search* search()`,
  `ActionRegistry* actions()` — `Q_PROPERTY` pointers (owned by Application).
- `Q_INVOKABLE void openPath(const QString& path)` — opens in a new window.
- `Q_INVOKABLE WindowController* newWindow()`.
- `Q_INVOKABLE void quit()`.
- `Q_INVOKABLE void reloadTheme()` / `reloadActions()` / `reloadConfig()` — hot reload.
- Signals: `themeChanged()`, `configReloaded()`, `error(QString title, QString detail)`.

Startup sequence (in `main.cpp`):
1. Construct `QGuiApplication` (org name `KromaKobra`, app name `Fang`, desktop file
   `dev.kromakobra.Fang`).
2. Set Wayland-friendly attributes; honor `QT_QPA_PLATFORM`.
3. Parse CLI: positional `[path...]`, `--new-window` (no-op; always new process),
   `--select <file>`, `--version`, `--help`.
4. Load config → theme → actions → places.
5. Create `QQmlApplicationEngine`; add the UI import path resolution (see §UI loading).
6. Register C++ QML types; expose `Application` singleton.
7. Load root UI component; restore session if enabled; open CLI paths.

### UI loading & override resolution

The engine loads the **root component** from the first location that exists:
1. `~/.config/fang/ui/main.qml` (user override project root).
2. `qrc:/Fang/ui/Main.qml` (bundled default).

If a user override exists, its directory is prepended to the QML import path so the
override can import its own components and (optionally) reuse bundled ones via the `Fang`
module. If the override fails to load (QML error), Fang logs the error, shows a small
native fallback notice, and loads the bundled UI so the app is never bricked by a bad
override. The override contract is defined in [04-qml-api.md](04-qml-api.md).

---

## 2. WindowController (QML type)

One per window. Created by `Application.newWindow()` or by the root UI.

- `Q_PROPERTY QQmlListProperty<TabController> tabs`.
- `Q_PROPERTY int activeTabIndex`.
- `Q_PROPERTY bool splitView` — whether the active tab shows two panes.
- `Q_INVOKABLE TabController* addTab(const QString& path = home)`.
- `Q_INVOKABLE void closeTab(int index)`; `moveTab(from,to)`; `duplicateTab(int)`.
- `Q_INVOKABLE void toggleSplit()`; `focusOtherPane()`.
- Session serialization to/from a TOML fragment.

### TabController
- Holds one or two `PaneController`s (`leftPane`, `rightPane`, `activePane`).
- `title` derived from active pane location (basename or place name).

---

## 3. PaneController (QML type)

The heart of a browsing view. One per pane.

Properties (`Q_PROPERTY`, notifiable):
- `QString location` (current absolute path/URL).
- `DirectoryModel* model` (read-only).
- `Navigation* navigation`.
- `Selection* selection`.
- `Clipboard* clipboard` (may be app-shared; exposed here for convenience).
- `FileOperations* fileOperations`.
- `ViewMode viewMode` — enum `Icons | List | Details | Tree` (`Q_ENUM`).
- `SortRole sortRole` + `bool sortDescending` + `bool foldersFirst`.
- `QStringList columns` — ordered visible columns for Details/Tree (see §DirectoryModel
  roles for the column keys).
- `bool showHidden`.
- `QString filterText` — live type-to-filter (name substring, case-insensitive).
- `int iconSize` (px) for Icons view; `int zoomLevel` general.
- `bool loading`; `int itemCount`; `qint64 totalSize` (of listing).

Invokables:
- `void open(const QString& pathOrUrl)` — navigate.
- `void activate(int row)` — open item at row (folder → navigate; file → launch default
  app via MimeResolver).
- `void openInNewTab(int row)` / `openInNewWindow(int row)`.
- `void refresh()`.
- `void setColumns(const QStringList&)`; `moveColumn(from,to)`; `setColumnWidth(key,px)`.
- `void setSort(SortRole, bool desc)`.
- `void createFolder(const QString& name)`; `createFile(const QString& name)`.
- `void renameAt(int row, const QString& newName)`.

Signals: `locationChanged`, `loadingChanged`, `activated(int row)`, `error(...)`.

---

## 4. DirectoryModel (QAbstractItemModel)

Backs every view. Flat for Icons/List/Details; hierarchical (lazy-expanded) for Tree.

- Populated asynchronously by a scan worker; emits rows in batches.
- Filtering: hidden files (unless `showHidden`), plus live `filterText`.
- Sorting: stable, by `SortRole`, folders-first optional, natural (numeric-aware) name
  sort, case-insensitive by default.
- Backed by `FileSystemWatcher` for live add/remove/modify.

### Roles (fixed enum — these are the canonical column keys too)

| Role enum | QML role name | Column key | Meaning |
|-----------|---------------|-----------|---------|
| `NameRole` | `name` | `name` | Display file name |
| `PathRole` | `path` | — | Absolute path |
| `UrlRole` | `url` | — | `file://` URL |
| `IsDirRole` | `isDir` | — | bool |
| `IsSymlinkRole` | `isSymlink` | — | bool |
| `SizeRole` | `size` | `size` | bytes (dirs: -1 unless computed) |
| `SizeTextRole` | `sizeText` | — | human-readable |
| `ModifiedRole` | `modified` | `modified` | QDateTime |
| `CreatedRole` | `created` | `created` | QDateTime (if available) |
| `AccessedRole` | `accessed` | `accessed` | QDateTime |
| `MimeRole` | `mimeType` | `type` | mime name |
| `MimeCommentRole` | `typeText` | — | human type description |
| `PermissionsRole` | `permissions` | `permissions` | rwx string |
| `OwnerRole` | `owner` | `owner` | user name |
| `GroupRole` | `group` | `group` | group name |
| `IconNameRole` | `iconName` | — | themed icon name |
| `ThumbnailRole` | `thumbnail` | — | `image://fang-thumb/<path>` URL or "" |
| `IsHiddenRole` | `isHidden` | — | bool |
| `SelectedRole` | `selected` | — | bool (bridged from Selection) |
| `ExtensionRole` | `extension` | `extension` | file suffix |

Default Details columns: `name, size, type, modified`. All keys above with a "Column key"
are user-selectable/reorderable (see [06-config-theming.md](06-config-theming.md)).

Tree view: `hasChildren` true for dirs; `fetchMore`/`canFetchMore` lazily scan children.

---

## 5. FileSystemWatcher

Wraps `QFileSystemWatcher`. Coalesces bursts (debounce ~100 ms), diffs the directory, and
emits `added(paths)`, `removed(paths)`, `changed(paths)`. Re-arms watches on directory
replacement. Watches the current directory (and expanded tree nodes in Tree view). Guards
against the inotify watch-count limit by only watching visible/expanded scopes.

---

## 6. FileOperationEngine + workers

Central queue for copy/move/delete/trash. Each enqueued job is an `Operation` QObject
exposed to QML.

`Operation` properties: `id`, `type` (`Copy|Move|Delete|Trash|Restore`), `state`
(`Queued|Running|Paused|WaitingConflict|Completed|Failed|Cancelled`), `progress` (0..1),
`bytesDone`, `bytesTotal`, `filesDone`, `filesTotal`, `currentSource`, `currentDest`,
`speedBytesPerSec`, `etaSeconds`, `errorText`.

`Operation` invokables: `pause()`, `resume()`, `cancel()`,
`resolveConflict(ConflictMode mode, bool applyToAll)` where `ConflictMode` is
`Overwrite | Skip | Rename | Merge | KeepBoth` (`Merge` only for directories).

Engine invokables (via `FileOperations` controller):
- `Operation* copy(const QList<QUrl>& sources, const QUrl& destDir)`.
- `Operation* move(...)`, `Operation* remove(const QList<QUrl>&)` (permanent delete),
  `Operation* trash(const QList<QUrl>&)`, `Operation* restoreFromTrash(...)`.
- `Operation* createFolder(destDir, name)`, `rename(url, newName)`.

Worker behavior:
- Copy: recursive, preserves timestamps/permissions where possible, buffered streaming
  with progress by bytes, handles same-device rename fast-path for Move.
- Conflict detection before overwrite; pause worker and emit conflict; wait for
  resolution; `applyToAll` caches the choice for the rest of the batch.
- Cross-filesystem Move = copy+verify+delete-source (atomic per file).
- Symlinks copied as links by default (configurable).
- On per-file error: record, and either abort or continue per an operation policy
  (default: collect errors, continue, surface a summary at end).
- Every successful mutating op appends an inverse record to `UndoStack`.

Idempotency & safety: never follow a delete into a mount boundary unexpectedly; refuse to
delete `/` or a mount root without explicit confirmation; verify destination is writable
before starting.

---

## 7. TrashManager (freedesktop Trash spec v1.0)

- Trash location resolution: `$XDG_DATA_HOME/Trash` for home volume; `.Trash/$uid` or
  `.Trash-$uid` at the top of the file's mount for other volumes, created as needed.
- On trash: move file into `files/`, write `info/<name>.trashinfo` with original `Path`
  (URL-encoded) and `DeletionDate` (ISO-8601 local time).
- List trashed items (parse `.trashinfo`), with original path + deletion date.
- Restore: move back to original path (conflict-checked); remove info file.
- Empty trash (all, or selected). Handle name collisions in `files/` by suffixing.
- Robust to missing/corrupt `.trashinfo` (surface as "unknown original location").

---

## 8. UndoStack

Bounded (default 50) multi-level undo/redo.

- Records: `Move(records of from→to)`, `Rename`, `Trash(items→trash paths)`,
  `Copy(created paths)` (undo = delete the copies), `CreateFolder/CreateFile` (undo =
  delete), `RestoreFromTrash`.
- `undo()` / `redo()` run the inverse as a new engine operation (with progress) and are
  themselves reversible.
- Undo of a copy deletes only the files the copy created (tracked precisely), never
  pre-existing files.
- Exposed to QML: `canUndo`, `canRedo`, `undoText`, `redoText`, `undo()`, `redo()`.

---

## 9. SearchEngine + SearchResultsModel

- `Search` controller invokables: `start(SearchQuery)`, `cancel()`, `clear()`.
- `SearchQuery`: `rootPath`, `namePattern` (glob/substring), `bool contentSearch`,
  `contentQuery`, `bool caseSensitive`, `bool regex`, `bool includeHidden`,
  filters: `minSize/maxSize`, `modifiedAfter/Before`, `mimeTypes`.
- Threaded recursive walk (respects a max-depth guard and skips pseudo-filesystems like
  `/proc`, `/sys`, `/dev` unless root is inside them). Streams matches into
  `SearchResultsModel` (same roles as DirectoryModel + a `matchContext` role for content
  hits). Cancellable; reports progress (`dirsScanned`, `matches`).
- Content search reads files directly (skips binary by null-byte heuristic + size cap).
- Live type-to-filter (`PaneController.filterText`) is separate and purely in-model (no
  worker) — instant filtering of the already-listed directory.

---

## 10. ThumbnailProvider (QQuickImageProvider)

- Registered as image provider id `fang-thumb`; QML uses `image://fang-thumb/<abs-path>`.
- Implements the freedesktop **Thumbnail Managing Standard**: looks up
  `~/.cache/thumbnails/{normal,large}/<md5(file-uri)>.png`; validates via stored
  `Thumb::MTime`/`Thumb::URI`; regenerates if stale.
- Generation runs on a bounded thread pool. v1 generators: raster images and SVG via Qt
  (`QImageReader`, respects EXIF orientation). Architecture allows registering more
  generators by MIME (video/pdf/etc.) later without touching callers.
- Writes thumbnails back to the shared cache (interop with other apps) with correct PNG
  metadata keys. Respects `normal` (128px) and `large` (256px) sizes.
- Never blocks the GUI thread; returns a placeholder synchronously and updates when ready.
- Skips thumbnailing for files above a configurable size cap and for non-image mimes in
  v1 (returns "" so the view falls back to the themed icon).

---

## 11. MountMonitor + Places

- Enumerate mounts via `QStorageInfo::mountedVolumes()` + parse `/proc/self/mountinfo`;
  watch mountinfo for changes.
- Classify: internal drives, removable devices, optical, network (gvfs under
  `/run/user/$UID/gvfs`).
- Optional udisks2 (D-Bus) integration for mount/unmount/eject of removable devices;
  actions hidden if udisks2 is unavailable.
- **Places** model sections (in order): *Places* (Home + XDG user dirs: Desktop,
  Documents, Downloads, Music, Pictures, Videos, Public, Templates — only those that
  exist), *Bookmarks* (user-editable, persisted to `bookmarks.toml`, drag-reorderable),
  *Devices* (from MountMonitor, with free/total space + mount state), *Network* (mounted
  gvfs), *Trash* (with item-count badge). Each entry: `label`, `iconName`, `url`,
  `section`, `isMounted`, `isEjectable`, `usedBytes`, `totalBytes`.
- Invokables: `addBookmark(url,label)`, `removeBookmark(id)`, `moveBookmark(from,to)`,
  `mount(deviceId)`, `unmount(deviceId)`, `eject(deviceId)`.

---

## 12. MimeResolver & app launching

- `QMimeDatabase` for detection (content + name). Human descriptions from mime comments.
- Default app: query via `gio mime`/`xdg-mime query default`; fallback to parsing
  associations + `.desktop` files across `$XDG_DATA_DIRS/applications`.
- `Q_INVOKABLE openWith(urls, desktopFileId)` and `openDefault(urls)` launch detached via
  the resolved `Exec` line (field-code expansion) or `gio open`.
- `Q_INVOKABLE QList<DesktopApp> appsFor(mimeType)` powers the "Open With…" picker
  (`DesktopApp`: `id`, `name`, `iconName`, `exec`).

---

## 13. ConfigStore

- Loads/saves TOML for: `config.toml`, `actions.toml`, `bookmarks.toml`,
  `session.toml`, and the theme. Provides typed getters with documented defaults; emits
  granular change signals consumed by `Settings`/`Theme`.
- Writes atomically (temp file + rename). Never loses unknown keys on rewrite (round-trip
  preservation). Versioned with a `schema_version` for future migration.
- File locations per XDG in [06-config-theming.md](06-config-theming.md).

---

## 14. ActionRegistry & ActionParser

- Parse `actions.toml` into `Action` objects: `id`, `label`, `iconName`, `command`
  (template), `mimeTypes`/`extensions`/`appliesTo` conditions, `minSelection`,
  `maxSelection`, `showInMenu`, `runInTerminal`, `submenu`.
- `Q_INVOKABLE QList<Action> actionsFor(const Selection*)` — filters by selection
  count + MIME/extension match.
- `Q_INVOKABLE void run(actionId, const Selection*)` — expands placeholders and launches
  detached (optionally in a terminal via the configured terminal command).
- Placeholders: `%f` (single file path), `%F` (all selected paths, quoted),
  `%u`/`%U` (URLs), `%d` (parent dir), `%n` (basename), `%%` (literal). Proper shell
  quoting is mandatory — never string-concatenate unescaped paths.
- Ships built-in default actions (see [06-config-theming.md](06-config-theming.md)) that
  the user can override or disable.

---

## 15. Settings & Theme controllers

- `Settings`: typed `Q_PROPERTY`s mirroring `config.toml` (see schema) with change
  signals and setters that persist. Groups: `general`, `view`, `behavior`, `previews`,
  `search`, `terminal`, `appearance`.
- `Theme`: exposes palette + metrics to QML as properties (colors, radii, spacing, font
  sizes). Loads the active theme (bundled `dark` default) or a user override theme.
  If `appearance.follow_system = true`, subscribes to the freedesktop appearance portal
  and swaps between the configured `light`/`dark` theme variants; otherwise stays fixed.
  Full property list in [04-qml-api.md](04-qml-api.md#theme) and default values in
  [06-config-theming.md](06-config-theming.md).
