# 04 — The QML API Contract

This is the **stable, versioned surface** between the C++ core and the QML view layer. The
bundled default UI and any user override consume exactly this API. Treat it as a public
contract: additive changes are allowed within an API version; breaking changes bump
`Application.apiVersion`.

All types are registered in the QML module **`Fang`** (import as `import Fang`).

---

## Override mechanism

### How an override is loaded

1. On startup Fang looks for `~/.config/fang/ui/main.qml`.
2. If present, that file is loaded as the **root component** instead of the bundled
   `qrc:/Fang/ui/Main.qml`, and `~/.config/fang/ui/` is added to the QML import path.
3. The override may `import Fang` to reach all controllers/models/enums/singletons below,
   and may optionally `import` bundled default components if it wants to reuse them.
4. If the override throws a QML error at load, Fang logs it, shows a minimal native error
   banner, and falls back to the bundled UI. **A broken override never bricks Fang.**

### What the root component must provide

The root `main.qml` must instantiate at least one top-level window. The recommended
(but not enforced) shape:

```qml
import QtQuick
import QtQuick.Window
import Fang

Window {
    id: root
    visible: true
    width: 1100; height: 700
    title: win.activeTab ? win.activeTab.title : "Fang"

    // Obtain a window controller from the app singleton.
    property WindowController win: Application.newWindow()

    // ... build UI against `win`, its tabs, and each tab's panes ...
}
```

An override is free to ignore most of the default UX and build something entirely
different, as long as it drives the same controllers. The **minimum viable override** only
needs to: create a window, create/read a `WindowController`, show at least one
`PaneController`'s `model`, and call `pane.activate()`/`pane.open()`. Everything else
(tabs, split, previews, search) is optional to surface.

### Override capability matrix

| Capability | Available to overrides |
|-----------|------------------------|
| All controllers/models/enums | Yes (`import Fang`) |
| Thumbnails via `image://fang-thumb/...` | Yes |
| Theme palette/metrics | Yes (`Application.theme`) |
| Reuse bundled components | Yes (optional import) |
| Custom actions list | Yes (`Application.actions`) |
| Direct filesystem I/O from QML/JS | **No** — must go through controllers |
| Loading native/C++ plugins | **No** in v1 |

### Versioning & compatibility

- `Application.apiVersion : int` — starts at `1`. Overrides may guard:
  `if (Application.apiVersion < 1) { /* show incompatibility notice */ }`.
- Within a major API version, only **additive** changes occur (new properties/methods/
  roles/enums). Existing names/semantics never change.
- Breaking changes increment `apiVersion` and are documented in a `CHANGELOG` API section.
- Each role name, enum value, property, method, and signal in this document is part of the
  contract for API v1.

---

## Singletons

### `Application` (QML_SINGLETON)

| Member | Type | Notes |
|--------|------|-------|
| `apiVersion` | `int` (prop) | `1` |
| `appVersion` | `string` (prop) | e.g. `"0.1.0"` |
| `theme` | `Theme*` (prop) | active theme |
| `settings` | `Settings*` (prop) | config access |
| `places` | `Places*` (prop) | sidebar model |
| `search` | `Search*` (prop) | global search |
| `actions` | `ActionRegistry*` (prop) | custom actions |
| `clipboard` | `Clipboard*` (prop) | shared clipboard |
| `undo` | `UndoStack*` (prop) | shared undo/redo |
| `newWindow()` | `WindowController*` | create a window controller |
| `openPath(string)` | void | open in new process/window |
| `quit()` | void | |
| `reloadTheme()/reloadActions()/reloadConfig()` | void | hot reload |
| signal `error(string title, string detail)` | | surface as notification |
| signal `themeChanged()` | | |

---

## Controllers

### `WindowController`
Props: `tabs : list<TabController>`, `activeTabIndex : int`, `activeTab : TabController`,
`splitView : bool`.
Methods: `addTab(path) : TabController`, `closeTab(i)`, `moveTab(from,to)`,
`duplicateTab(i)`, `toggleSplit()`, `focusOtherPane()`.

### `TabController`
Props: `leftPane : PaneController`, `rightPane : PaneController` (null unless split),
`activePane : PaneController`, `title : string`, `splitView : bool`.

### `PaneController`
Props: `location`, `model : DirectoryModel*`, `navigation : Navigation*`,
`selection : Selection*`, `fileOperations : FileOperations*`, `viewMode : ViewMode`,
`sortRole : SortRole`, `sortDescending : bool`, `foldersFirst : bool`,
`columns : list<string>`, `showHidden : bool`, `filterText : string`, `iconSize : int`,
`loading : bool`, `itemCount : int`, `totalSize : qint64`.
Methods: `open(pathOrUrl)`, `activate(row)`, `openInNewTab(row)`, `openInNewWindow(row)`,
`refresh()`, `setColumns(list)`, `moveColumn(from,to)`, `setColumnWidth(key,px)`,
`setSort(role,desc)`, `createFolder(name)`, `createFile(name)`, `renameAt(row,newName)`.
Signals: `locationChanged`, `activated(row)`, `error(title,detail)`.

### `Navigation`
Props: `canGoBack`, `canGoForward`, `canGoUp`, `breadcrumbs : list<Crumb>`
(`Crumb{label,url}`), `editableText : string`.
Methods: `back()`, `forward()`, `up()`, `home()`, `refresh()`, `goTo(url)`,
`commitEdit(text)` (parse + validate typed location).

### `Selection`
Props: `count : int`, `selectedUrls : list<url>`, `hasSelection : bool`,
`selectedSize : qint64`, `current : int`, `anchor : int`, `allSelected : bool`.
Methods: `select(row)`, `toggle(row)`, `range(from,to)`, `selectAll()`, `clear()`,
`invert()`, `isSelected(row) : bool`.

### `Clipboard`
Props: `hasFiles : bool`, `mode : ClipboardMode (Copy|Cut)`.
Methods: `copy(urls)`, `cut(urls)`, `paste(destDir) : Operation*`, `clear()`.

### `FileOperations`
Props: `operations : list<Operation>` (active/recent).
Methods: `copy(urls,destDir):Operation*`, `move(...):Operation*`,
`remove(urls):Operation*`, `trash(urls):Operation*`, `restore(trashItems):Operation*`,
`rename(url,newName):Operation*`, `createFolder(destDir,name):Operation*`.

### `Operation`
Props: `id`, `type : OpType`, `state : OpState`, `progress : real`, `bytesDone`,
`bytesTotal`, `filesDone`, `filesTotal`, `currentSource`, `currentDest`,
`speedBytesPerSec`, `etaSeconds`, `errorText`.
Methods: `pause()`, `resume()`, `cancel()`, `resolveConflict(mode:ConflictMode, applyToAll:bool)`.
Signals: `conflict(sourceUrl, destUrl, ConflictInfo info)`, `finished()`, `failed(text)`.

### `Search`
Props: `results : SearchResultsModel*`, `running : bool`, `dirsScanned : int`,
`matchCount : int`, `query : SearchQuery` (settable sub-object).
Methods: `start(query)`, `cancel()`, `clear()`.

### `Places`
Is itself a `QAbstractListModel` (use directly in a `ListView`/`Repeater`). Roles:
`label`, `iconName`, `url`, `section` (`Places|Bookmarks|Devices|Network|Trash`),
`isMounted`, `isEjectable`, `usedBytes`, `totalBytes`, `itemCount` (trash).
Methods: `addBookmark(url,label)`, `removeBookmark(id)`, `moveBookmark(from,to)`,
`mount(id)`, `unmount(id)`, `eject(id)`, `activate(url)` (returns url to open).

### `ActionRegistry`
Methods: `actionsFor(selection) : list<Action>`, `run(actionId, selection)`.
`Action{ id, label, iconName, hasSubmenu, submenuLabel }`.

### `UndoStack`
Props: `canUndo`, `canRedo`, `undoText`, `redoText`. Methods: `undo()`, `redo()`.

### `Settings`
Typed props mirroring `config.toml` groups (see [06-config-theming.md](06-config-theming.md)).
All are read/write and persist on change. Signal per group + `changed()`.

### `Theme`
See [Theme properties](#theme) below.

---

## Models

### `DirectoryModel` (QAbstractItemModel)
Roles: exactly the role names in [03-core-backend.md §4](03-core-backend.md#4-directorymodel-qabstractitemmodel):
`name, path, url, isDir, isSymlink, size, sizeText, modified, created, accessed,
mimeType, typeText, permissions, owner, group, iconName, thumbnail, isHidden, selected,
extension`. Tree view uses the model hierarchy (`hasChildren`, lazy `fetchMore`).

### `SearchResultsModel` (QAbstractListModel)
Same roles as `DirectoryModel` plus `matchContext : string` (snippet for content hits) and
`parentDir : string`.

---

## Enums (all `Q_ENUM`, reachable as `Fang.<Enum>.<Value>`)

- `ViewMode { Icons, List, Details, Tree }`
- `SortRole { ByName, BySize, ByType, ByModified, ByCreated, ByAccessed, ByExtension, ByOwner }`
- `OpType { Copy, Move, Delete, Trash, Restore, Rename, CreateFolder }`
- `OpState { Queued, Running, Paused, WaitingConflict, Completed, Failed, Cancelled }`
- `ConflictMode { Overwrite, Skip, Rename, Merge, KeepBoth }`
- `ClipboardMode { Copy, Cut }`
- `PlaceSection { Places, Bookmarks, Devices, Network, Trash }`

---

## Theme

`Application.theme` exposes (all `Q_PROPERTY`, live-updating on reload / system-follow):

**Colors** (string hex/`color`): `background`, `backgroundAlt`, `surface`,
`surfaceHover`, `surfaceActive`, `foreground`, `foregroundMuted`, `accent`,
`accentForeground`, `border`, `selectionBackground`, `selectionForeground`, `error`,
`warning`, `success`, `shadow`, `scrollbar`, `scrollbarHover`.

**Metrics** (real/int): `radiusSmall`, `radiusMedium`, `radiusLarge`, `spacingSmall`,
`spacingMedium`, `spacingLarge`, `rowHeight`, `toolbarHeight`, `sidebarWidth`,
`iconSizeSmall`, `iconSizeMedium`, `iconSizeLarge`.

**Typography:** `fontFamily`, `fontFamilyMono`, `fontSizeSmall`, `fontSizeNormal`,
`fontSizeLarge`.

**State:** `name : string` (active theme name), `isDark : bool`, `followSystem : bool`.

Default values are defined in [06-config-theming.md](06-config-theming.md#default-theme).
The UI must reference these tokens rather than hard-coding colors, so a theme swap
recolors the whole app instantly.
