# 10 — Repository Layout

Target directory structure for the generated codebase. C++ files are `PascalCase` matching
the class; QML files are `PascalCase.qml`. All C++ core in `namespace fang`.

```
fang/
├── CMakeLists.txt                 # top-level: project, Qt, toml++, subdirs, tests
├── LICENSE                        # GPL-3.0-or-later
├── README.md
├── .clang-format
├── .editorconfig
├── .gitignore
├── flake.nix                      # OPTIONAL (dev shell / package for NixOS)
│
├── docs/                          # this specification set
│   ├── 00-overview.md ... 10-repo-layout.md
│   └── PROMPT.md
│
├── data/                          # installed desktop integration assets
│   ├── dev.kromakobra.Fang.desktop
│   ├── dev.kromakobra.Fang.metainfo.xml
│   └── icons/dev.kromakobra.Fang.svg
│
├── src/
│   ├── CMakeLists.txt             # fangcore lib + fang executable + qt_add_qml_module
│   ├── main.cpp                   # bootstrap (see 03 §Application startup)
│   │
│   ├── app/
│   │   ├── Application.{h,cpp}
│   │   ├── WindowController.{h,cpp}
│   │   ├── TabController.{h,cpp}
│   │   └── PaneController.{h,cpp}
│   │
│   ├── fs/
│   │   ├── DirectoryModel.{h,cpp}
│   │   ├── FileSystemWatcher.{h,cpp}
│   │   ├── MimeResolver.{h,cpp}
│   │   ├── IconResolver.{h,cpp}
│   │   └── FileInfo.{h,cpp}       # value type: cached stat/mime/icon per entry
│   │
│   ├── ops/
│   │   ├── FileOperationEngine.{h,cpp}
│   │   ├── Operation.{h,cpp}
│   │   ├── CopyWorker.{h,cpp}
│   │   ├── MoveWorker.{h,cpp}
│   │   ├── DeleteWorker.{h,cpp}
│   │   ├── TrashManager.{h,cpp}
│   │   ├── UndoStack.{h,cpp}
│   │   └── Clipboard.{h,cpp}
│   │
│   ├── nav/
│   │   ├── Navigation.{h,cpp}
│   │   └── Selection.{h,cpp}
│   │
│   ├── search/
│   │   ├── SearchEngine.{h,cpp}
│   │   ├── SearchResultsModel.{h,cpp}
│   │   └── SearchQuery.{h,cpp}
│   │
│   ├── preview/
│   │   ├── ThumbnailProvider.{h,cpp}   # QQuickImageProvider "fang-thumb"
│   │   └── ThumbnailGenerator.{h,cpp}  # image/SVG generators (extensible)
│   │
│   ├── places/
│   │   ├── Places.{h,cpp}
│   │   └── MountMonitor.{h,cpp}        # QStorageInfo + mountinfo + optional udisks2
│   │
│   ├── config/
│   │   ├── ConfigStore.{h,cpp}
│   │   ├── Settings.{h,cpp}
│   │   ├── Theme.{h,cpp}
│   │   ├── ActionRegistry.{h,cpp}
│   │   ├── ActionParser.{h,cpp}
│   │   └── AppearancePortal.{h,cpp}    # opt-in freedesktop portal follow
│   │
│   ├── util/
│   │   ├── Enums.{h}                    # ViewMode, SortRole, OpType, ... (Q_ENUM)
│   │   ├── ShellQuote.{h,cpp}           # safe placeholder expansion/quoting
│   │   ├── HumanFormat.{h,cpp}          # sizes, dates, speeds
│   │   └── Logging.{h,cpp}              # QLoggingCategory definitions
│   │
│   └── ui/                              # bundled default UI (qrc:/Fang/ui/...)
│       ├── Main.qml
│       ├── MainWindow.qml
│       ├── TabBar.qml   TabButton.qml
│       ├── Toolbar.qml  LocationBar.qml  ViewModeSwitch.qml
│       ├── Sidebar.qml  PlaceRow.qml
│       ├── FileView.qml IconsView.qml ListView.qml DetailsView.qml TreeView.qml
│       ├── ColumnHeader.qml FileDelegate.qml
│       ├── PreviewPane.qml StatusBar.qml
│       ├── ConflictDialog.qml OperationOverlay.qml PropertiesDialog.qml
│       ├── OpenWithDialog.qml CreateDialog.qml RenameEditor.qml
│       ├── SearchPanel.qml ContextMenu.qml NotificationOverlay.qml
│       ├── Shortcuts.qml
│       ├── controls/     # Button, IconButton, TextField, Menu, Popup, ScrollBar, ...
│       └── themes/       # dark.toml, light.toml (bundled) + default actions.toml
│
├── tests/
│   ├── CMakeLists.txt
│   ├── tst_fileops.cpp  tst_trash.cpp  tst_undo.cpp  tst_search.cpp
│   ├── tst_config.cpp   tst_actions.cpp tst_mime.cpp tst_model.cpp
│   ├── tst_thumbnails.cpp
│   └── tst_qml_smoke.cpp
│
└── third_party/                    # vendored deps if not found (e.g. tomlplusplus)
```

## Notes

- **`fangcore` static library** contains everything in `src/` except `main.cpp` and the
  QML module registration, so `tests/` can link and exercise the core headlessly.
- The `fang` executable = `fangcore` + `main.cpp` + `qt_add_qml_module` (default UI +
  bundled themes/actions as resources).
- Bundled `themes/` and `actions.toml` are compiled into the binary via the QML module
  resources and used as defaults when user files are absent.
- Keep each C++ class in its own translation unit; keep QML components small and single-
  purpose so overrides can cherry-pick.
