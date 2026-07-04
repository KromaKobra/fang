# PROGRESS — implementation handoff state

**Purpose:** session-resume notes for finishing Fang. Read this together with
`docs/PROMPT.md` (staged plan) and the numbered spec docs. Everything below is
already decided/verified — do not re-derive.

## Build environment (VERIFIED WORKING)

No toolchain on PATH; NixOS host. Use the flake dev shell with the system's
registry nixpkgs (Qt 6.11, nixos-26.05) so nothing downloads:

```sh
nix develop --override-input nixpkgs flake:nixpkgs --no-write-lock-file \
  -c bash -c 'cmake -B build -G Ninja && cmake --build build'
# run:  ./build/src/fang ~            (Wayland; verified working on this host)
# or:   QT_QPA_PLATFORM=offscreen timeout 5 ./build/src/fang ~   (EXIT=124 == ran fine)
# tests: ctest --test-dir build --output-on-failure   (inside the same dev shell)
```

Files must be `git add`ed for nix flake commands to see them (flake in git repo).
Qt log output on this host goes to journald — use `QT_FORCE_STDERR_LOGGING=1`
to see qDebug/qWarning on the terminal. Enable Fang's categories with
`QT_LOGGING_RULES="fang.*=true"`.

The flake's shellHook exports `QML_IMPORT_PATH`/`QML2_IMPORT_PATH` to
qtdeclarative's qml dir — REQUIRED: unwrapped dev-shell binaries only resolve
QML modules whose backing C++ library is linked (embedded qmldirs); the
QtQuick.Controls.* plugins are not linked, so without the env var the UI
fails with `module "QtQuick.Controls.Basic" is not installed`.

## Status by stage

- **Stage 1 — DONE.** CMake skeleton, flake.nix, LICENSE (canonical GPL-3 text,
  md5 1ebbd3e34237af26da5dc08a4e440464), .clang-format/.editorconfig/.gitignore,
  main.cpp bootstrap (CLI parse, override resolution + fallback, icon provider,
  QQuickStyle "Basic"), Application singleton, Theme (full token set, TOML
  loading, compiled-in dark defaults), bundled themes dark/light + default
  actions.toml (under src/ui/themes/, aliased to qrc:/Fang/themes/).
- **Stage 2 — DONE.** FileInfo, HumanFormat, FileSystemWatcher (snapshot diff,
  100ms debounce, per-file watches when <=512 entries), DirectoryModel (Node
  tree, async QtConcurrent scan streaming batches, final sorted reset for root,
  single sorted insert for lazily fetched tree children, natural sort QCollator,
  filter/hidden, watcher deltas, all 20 roles per docs/03 §4), Selection
  (path-based set internally, row API per contract), Navigation (history,
  breadcrumbs, commitEdit, childDirs), PaneController, TabController,
  WindowController, MimeResolver (FULL: detection, mimeapps.list/mimeinfo.cache,
  .desktop parsing+Exec expansion, gio/xdg-open launching, setDefaultApp),
  IconResolver + IconImageProvider ("fang-icon", painted fallback glyphs).
- **Stage 3 — DONE** (commit "Stage 3 complete", 2026-07-04). Everything in
  docs/05 except stage-4+ features: MountMonitor, Places (+ additive
  bookmarkIndex/renameBookmark invokables), all controls/* primitives, all four
  views + FileView interaction hub, ContextMenu (stage-3 subset), LocationBar
  (breadcrumbs + edit + completion), Toolbar (anchors layout: nav buttons left,
  trailing controls right, LocationBar anchored between), TabBar/TabButton
  (drag-reorder via Translate transform + moveTab on release; Row positioner
  owns x so `drag.target` does NOT work), Sidebar (collapsible sections,
  bookmark drag-reorder, rename/remove popup, unmount/eject menu), PlaceRow
  (capacity bar, trash badge, eject), StatusBar (counts + selection size +
  zoom slider 16..256 bound to pane.iconSize + free space), NotificationOverlay
  (Application.error toasts + overrideError banner on startup),
  Shortcuts.qml (stage-3 subset of the docs/05 table), MainWindow (per-tab
  Repeater keyed on win.tabCount, visible:index===activeTabIndex preserves
  per-tab view state; split via ratio property + Splitter; right pane in a
  Loader active on tabCtl.splitView; active pane accent border; publishes
  root.currentFileView for F2), CreateDialog (folder/file name prompt wired to
  FileView.requestCreateFolder/File and Ctrl+Shift+N), final Main.qml.
  `Application.formatSize(bytes)` invokable added (additive) for QML byte text.
  Builds clean with -Werror; runs on offscreen AND wayland with zero QML
  warnings; single Application instance verified.

**App is in a user-testable state at this commit:** browse, tabs, split view,
sidebar/places/bookmarks, all four view modes, sorting/columns, type-to-filter,
hidden toggle, rename (F2), create folder/file, open files via default app,
breadcrumb/location editing, zoom slider, toasts. NOT yet present (later
stages): copy/move/trash/undo/paste/DnD (stage 4), thumbnails/preview pane
(stage 5), search (stage 6), persisted settings/session (stage 7), custom
actions/open-with/properties dialogs (stage 8), packaging files (stage 9),
tests (stage 10).

## Stage 4 — DESIGN DECIDED, ready to implement

`src/ops/Operation.h` is ALREADY WRITTEN (not yet in CMakeLists, so it does
not affect the build): Operation QObject with the docs/04 property set, plus
`OpControl` (shared control block: atomics for pause/cancel, QMutex +
QWaitCondition conflict handshake with applyToAll cache) and `OpWorker` base
(keepGoing() pause/cancel gate, blocking askConflict(), error collection).
Write Operation.cpp to match, then the rest in this order:

1. **TrashManager** (ops/TrashManager.{h,cpp}) — static methods, no QObject.
   Read `$XDG_DATA_HOME` AT CALL TIME (tests set it inside QTemporaryDir so
   the same-device home-trash path is taken). trashDirFor(path): compare
   st_dev of path vs data home (deepest existing ancestor) → home Trash, else
   $topdir/.Trash/$uid (validate sticky, non-symlink) or .Trash-$uid.
   trashFile(): reserve unique name by O_EXCL create of info/<name>.trashinfo
   (collisions: name.2, name.3 …), write `[Trash Info]` with URL-encoded Path
   (QUrl::toPercentEncoding(path, "/")) + ISO DeletionDate, then rename into
   files/. list(): parse info/*.trashinfo, corrupt → item flagged (never
   crash); files/ entries without info → "unknown origin". restore(item,
   overwrite): mkpath parent, rename back, remove info. emptyAll/removeItem.
2. **CopyWorker** (+ MoveWorker as thin moveMode subclass, own file per
   docs/10): enumerate pass (filesTotal/bytesTotal, no symlink follow), then
   per top-level entry: conflict check at dest (askConflict; Overwrite:
   replace — for dirs remove-then-copy; Skip; KeepBoth/Rename: uniqueName
   "name (2)" before extension; Merge (dirs): recurse, per-file conflicts as
   encountered). Streamed 1 MiB chunk copy with keepGoing() between chunks;
   cancel mid-file removes the partial dest. Preserve permissions +
   file times (QFile::setPermissions / setFileTime). Track EVERY created
   path in order (undo of merge must delete only created files). Per-file
   errors: collect + continue; all-failed → Failed, else Completed with
   errorText summary. Move: same-device (st_dev compare) rename fast-path
   per entry after conflict check; EXDEV/cross-device → copy+verify(size)+
   delete-source, deleting a source entry only when its whole subtree copied
   clean. Refuse dest inside source.
3. **DeleteWorker** — modes {Remove, Trash, Restore} (keeps docs/10 file
   list; trash/restore execution belongs to the removal family). Remove:
   refuse "/" and mount roots (QStorageInfo(path).rootPath()==path);
   bottom-up recursive delete with per-file progress. Trash: TrashManager::
   trashFile per entry, undoData records (orig, filesPath, infoPath).
   Restore: conflict flow when origin exists (Overwrite/Skip/KeepBoth).
4. **UndoStack** (ops/UndoStack.{h,cpp}) — QML_ELEMENT UNCREATABLE, bounded
   50. Record {OpType, text, QVariantMap data}. Data by type: copy {sources,
   destDir, createdPaths}; move {pairs from→to} (undo groups pairs by source
   parent, one engine op per group); rename {dir, oldName, newName}; trash
   {items}; create {path}. undo(): pop, run inverse via engine with
   recording SUPPRESSED, push onto redo stack when the inverse op finishes
   OK (drop + notify on failure); m_busy guard ignores undo/redo while an
   inverse op is in flight. undoText/redoText: "Undo Move (3 items)".
5. **Clipboard** (ops/Clipboard.{h,cpp}) — wraps QGuiApplication::clipboard.
   copy/cut write `x-special/gnome-copied-files` ("copy\n<url>\n…"/"cut\n…")
   + text/uri-list + text/plain; parse on QClipboard::dataChanged →
   hasFiles/mode/cutPaths (additive QStringList prop that FileView.cutPaths
   binds to for cut-dimming). paste(destDir): Cut → engine move + clear;
   Copy → engine copy; returns Operation*. copyText(str) additive invokable
   for "Copy Path".
6. **FileOperationEngine + FileOperations** (both classes in
   FileOperationEngine.{h,cpp}; docs/10 lists no separate FileOperations
   file). Engine: app singleton (C++ instance(), NOT a QML singleton), owns
   QList<Operation*> (prune terminal ops beyond ~20, deleteLater after
   operationsChanged), owns UndoStack + Clipboard. Per async op: new QThread,
   worker moveToThread, queued connects worker→Operation slots
   (progressed/conflictAsked/stateHint/done); done → record undo (unless
   suppressed), thread quit + deleteLater. Engine connects op conflict →
   conflictRequested(op, src, dest, info) and failures → Application::notify.
   rename/createFolder/createFile are synchronous (Operation finishSync).
   FileOperations: QML controller per pane (thin forwarder to the engine)
   with the docs/04 method set + additive autoTransfer(urls, destDir) for
   DnD "auto" (same st_dev → move, else copy) + operations list property
   + forwarded conflictRequested/operationsChanged.
7. **Wiring:** Application gains clipboard/undo Q_PROPERTYs (engine-owned).
   PaneController gains fileOperations property; route createFolder/
   createFile/renameAt through the engine so they undo. FileView.cutPaths:
   bind Application.clipboard.cutPaths. ContextMenu stage-4 items: Cut/Copy/
   Copy Path/Paste(-into)/Move to Trash/Delete + background Paste. Shortcuts:
   Ctrl+C/X/V, Del (trash), Shift+Del (confirm popup, permanent), Ctrl+Z,
   Ctrl+Shift+Z. Toolbar menu: Undo/Redo entries with undoText/redoText.
8. **QML:** ConflictDialog.qml (modal; both files' name/size/mtime; buttons
   Overwrite/Skip/Keep Both/Rename/Merge-if-dir; apply-to-all Checkbox →
   op.resolveConflict; opened from MainWindow Connections on
   pane.fileOperations.conflictRequested). OperationOverlay.qml (bottom-right
   stacked cards over pane.fileOperations.operations: type, progress bar,
   speed/ETA via Application.formatSize, pause/resume/cancel; show
   non-terminal + briefly-completed). RenameEditor.qml: extract the inline
   editor from FileDelegate (keep FileDelegate's behavior identical, it just
   instantiates RenameEditor). DnD: FileDelegate drag via proxy Item with
   Drag.dragType: Drag.Automatic + Drag.mimeData {"text/uri-list": …} using
   the full selection; DropArea on FileView background (→ pane.location),
   on dir delegates (→ that dir), on Sidebar PlaceRow (→ place url);
   modifiers: Ctrl → copy, Shift → move, none → autoTransfer.

Then stages 5–10 per the plan below.

## Key architecture decisions (verified compiling/working)

- `fangcore` STATIC lib is the QML module backing target:
  `qt_add_qml_module(fangcore URI Fang VERSION 1.0 RESOURCE_PREFIX /)`.
  Executable links `fangcore fangcoreplugin`; main.cpp has
  `Q_IMPORT_QML_PLUGIN(FangPlugin)`. Bundled UI at `qrc:/Fang/ui/Main.qml`;
  engine gets `engine.addImportPath("qrc:/")`. Tests must link fangcoreplugin
  too for the smoke test.
- `qt_policy(SET QTP0004 NEW)` needed (QML files in subdirs).
- Generated type registration requires bare-name includes → every src subdir is
  on fangcore's PUBLIC include path.
- Forward-declared pointer Q_PROPERTY types need `Q_MOC_INCLUDE("dir/File.h")`
  next to the fwd decl (pattern used in Application.h/PaneController.h etc.).
- Enums: `util/Enums.h` = namespace fang + Q_NAMESPACE + QML_NAMED_ELEMENT(Fang)
  + enum class + Q_ENUM_NS → QML `Fang.ViewMode.Icons` etc. (all 7 enums per
  docs/04). `Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")` in the
  namespace block — required, several enums share value names (OpType.Rename/
  ConflictMode.Rename, OpType.Trash/PlaceSection.Trash) and unscoped
  registration warns at startup.
- **QML singletons with a C++-owned instance MUST NOT be default-
  constructible.** QQmlPrivate::singletonConstructionMode prefers the
  constructor over static create() when both exist → the engine silently
  built a SECOND Application (and would for MimeResolver). Constructors now
  take a mandatory `QObject* parent` (main passes nullptr). If you add a
  singleton, repeat this pattern.
- Application/MimeResolver are QML singletons with static create() returning a
  C++-owned instance (Application constructed in main before engine load).
- Selection tracks paths internally (survives sort/refresh); `selectPath`/
  `isPathSelected` are additive API used by tree + SelectedRole bridging
  (model->notifySelectionChanged emits dataChanged for SelectedRole).
- QML conventions: inside src/ui/, `import QtQuick as QQ` and use QQ.ListView/
  QQ.GridView/QQ.TreeView because local ListView.qml/TreeView.qml shadow the
  QtQuick types. In delegates use `required property var model` + `model.role`
  (role names clash with FileDelegate's own properties; Places' `id` role is
  unusable as a bare name — always `model.id`). Menus: controls/Menu
  auto-assigns `menu` property to children (auto-close) and sizes to widest
  item; MenuItem/Separator children.
- Icons in QML: `image://fang-icon/<name>` (IconImageProvider). Thumbnails
  (stage 5): role already returns `image://fang-thumb<abs-path>` for images —
  note the id arrives WITHOUT leading slash to the provider; provider must
  re-add it.
- FileView is the interaction hub: clickRow/doubleClickRow/middleClickRow/
  rightClickRow/backgroundClick/backgroundRightClick, keyboard nav via
  selection.current + currentView.ensureVisible/columnsPerRow, type-to-filter
  appends to pane.filterText, renameRow + finishRename/cancelRename, cutPaths
  (empty until stage 4 Clipboard), singleClickOpens=true until Settings wired.
- MainWindow publishes `currentFileView` (active tab's active pane's FileView)
  by having each tab page push it on visibility/active-pane change — Repeater
  itemAt() is not reactive, don't try to bind through it.

## Remaining stages 5–10 — plan + decisions already made

- **Stage 5 previews:** preview/{ThumbnailProvider,ThumbnailGenerator}.
  QQuickAsyncImageProvider ("fang-thumb") + freedesktop cache
  $XDG_CACHE_HOME/thumbnails/{normal,large}/<md5(file-uri)>.png with Thumb::URI/
  Thumb::MTime keys (QImage text()); EXIF via QImageReader
  setAutoTransform(true); size cap from config; PreviewPane.qml (image/text
  peek via new Previews controller invokable readTextPeek(path, maxKb) —
  Previews controller listed in docs/01; expose on Application or pane).
  F11 toggles.
- **Stage 6 search:** search/{SearchEngine,SearchResultsModel,SearchQuery}.
  SearchQuery Q_GADGET value or QObject with props; worker thread walk
  (QDirIterator manual recursion, depth cap, ignore_paths, hidden toggle),
  name glob/substring/regex + content grep (null-byte binary skip + size cap),
  streams via queued batches; Search controller on Application.search;
  SearchPanel.qml + FileView shows results model (pane switches model? simplest:
  SearchPanel lists results in its own view using same delegates or a plain
  list; activate -> open containing dir/select).
- **Stage 7 config:** config/{ConfigStore,Settings}. toml++ round-trip
  (keep parsed toml::table, mutate + rewrite via QSaveFile), schema_version=1;
  Settings typed props for every key in docs/06 §config.toml with per-group
  change signals + persistence; Theme picks appearance.* (icon_theme →
  IconResolver::setIconTheme, font override, theme name, follow_system →
  config/AppearancePortal (org.freedesktop.portal.Settings Read color-scheme
  + SettingChanged listener; only active when follow_system)). Shortcuts.qml:
  central Shortcut map reading Settings shortcut overrides ([shortcuts] table).
  session.toml save on exit (Application::saveSession serializing windows/tabs/
  panes: geometry, locations, viewMode, sort, columns, split, active) +
  restore in newWindow when general.restore_session and no CLI paths.
  Settings.open_with single/double → FileView.singleClickOpens;
  view defaults (mode/sort/columns/hidden/icon_size) → new panes;
  behavior.new_tab_position → WindowController.addTab.
- **Stage 8 actions:** config/{ActionRegistry,ActionParser} + util/ShellQuote.
  ShellQuote::quote(str) POSIX single-quote escaping ('\'' pattern);
  expand %f %F %u %U %d %n %% per docs/06 (each path individually quoted;
  %F/%U expand to multiple quoted words). Parse bundled qrc actions.toml then
  user file; same id overrides; show_in_menu=false disables. actionsFor
  (Selection*) filters applies_to files/background/both + mime glob (text/*)
  + extensions + min/max selection. run(): expand + `/bin/sh -c <cmd>`
  startDetached (run_in_terminal → wrap in terminal_command; "auto" detects
  $TERMINAL, kitty, foot, alacritty, wezterm, konsole, gnome-terminal, xterm
  first found; terminal exec arg separator: kitty/foot/alacritty use direct
  argv, konsole/gnome-terminal need -e). ActionRegistry on Application.actions;
  ContextMenu inserts actionsFor items + "Open With…" submenu via
  MimeResolver.appsFor + PropertiesDialog/OpenWithDialog QML; properties dialog
  needs async recursive dir size → add invokable on a controller (Previews or
  PaneController) using QtConcurrent.
- **Stage 9 override/packaging:** override loading ALREADY IMPLEMENTED in
  main.cpp (fallback + Application.overrideError banner path; NotificationOverlay
  already shows it on startup). Write data/dev.kromakobra.Fang.desktop,
  metainfo.xml, icons/dev.kromakobra.Fang.svg (draw a simple fang/chevron),
  README.md. Install rules exist in root CMakeLists (already). Installing the
  .desktop file also fixes the harmless Wayland warning "Could not register
  app ID: App info not found for 'dev.kromakobra.Fang'".
- **Stage 10 tests:** tests/CMakeLists.txt loop exists; set FANG_TESTS list to
  the 10 suites (docs/09): tst_fileops tst_trash tst_undo tst_search tst_config
  tst_actions tst_mime tst_model tst_thumbnails tst_qml_smoke. All FS work in
  QTemporaryDir; set XDG_DATA_HOME/XDG_CACHE_HOME/XDG_CONFIG_HOME per-test via
  qputenv BEFORE touching the classes; offscreen platform set via test env in
  CMake. tst_model: use QSignalSpy on loadingChanged + QTRY_COMPARE row counts;
  watcher change test: touch file (rewrite content w/ different size) then
  QTRY. tst_qml_smoke: QQmlApplicationEngine + addImportPath("qrc:/") + load
  bundled Main.qml; requires QQuickWindow — offscreen ok; also test broken
  override fallback by pointing XDG_CONFIG_HOME at a dir with a bad
  ~/.config/fang/ui/main.qml equivalent and re-running the resolution logic
  (factor override resolution in main.cpp into a small helper in fangcore, e.g.
  fang::resolveRootUrl(engine) — refactor needed for testability).
- **Finally:** update README, `git commit`. Manual QA per docs/09.

## Gotchas discovered (do not rediscover)

- Qt 6.11 REMOVED the `QtQuick.Window` compat import — `Window` comes from
  plain `import QtQuick`. Also `closePolicy: 0` no longer parses ("unknown
  enumeration"): use `QQC.Popup.NoAutoClose` (LocationBar completions popup).
- Do not declare `property bool enabled` on custom QML components — Item
  already has it and Qt 6.11 warns about the override on every instantiation.
- FileView Loader creates views with `required property` — passed via
  sourceComponent items with bindings `fileView: root; pane: root.pane` (OK).
- Row/Column positioners own child x/y: MouseArea `drag.target` on a
  positioner child does nothing — drag visuals need a Translate transform and
  the reorder applied on release (TabButton, Sidebar bookmarks do this).
- DirectoryModel.setPath emits pathChanged AFTER reset; Selection clears via
  that signal.
- WindowController.addTab has first-tab activeTabChanged workaround (guard in
  setActiveTabIndex made it a no-op) — keep when editing.
- QQC2 style pinned via QQuickStyle::setStyle("Basic") before engine; import
  `QtQuick.Controls.Basic as QQC` in controls/Popup|Tooltip|Menu (and
  LocationBar for the closePolicy enum).
- fang.* logging categories default QtWarningMsg; enable with
  QT_LOGGING_RULES="fang.*=true" (+ QT_FORCE_STDERR_LOGGING=1 on this host).
- flake.lock is gitignored intentionally (registry override workflow).
