# 00 — Overview

## Vision

Fang is a lightweight, native Linux file manager whose graphical interface is entirely
QML and can be fully replaced by the user. It targets the feature richness of KDE
Dolphin but ships without the Plasma/KDE runtime dependency, is Wayland-first, and gives
the user complete control over appearance and interaction.

The name is **Fang**; the author is **Kroma Kobra**; the product category is **file
manager**.

## Primary goals

- **G1 — Overridable GUI.** The complete visual layer is QML loaded over a stable C++
  API. A developer can create a QML project, place it in `~/.config/fang/`, and it
  replaces the default GUI without recompiling Fang.
- **G2 — Lightweight & native.** C++20 core on Qt 6, custom Qt Quick components, small
  dependency footprint, fast on large directories.
- **G3 — Dolphin-class features.** Multiple view modes with customizable columns, tabs,
  split view, breadcrumbs, places sidebar, previews, robust file operations, search,
  and user-defined context actions.
- **G4 — Predictable, self-owned theming.** Fang defines its own palette. Dark mode
  always sticks. System light/dark following is strictly opt-in.
- **G5 — Distro-agnostic, Wayland-first.** Runs anywhere Qt 6 runs; developed and tested
  under Wayland (Hyprland) and expected to also work on X11.

## Non-goals (for v1)

- **N1** — Native remote protocols (SFTP/SMB/MTP/archives-as-folders). v1 only handles
  local filesystems and paths already mounted by gvfs/udisks (they appear as local
  paths). Native VFS is a later phase.
- **N2** — A general C++/binary plugin ABI. Extensibility in v1 is via (a) replacing the
  QML UI and (b) TOML-defined custom actions. No dynamically loaded native plugins.
- **N3** — A background daemon / single-instance coordination. Fang is multi-process:
  each launch is an independent process.
- **N4** — Windows/macOS support. Linux only.
- **N5** — Video/PDF/audio-cover thumbnails in v1 (image thumbnails only; the thumbnail
  subsystem is designed to accept more generators later).
- **N6** — Cloud storage integrations, tagging/semantic indexing (Baloo-style), and
  version-control overlays. Deferred.

## Target users

- Power users on tiling Wayland compositors (Hyprland, Sway, river) who want a file
  manager that matches their setup and theme.
- Developers who want to reskin or rebuild the UI without forking a large C++ codebase.
- Anyone wanting a fast Dolphin alternative without pulling in Plasma.

## Glossary

- **Core** — the C++ backend: models, controllers, and services with no visual code.
- **View layer / UI** — the QML that renders the interface and calls into the Core.
- **QML API** — the versioned, documented surface (singletons, models, roles, methods,
  signals) the Core exposes to QML. See [04-qml-api.md](04-qml-api.md).
- **Override project** — a user-supplied QML project under `~/.config/fang/ui/` that
  replaces the bundled default UI.
- **Pane** — one navigable file view with its own location and history. A window may show
  one pane or two side-by-side (split view). Each pane may host multiple tabs.
- **Place** — a sidebar entry (home, XDG user dirs, bookmarks, devices, trash, network).
- **Custom action** — a user-declared context-menu command (TOML) run against selected
  files.
- **Thumbnail cache** — the shared freedesktop cache at `~/.cache/thumbnails/`.

## Success criteria for v1

1. Browse local filesystem in icon, list (detail), and tree views with customizable,
   reorderable, sortable columns.
2. Tabs + split view + breadcrumb navigation + back/forward/up history.
3. Places sidebar with XDG dirs, bookmarks, mounted devices, and trash.
4. Copy/move/delete/trash with progress, pause/cancel, conflict resolution, and
   multi-level undo/redo.
5. Image thumbnails via the shared freedesktop cache; text/code peek in a preview pane.
6. Native search + instant type-to-filter.
7. TOML custom actions in the context menu; open-with via xdg/gio.
8. Self-owned dark theme that persists across sessions and desktops.
9. The entire UI can be replaced by a QML project in `~/.config/fang/ui/`.
10. Core unit tests pass; a QML smoke test loads the default UI headlessly.
