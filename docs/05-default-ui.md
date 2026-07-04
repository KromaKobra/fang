# 05 — Default UI Specification

The bundled default UI lives under `qrc:/Fang/ui/` (source: `src/ui/`). It is built from
**custom Qt Quick components** (no styled QQC2 widgets), references only Theme tokens for
appearance, and drives the controllers from [04-qml-api.md](04-qml-api.md). It doubles as
the reference implementation an override author reads.

## Window layout

```
┌──────────────────────────────────────────────────────────────────────┐
│  TAB BAR:  [ Documents ×][ Downloads ×][ + ]                          │
├──────────────────────────────────────────────────────────────────────┤
│  TOOLBAR:  ◀ ▶ ▲  ⟳ | Home  |  breadcrumb / editable location  | 🔍   │
│            ...............................  | view: ▤ ▥ ▦ ▧ | ⚙        │
├───────────────┬──────────────────────────────────────────────────────┤
│  SIDEBAR      │  FILE VIEW (single pane, or split into two)           │
│  Places       │                                                       │
│   Home        │   [icons / list / details / tree]                     │
│   Documents   │                                                       │
│   Downloads   │                                                       │
│  Bookmarks    │                                                       │
│   ...         │                                                  ┌────┤
│  Devices      │                                                  │ P  │
│   / 40G free  │                                                  │ R  │
│   USB ⏏       │                                                  │ E  │
│  Network      │                                                  │ V  │
│  Trash (12)   │                                                  │    │
├───────────────┴──────────────────────────────────────────────────────┤
│  STATUS BAR:  120 items, 3 selected (18.4 MB)      | zoom ▁▂▃  | free  │
└──────────────────────────────────────────────────────────────────────┘
```

Optional overlays: operation-progress popovers, conflict dialog, rename inline editor,
create-folder dialog, properties dialog, open-with dialog, search panel.

## Component inventory (files under `src/ui/`)

**Shell**
- `Main.qml` — root; creates a `Window`, one `WindowController`, wires global shortcuts,
  hosts `NotificationOverlay` and `OperationOverlay`.
- `MainWindow.qml` — the window chrome: `TabBar` + `Toolbar` + body (`SplitContainer`) +
  `StatusBar`.
- `TabBar.qml` / `TabButton.qml` — tabs with close buttons, `+`, drag-reorder, middle-click
  close, double-click-empty to add.

**Navigation / toolbar**
- `Toolbar.qml` — back/forward/up/refresh/home buttons, `LocationBar`, search toggle,
  `ViewModeSwitch`, settings/menu button.
- `LocationBar.qml` — breadcrumb mode (segmented, clickable, each segment a dropdown of
  siblings) that switches to an editable text field on click/`Ctrl+L`, with path
  autocomplete.
- `ViewModeSwitch.qml` — icons/list/details/tree selector.

**Sidebar**
- `Sidebar.qml` — sections from `Application.places`; collapsible; drag-reorder bookmarks;
  drop targets (drop files onto a place to copy/move there); eject/unmount buttons;
  right-click context menu (rename/remove bookmark, unmount, open in new tab).
- `PlaceRow.qml` — one entry with icon, label, capacity bar (devices), badge (trash count).

**File views** (all consume `pane.model`)
- `FileView.qml` — switches between the four view delegates based on `pane.viewMode`;
  owns shared selection/keyboard/drag-drop/context-menu behavior.
- `IconsView.qml` — `GridView`; thumbnail or themed icon + elided name; adjustable icon
  size.
- `ListView.qml` — compact single-column names (multi-column wrap).
- `DetailsView.qml` — `TableView`-style: header row with reorderable/resizable/sortable
  columns from `pane.columns`; rows render the corresponding roles.
- `TreeView.qml` — hierarchical, lazy-expanding, same columns as Details.
- `ColumnHeader.qml` — drag to reorder, drag edge to resize, click to sort (asc/desc),
  right-click to choose visible columns from the full role set.
- `FileDelegate.qml` — shared delegate bits: selection highlight, rename inline editor,
  cut-dimming, drag source, hover.

**Preview pane**
- `PreviewPane.qml` — collapsible right dock; for a single selection shows: image
  thumbnail (large) for images; monospace text peek for text/code (first N KB); otherwise
  a large themed icon + metadata (name, type, size, modified, permissions, owner).

**Dialogs / overlays**
- `ConflictDialog.qml` — per-conflict: shows both files' details; buttons Overwrite / Skip
  / Keep Both / Rename / Merge(dirs); "apply to all remaining" checkbox → calls
  `operation.resolveConflict`.
- `OperationOverlay.qml` — stacked progress cards for active `FileOperations.operations`
  (name, progress, speed, ETA, pause/resume/cancel).
- `PropertiesDialog.qml` — tabs: General (name, type, location, size incl. recursive dir
  size computed async, timestamps), Permissions (rwx for owner/group/other, sticky),
  Open-With default selector.
- `OpenWithDialog.qml` — lists `MimeResolver.appsFor(mime)`; "always use" toggle.
- `CreateDialog.qml` — new folder / new empty file / from template.
- `RenameEditor.qml` — inline; selects basename (not extension) by default; batch rename
  entry point (Phase 2, stubbed disabled in v1 menu).
- `SearchPanel.qml` — query builder (name, content toggle, filters) bound to
  `Application.search`; results shown in the file view via `SearchResultsModel`.
- `ContextMenu.qml` — file/background context menus (see below).
- `NotificationOverlay.qml` — transient toasts for `Application.error` and op summaries.
- `StatusBar.qml` — item/selection counts + aggregate size, zoom slider, free space.

**Primitives** (`src/ui/controls/`): `Button`, `IconButton`, `ToolButton`, `TextField`,
`Menu`, `MenuItem`, `Popup`, `Tooltip`, `ScrollBar`, `Splitter`, `Checkbox`, `Slider`,
`Badge`, `Spinner`, `Separator`. All themed via Theme tokens; **do not** rely on QQC2
styling. Keep them minimal and lightweight.

## Context menus

**On file(s):** Open, Open With ▸, Open in New Tab, Open in New Window, — , Cut, Copy,
Copy Path, Paste-into (if dir), — , Rename (F2), Move to Trash (Del), Delete (Shift+Del),
— , Compress ▸ / custom actions from `ActionRegistry.actionsFor(selection)`, — ,
Properties (Alt+Enter).

**On empty background:** Paste, — , New Folder (Ctrl+Shift+N), New File, New from Template
▸, — , View ▸ (mode, sort, columns, show hidden, icon size), Open Terminal Here (custom
action), — , Properties.

Custom actions are inserted where their TOML declares (default: above Properties).

## Interaction rules (v1 defaults, all configurable)

- **Single-click opens** files/folders; hover to preselect. Rubber-band select on empty
  drag. `Ctrl+click` toggles, `Shift+click`/`Shift+arrows` range-select. (A config switch
  flips to double-click-open / single-click-select.)
- **Keyboard:** arrows/Home/End/PageUp/Down navigate; type-to-search jumps to matching
  name (and feeds `filterText` when the filter bar is open); Enter opens; Backspace/Alt+↑
  go up; Alt+←/→ back/forward.
- **Drag & drop:** internal DnD moves within same filesystem, copies across; `Ctrl` forces
  copy, `Shift` forces move; drop onto folder/place enqueues an op with the conflict flow;
  external DnD (to/from other Wayland apps) via `text/uri-list`.
- **Split view:** `F3` toggles; drag/copy between panes; each pane independent
  location/view/selection; active pane highlighted.
- **Zoom:** `Ctrl+scroll` / `Ctrl+±` adjusts icon size / row density.

## Keyboard shortcuts (defaults)

| Action | Shortcut |
|--------|----------|
| New tab / Close tab | `Ctrl+T` / `Ctrl+W` |
| New window | `Ctrl+N` |
| Next/prev tab | `Ctrl+Tab` / `Ctrl+Shift+Tab` |
| Back / Forward / Up | `Alt+←` / `Alt+→` / `Alt+↑` |
| Reload | `F5` |
| Location bar | `Ctrl+L` |
| Focus search | `Ctrl+F` |
| Toggle hidden files | `Ctrl+H` |
| Toggle split view | `F3` |
| Toggle preview pane | `F11` (or `F7`) |
| Select all / Invert | `Ctrl+A` / `Ctrl+Shift+A` |
| Copy / Cut / Paste | `Ctrl+C` / `Ctrl+X` / `Ctrl+V` |
| Rename | `F2` |
| Move to Trash / Delete | `Del` / `Shift+Del` |
| New folder | `Ctrl+Shift+N` |
| Properties | `Alt+Enter` |
| Undo / Redo | `Ctrl+Z` / `Ctrl+Shift+Z` |
| View modes | `Ctrl+1..4` (icons/list/details/tree) |
| Zoom in/out/reset | `Ctrl++` / `Ctrl+-` / `Ctrl+0` |
| Quit | `Ctrl+Q` |

Shortcuts are defined centrally (a `Shortcuts.qml` map bound from config) so the user/
override can remap them; the table above is the default map in `config.toml`.

## Appearance

- All colors/metrics/typography come from `Application.theme`. No literal colors in
  components.
- Default theme is **dark** and is applied before first paint (no flash of unstyled/light
  content).
- Icons via `iconName` roles + `QIcon::fromTheme` (configured icon theme); thumbnails via
  `image://fang-thumb/<path>` with a graceful icon fallback while loading.
