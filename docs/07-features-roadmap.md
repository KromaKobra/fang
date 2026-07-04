# 07 — Features & Roadmap

## v1 feature checklist (must ship)

### Navigation & windows
- [ ] Multi-process launch; `fang [path...]`, `--select <file>`, `--version`, `--help`.
- [ ] Tabs: add, close, reorder (drag), duplicate, middle-click close, `Ctrl+Tab` cycle.
- [ ] Split view (two independent panes) with drag/copy between panes.
- [ ] Back/forward/up history per pane; Home.
- [ ] Breadcrumb location bar with per-segment sibling dropdowns; editable path mode
      (`Ctrl+L`) with autocomplete.
- [ ] Session restore (optional).

### Views
- [ ] Icons, List, Details, Tree view modes.
- [ ] Details/Tree: customizable, reorderable, resizable, sortable columns from the full
      role set (name, size, type, modified, created, accessed, permissions, owner, group,
      extension).
- [ ] Sort by any column; folders-first; natural name sort; asc/desc.
- [ ] Show/hide hidden files; adjustable icon size / row density (zoom).
- [ ] Live type-to-filter of current listing.

### Sidebar / places
- [ ] Places (Home + existing XDG user dirs), Bookmarks (add/remove/reorder), Devices
      (with capacity + mount/unmount/eject via udisks2 when available), Network (mounted
      gvfs), Trash (with count badge).
- [ ] Drop files onto a place to copy/move.

### File operations
- [ ] Copy, Move, Delete (permanent), Trash, Restore-from-trash.
- [ ] Threaded with progress (speed/ETA), pause/resume/cancel.
- [ ] Conflict resolution dialog (overwrite/skip/keep-both/rename/merge, apply-to-all).
- [ ] Multi-level Undo/Redo (move/rename/trash/copy/create).
- [ ] Cut/Copy/Paste via system clipboard (freedesktop interop).
- [ ] Create folder / empty file / from template; inline rename.
- [ ] Freedesktop Trash spec (per-volume trash, restore, empty).

### Previews
- [ ] Image thumbnails (raster + SVG, EXIF-aware) via shared freedesktop cache.
- [ ] Preview pane: image, text/code peek, or icon + metadata.

### Search & filter
- [ ] Threaded recursive search (name glob/substring; optional content grep; size/date/
      mime filters); streamed, cancellable results shown in the view.
- [ ] Instant type-to-filter (separate from full search).

### Extensibility & launching
- [ ] TOML custom context-menu actions with placeholders + bundled defaults.
- [ ] Open / Open-With via xdg/gio + `.desktop`; "Open With…" picker; set default app.
- [ ] Properties dialog (general, permissions, open-with; async recursive dir size).

### Theming & override
- [ ] Self-owned theme system (bundled dark default, light variant, user themes).
- [ ] Opt-in follow-system light/dark via freedesktop appearance portal.
- [ ] Full UI override from `~/.config/fang/ui/` with safe fallback on error.
- [ ] Configurable icon theme; configurable keyboard shortcuts.

### Quality
- [ ] Qt Test unit tests for core (file ops, trash, undo, search, config, actions, mime).
- [ ] Headless QML smoke test (loads default UI, basic model interaction).
- [ ] Never blocks GUI thread; never crashes on filesystem errors.

## Explicitly deferred (documented non-goals for v1)

See [00-overview.md](00-overview.md#non-goals-for-v1). Summary: native remote protocols,
native/C++ plugin ABI, single-instance daemon, video/PDF/audio thumbnails, tagging/
semantic index, VCS overlays, batch-rename engine.

## Phased roadmap

**Phase 2 — Power features**
- Batch/advanced rename engine (patterns, counters, regex, case).
- More thumbnail generators: video (ffmpeg), PDF (poppler), audio cover art, fonts.
- Embedded terminal panel (toggle) using the configured terminal.
- KDE/freedesktop ServiceMenu `.desktop` parsing (interop with existing Dolphin actions).
- Dual-pane keyboard-driven operations (F5 copy / F6 move like classic managers).

**Phase 3 — Virtual filesystems**
- Native VFS abstraction: SFTP, SMB, MTP, and archives-as-folders, layered under the
  existing file-access API so models/ops/previews work transparently.
- Remote bookmarks and credential handling.

**Phase 4 — Indexing & integration**
- Optional background content/metadata index for instant search.
- File tagging and coloring; saved searches.
- Version-control status overlays (git).
- Optional single-instance mode + D-Bus interface for scripting.

Each phase must preserve the v1 QML API (additive only) so existing overrides keep working.
