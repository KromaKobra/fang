# RICING — customizing Fang

Fang is built to be re-skinned, re-arranged, or completely rebuilt. This guide covers
every customization layer, from a color tweak to replacing the whole interface. All
paths below live under `$XDG_CONFIG_HOME/fang/` (usually `~/.config/fang/`).

| Layer | File(s) | Effort |
|-------|---------|--------|
| [Settings](#1-settings-configtoml) | `config.toml` | seconds |
| [Themes](#2-themes) | `themes/<name>.toml` | minutes |
| [Custom icons](#3-custom-icons) | `icons/<name>.svg` | minutes |
| [Custom actions](#4-custom-actions-actionstoml) | `actions.toml` | minutes |
| [UI override](#5-replacing-the-ui) | `ui/main.qml` (+ anything) | hours–days |

Nothing here can brick Fang: a broken theme falls back to the bundled dark theme, a
broken override logs the error, shows a banner, and loads the bundled UI.

---

## 1. Settings (`config.toml`)

The full schema with defaults is in [06-config-theming.md](06-config-theming.md).
Highlights for ricers:

```toml
[behavior]
open_with = "double"   # "double" (default): click selects, double-click/Enter opens
                       # "single": click opens immediately
type_ahead = "filter"  # typing filters the listing; "jump" selects the first match

[view]
default_mode = "details"          # icons | list | details | tree
preview_pane = true               # show the preview/details pane on start

[appearance]
theme       = "dark"              # bundled: "dark", "light"; or your theme name
icon_theme  = "auto"              # any installed XDG icon theme name
follow_system = false             # true = follow the desktop light/dark portal

[shortcuts]
# Override any binding from the docs/05 table by name:
new_tab = "Ctrl+T"
properties = "Alt+Return"
```

Settings persist live — changes made by the app are written back atomically and
unknown keys are preserved. `Application.reloadConfig()` re-reads the file at runtime.

## 2. Themes

Drop `~/.config/fang/themes/mytheme.toml` and set `appearance.theme = "mytheme"`.
A theme maps 1:1 onto the `Theme` QML object — copy the bundled dark theme from
[06-config-theming.md](06-config-theming.md#default-theme) as a starting point.
Every color, radius, spacing, row height and font in the default UI comes from these
tokens, so a theme swap recolors everything instantly (use
`Application.reloadTheme()` to iterate without restarting).

The default UI never hard-codes a color. If you write an override, do the same:
read `Application.theme.accent`, not `"#89b4fa"`.

## 3. Custom icons

Files that don't get a live thumbnail (everything except images in v1) are drawn with
a themed icon. You can override any of them **per icon name** without installing a
full icon theme: put an `.svg` (or `.png`) named after the freedesktop icon name into
`~/.config/fang/icons/`:

```
~/.config/fang/icons/
├── folder.svg                    # all directories
├── folder-downloads.svg          # ~/Downloads in the sidebar
├── text-x-generic.svg            # plain text files
├── application-pdf.svg           # PDFs
├── video-x-generic.svg           # any video file
├── application-x-archive.svg     # tars/zips (name varies per mime type)
└── utilities-terminal.svg        # also used by menu/action entries
```

How a file's icon name is chosen and resolved:

1. The icon name is the file's **mime-type icon name** (`QMimeType::iconName()`), e.g.
   `text-x-python`, `image-jpeg`, `application-pdf`; directories use `folder` (XDG
   user dirs get specific names like `folder-music` in the sidebar).
2. Fang walks a fallback chain for each file: exact name → its generic variant
   (`text-x-python` → `text-x-generic`) → `text-x-generic` →
   `application-octet-stream` → `unknown`.
3. **At every step your `~/.config/fang/icons/` file wins over the icon theme.** So
   `text-x-generic.svg` restyles all text-ish files at once, while `text-x-python.svg`
   targets just Python.
4. If nothing matches anywhere, Fang paints a minimal built-in glyph.

To find the icon name for a given file, select it and check the preview pane, or run
Fang with `QT_LOGGING_RULES="fang.*=true"` — or just check the file's mime type with
`xdg-mime query filetype <file>` and swap `/` for `-`.

Icons render at many sizes (16 px rows up to 256 px grid zoom) — prefer SVG.
Note: for image files a real thumbnail (from the shared `~/.cache/thumbnails`) covers
the icon once generated; your icon still shows while it loads. Disable thumbnails with
`previews.images = false` if you want icons everywhere.

## 4. Custom actions (`actions.toml`)

Add context-menu entries that run shell commands. Full schema in
[06-config-theming.md](06-config-theming.md#actionstoml--custom-actions).

```toml
[[action]]
id          = "open-terminal-here"   # same id as a bundled action = override it
label       = "Open Terminal Here"
icon        = "utilities-terminal"   # themed or ~/.config/fang/icons/ name
command     = "kitty --directory %d"
applies_to  = "background"           # files | background | both
```

Placeholders: `%f` first selected path, `%F` all selected paths, `%u`/`%U` URLs,
`%d` current directory, `%n` basename, `%%` literal `%`. Every expansion is
shell-quoted by Fang — a file named `; rm -rf ~` is passed as an inert argument.
Filter by `mime_types = ["text/*"]`, `extensions = ["zip"]`, `min_selection`/
`max_selection`; set `run_in_terminal = true` to wrap in your
`general.terminal_command`. `Application.reloadActions()` hot-reloads the file.

## 5. Replacing the UI

The nuclear option: Fang loads **your** QML as the root component.

### How it works

1. On startup Fang looks for `~/.config/fang/ui/main.qml`.
2. If present, it is loaded instead of the bundled UI, and `~/.config/fang/ui/` is
   added to the QML import path (so you can split your rice into modules/files).
3. If your QML fails to load, Fang logs the error, shows a banner, and falls back to
   the bundled UI. You never lose your file manager to a typo.

QML never touches the filesystem directly — every operation goes through the C++
controllers below. That's the whole trick: your UI is just bindings and signal
handlers around a stable API.

### Minimal working override

```qml
// ~/.config/fang/ui/main.qml
import QtQuick
import Fang

Window {
    id: root
    visible: true
    width: 900; height: 600
    color: Application.theme.background
    title: win.activeTab ? win.activeTab.title : "Fang"

    property WindowController win: Application.newWindow()
    readonly property PaneController pane: win.activeTab.activePane

    ListView {
        anchors.fill: parent
        model: pane.model                       // DirectoryModel
        delegate: Text {
            required property var model
            text: (model.isDir ? "▸ " : "  ") + model.name
            color: model.selected ? Application.theme.accent
                                  : Application.theme.foreground
            font.pixelSize: 15
            MouseArea {
                anchors.fill: parent
                onClicked: pane.selection.select(model.index)
                onDoubleClicked: pane.activate(model.index)   // open
            }
        }
    }

    Shortcut { sequence: "Alt+Up";   onActivated: pane.navigation.up() }
    Shortcut { sequence: "Alt+Left"; onActivated: pane.navigation.back() }
}
```

That's a browsable file manager in ~35 lines. Grow it from there.

### What you get with `import Fang`

The complete, versioned contract is [04-qml-api.md](04-qml-api.md) — that document
is the source of truth; this is the orientation map. Guard against future changes
with `Application.apiVersion` (currently `1`; only additive changes within a version).

**Singletons**

- `Application` — the root of everything: `theme`, `settings`, `places`, `search`,
  `actions`, `clipboard`, `undo`, `previews`; `newWindow()`, `openPath(path)`,
  `formatSize(bytes)`, `reloadTheme()/reloadActions()/reloadConfig()`, `quit()`;
  `error(title, detail)` signal for your notification toasts.
- `MimeResolver` — `mimeTypeFor(path)`, `appsFor(mime)`, `openDefault(urls)`,
  `openWith(urls, desktopId)`, `setDefaultApp(mime, desktopId)` for open-with UIs.

**Controller tree** (what the default UI is built around)

```
Application.newWindow() → WindowController      tabs, addTab/closeTab, split
  └─ TabController                              leftPane/rightPane/activePane
      └─ PaneController                         THE workhorse, one per view:
          ├─ .model      DirectoryModel         the listing (QAbstractItemModel)
          ├─ .navigation Navigation             back/forward/up/breadcrumbs/goTo
          ├─ .selection  Selection              select/toggle/range/selectAll
          └─ .fileOperations FileOperations     copy/move/trash/… → Operation*
```

`PaneController` also carries the per-view state you'll bind your chrome to:
`location`, `viewMode`, `sortRole`, `sortDescending`, `showHidden`, `filterText`,
`iconSize`, `columns`, `loading`, `itemCount`, `totalSize` — all read/write where it
makes sense, plus `open(path)`, `activate(row)`, `openInNewTab(row)`, `refresh()`,
`createFolder(name)`, `renameAt(row, newName)` and friends.

**Model roles** — `DirectoryModel` exposes `name, path, url, isDir, isSymlink, size,
sizeText, modified, created, accessed, mimeType, typeText, permissions, owner, group,
iconName, thumbnail, isHidden, selected, extension` on every row. Search results add
`matchContext` and `parentDir`.

**Long-running work** comes back as objects with progress: `FileOperations.copy(...)`
returns an `Operation` (`progress`, `speedBytesPerSec`, `etaSeconds`, `pause()`,
`cancel()`, `conflict` signal). `Application.search` runs queries with a live
`SearchResultsModel`. `Application.previews` gives text peeks, file details maps and
async recursive dir sizes. Your UI stays responsive for free — everything heavy is on
worker threads.

**Images** — two providers:

- `image://fang-icon/<name>` — themed icon by name (respects your
  `~/.config/fang/icons/` overrides, section 3).
- `image://fang-thumb<absolute-path>` — async thumbnail; the `thumbnail` model role
  already contains the full URL for files that have one.

**Enums** — `Fang.ViewMode.Icons/List/Details/Tree`, `Fang.SortRole.*`,
`Fang.OpType.*`, `Fang.OpState.*`, `Fang.ConflictMode.*`, `Fang.ClipboardMode.*`,
`Fang.PlaceSection.*`.

**Theme tokens** — `Application.theme.<token>` for all colors
(`background`, `surface`, `accent`, `selectionBackground`, …), metrics
(`rowHeight`, `spacingMedium`, `radiusSmall`, …) and fonts (`fontFamily`,
`fontFamilyMono`, `fontSizeNormal`, …). Full list in
[04-qml-api.md §Theme](04-qml-api.md#theme). They update live on theme reload.

### Reusing bundled components

The default UI is intentionally split into small, single-purpose components
(`Sidebar`, `Toolbar`, `LocationBar`, `FileView`, `PreviewPane`, the `controls/*`
primitives, …). Your override can import and compose them instead of starting from
zero:

```qml
import Fang                       // controllers + models
import "qrc:/Fang/ui" as Default  // bundled components
import "qrc:/Fang/ui/controls" as C

// e.g. keep Fang's file views but build your own chrome:
Default.FileView { pane: myPane; anchors.fill: parent }
C.Button { text: "Up"; onClicked: myPane.navigation.up() }
```

Their source lives in this repo under `src/ui/` — read it as a cookbook; it uses
exactly the same public API your override does (plus a couple of signals wired by
the chrome: look at how `MainWindow.qml` connects `FileView`'s
`requestCreateFolder/requestProperties/…` signals to dialogs).

### Rules & tips

- **No filesystem I/O from QML.** `XMLHttpRequest`/`Qt.labs.folderlistmodel` etc. are
  not the supported path; drive the controllers.
- Single `Window` per process; Fang is multi-process (`Application.openPath()` spawns
  a new one).
- Bind colors/metrics to `Application.theme` so themes keep working under your rice.
- Respect `Application.settings` where you can (e.g. `settings.openWith === "single"`)
  so behavior config keeps working too.
- Debugging: run from a terminal with
  `QT_FORCE_STDERR_LOGGING=1 QT_LOGGING_RULES="fang.*=true" fang ~`.
  Load errors for your override appear there and in the fallback banner
  (`Application.overrideError`).
- Iterate fast: keep a second terminal running `fang ~` — each launch is an
  independent process with your latest QML.
