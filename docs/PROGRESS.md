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
# run:  QT_QPA_PLATFORM=offscreen timeout 5 ./build/src/fang ~   (EXIT=124 == ran fine)
# tests: ctest --test-dir build --output-on-failure   (inside the same dev shell)
```

Files must be `git add`ed for nix flake commands to see them (flake in git repo).

## Status by stage (see task list in PROMPT.md / my task tracker)

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
  Builds and runs; listed a directory via interim UI.
- **Stage 3 — IN PROGRESS.** Done: MountMonitor (mountinfo POLLPRI watch,
  udisks2 via QDBus optional), Places model (XDG dirs + bookmarks.toml via
  toml++ + devices + gvfs network + trash w/ count badge, watcher on trash dir),
  Application.places property + notify(); PaneController.activatePath/freeBytes;
  Navigation.childDirs. QML controls ALL WRITTEN: Separator, Badge, Spinner,
  Button, IconButton, ToolButton, TextField, Popup, Tooltip, Menu, MenuItem,
  ScrollBar (v+h), Splitter, Checkbox, Slider. Views WRITTEN: FileDelegate,
  FileView, IconsView, ListView, DetailsView, ColumnHeader, TreeView,
  ContextMenu (stage-3 subset), ViewModeSwitch, LocationBar, Toolbar.
  **NOT yet written:** TabBar.qml, TabButton.qml, Sidebar.qml, PlaceRow.qml,
  StatusBar.qml, NotificationOverlay.qml, Shortcuts.qml, MainWindow.qml,
  CreateDialog.qml, and the final Main.qml (current Main.qml is the stage-2
  interim). **None of the new QML files are compiled yet** — nothing since the
  stage-2 build has been compiled; expect a round of QML/C++ fixups.

### Immediate TODOs to finish stage 3

1. Update `src/CMakeLists.txt`: add places/*.cpp (DONE), and list ALL new QML
   files in `FANG_QML_FILES` (controls/* and ui/* written above) — currently it
   only lists `ui/Main.qml`.
2. Write the missing QML files listed above. MainWindow layout:
   Column[TabBar, Toolbar, Row[Sidebar, Splitter, pane area (FileView x1|x2 +
   Splitter)], StatusBar]; per-tab Repeater with `visible: index===activeTabIndex`
   to preserve per-tab view state; active pane gets accent border; click in a
   pane -> tab.setActivePane. FileView.requestCreateFolder/File signals ->
   MainWindow opens CreateDialog (simple name prompt -> pane.createFolder/File).
3. Fix Toolbar LocationBar width: replace the hacky `width:` expression with a
   proper anchors-based layout (anchor location bar between button row and
   trailing controls, or use explicit Item with anchors).
4. Build; fix errors; run `./build/src/fang ~` under offscreen AND wayland.

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
  docs/04).
- Application/MimeResolver are QML singletons with static create() returning a
  C++-owned instance (Application constructed in main before engine load).
- Selection tracks paths internally (survives sort/refresh); `selectPath`/
  `isPathSelected` are additive API used by tree + SelectedRole bridging
  (model->notifySelectionChanged emits dataChanged for SelectedRole).
- QML conventions: inside src/ui/, `import QtQuick as QQ` and use QQ.ListView/
  QQ.GridView/QQ.TreeView because local ListView.qml/TreeView.qml shadow the
  QtQuick types. In delegates use `required property var model` + `model.role`
  (role names clash with FileDelegate's own properties). Menus: controls/Menu
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

## Remaining stages 4–10 — plan + decisions already made

- **Stage 4 ops:** ops/{FileOperationEngine,Operation,CopyWorker,MoveWorker,
  DeleteWorker,TrashManager,UndoStack,Clipboard}. Operation QObject exposed to
  QML (props per docs/04); worker QThread per operation; pause/cancel via
  atomics; conflicts: worker blocks on QWaitCondition, resolveConflict(mode,
  applyToAll) wakes it. Cross-fs move = copy+verify+delete. TrashManager:
  $XDG_DATA_HOME/Trash for same-device (read env at call time → tests set
  XDG_DATA_HOME inside QTemporaryDir so same-device path is used), .Trash-$uid
  at mount top otherwise; .trashinfo URL-encoded Path + ISO DeletionDate;
  collision suffixing. UndoStack: records with precise created-paths for copy.
  Clipboard: x-special/gnome-copied-files ("copy\n<urls>" / "cut\n...") +
  text/uri-list; Clipboard.cutPaths drive FileView cut-dimming; add copyText()
  invokable for "Copy Path". FileOperations controller per pane (property on
  PaneController + engine app-shared singleton internally; Application.clipboard
  + Application.undo properties). Route PaneController createFolder/createFile/
  renameAt through the engine for undo. QML: ConflictDialog, OperationOverlay,
  CreateDialog already needed, RenameEditor exists inline in FileDelegate (docs
  file RenameEditor.qml still expected — make it the extracted editor or thin
  wrapper). Wire DnD (drag from FileDelegate with text/uri-list, DropArea on
  views/sidebar places; Ctrl=copy Shift=move default auto).
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
  config/AppearancePortal (org.freedesktop.portal.Settings Read color-scheme +
  SettingChanged listener; only active when follow_system)). Shortcuts.qml:
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
  should show it on startup). Write data/dev.kromakobra.Fang.desktop,
  metainfo.xml, icons/dev.kromakobra.Fang.svg (draw a simple fang/chevron),
  README.md. Install rules exist in root CMakeLists (already).
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

- Toolbar.qml currently has a broken-ish LocationBar width formula — fix with
  anchors (see TODO above).
- FileView Loader creates views with `required property` — passed via
  sourceComponent items with bindings `fileView: root; pane: root.pane` (OK).
- controls/Menu `closePolicy: 0` used by LocationBar completions popup (valid:
  QQC Popup.NoAutoClose == 0).
- DirectoryModel.setPath emits pathChanged AFTER reset; Selection clears via
  that signal.
- WindowController.addTab has first-tab activeTabChanged workaround (guard in
  setActiveTabIndex made it a no-op) — keep when editing.
- QQC2 style pinned via QQuickStyle::setStyle("Basic") before engine; import
  `QtQuick.Controls.Basic as QQC` in controls/Popup|Tooltip|Menu.
- fang.* logging categories default QtWarningMsg; enable with
  QT_LOGGING_RULES="fang.*=true".
- flake.lock is gitignored intentionally (registry override workflow).
