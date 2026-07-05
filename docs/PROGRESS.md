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

- **Stage 4 — DONE** (commit 8b61648). FileOperationEngine + workers
  (copy/move/delete/trash/restore), TrashManager (freedesktop spec), UndoStack
  (bounded 50, inverse ops with recording suppressed), Clipboard
  (x-special/gnome-copied-files + uri-list, cut-dimming), ConflictDialog +
  OperationOverlay + DnD (delegates/background/sidebar drops, modifier policy).
- **Stage 5 — DONE** (commit c582f13). ThumbnailProvider ("fang-thumb"
  QQuickAsyncImageProvider) + ThumbnailGenerator (freedesktop shared cache
  ~/.cache/thumbnails/{normal,large}, Thumb::URI/MTime keys, EXIF
  autotransform, pluggable registry, size cap + shared-cache statics),
  Previews controller (fileDetails/isProbablyText/readTextPeek), PreviewPane
  (image/text peek/icon+metadata), F11/F7 toggle.
- **Stage 6 — DONE** (commit 3e2c586). SearchEngine (worker-thread manual
  recursion, depth cap/ignore paths/hidden/symlink options, glob/substring/
  regex names, content grep with binary skip + size cap, queued batches),
  SearchQuery (QObject props), SearchResultsModel (DirectoryModel roles +
  matchContext/parentDir), SearchPanel (Ctrl+F, results list, activate opens
  containing dir + selects).
- **Stage 7 — DONE** (this commit). config/ConfigStore (kept toml::table for
  unknown-key-preserving round-trip, QSaveFile atomic writes, schema_version,
  per-group change signals; stateDir = GenericStateLocation + "/fang" per
  docs/06), config/Settings (typed props for every docs/06 key, per-group
  notify + changed(), shortcut(name, fallback) invokable, notifyAllChanged()
  for hot reload), Application.settings exposed; appearance wiring (theme
  name/user themes dir/icon_theme → IconResolver, font override only when
  keys explicitly set, follow_system → config/AppearancePortal reading
  org.freedesktop.portal.Settings color-scheme + SettingChanged, portal only
  instantiated while follow_system=true); previews wiring (enabled/images →
  size-cap collapse, max_file_size_mb, use_shared_cache static, text_peek_kb
  in PreviewPane); [view] defaults seed new PaneControllers; [behavior]:
  open_with → FileView.singleClickOpens, type_ahead filter|jump (new additive
  DirectoryModel.rowMatchingPrefix + jump buffer in FileView), drag_default
  in handleDrop, new_tab_position + general.default_location in
  WindowController::addTab, close_confirm_multiple_tabs popup in Main.qml;
  confirm_delete/confirm_trash both flow through the (generalized)
  deleteConfirm popup; [search] group feeds SearchPanel checkbox defaults +
  query walk limits; Shortcuts.qml resolves every sequence via
  settings.shortcut() with a keymapRevision re-bind trick; session.toml
  save on clean exit / restore in newWindow (windows/tabs/panes: location,
  view mode, sort, hidden, icon size, columns, split, active pane/tab,
  window geometry via new additive WindowController.geometry QRect prop
  mirrored by Main.qml); Application.reloadConfig() = disk reload +
  notifyAllChanged.

**App is in a user-testable state at this commit:** browse, tabs, split view,
sidebar/places/bookmarks, all four view modes, sorting/columns, type-to-filter
or type-to-jump, hidden toggle, rename (F2), create folder/file, open files
via default app, breadcrumb/location editing, zoom slider, toasts,
copy/move/trash/undo/redo/paste/DnD with conflict + progress UI, image
thumbnails, preview pane, threaded search, full config.toml (live-persisted
settings, shortcut remaps, themes incl. user themes + follow_system portal),
session save/restore. NOT yet present: custom actions/open-with/properties
dialogs (stage 8), packaging files (stage 9), tests (stage 10).

## Next: stages 8–10 per the plan below.

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

## Remaining stages 8–10 — plan + decisions already made

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
- This host's toml++ is built WITH exceptions: `toml::parse` throws
  `toml::parse_error` and there is no `toml::parse_result` — wrap in
  try/catch, do not use the `if (!result)` pattern from the docs.
- QStandardPaths::StateLocation appends org+app ("…/state/KromaKobra/Fang");
  docs/06 fixes the path as $XDG_STATE_HOME/fang → use GenericStateLocation
  + "/fang" (ConfigStore::stateDir).
- session.toml is written from QCoreApplication::aboutToQuit — a `timeout`-
  killed run (SIGTERM) never fires it, so headless session tests must quit
  cleanly (e.g. a UI override with a Timer calling Application.quit()).
- Q_INVOKABLE results don't create binding dependencies: Shortcuts.qml keeps
  a keymapRevision int bumped from onShortcutsChanged and reads it inside
  key() so sequence bindings re-evaluate on config reload.
