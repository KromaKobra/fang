# Fang

**A lightweight, fully customizable file manager for Linux, built on Qt 6 + QML.**

- **By:** Kroma Kobra
- **Type:** Desktop file manager (Wayland-first)
- **License:** GPL-3.0-or-later
- **Status:** Planning / pre-implementation. This repository currently contains the
  complete design specification. No application code has been generated yet.

---

## What Fang is

Fang is a fast, native file manager whose **entire graphical interface is QML** running
on top of a stable, versioned C++ core. The C++ core owns all the "hard" work
(filesystem models, threaded file operations, trash, undo, search, thumbnails) and
exposes it to QML through a documented API. The default UI is bundled with the app, but
**any developer can replace the whole interface** by dropping a QML project into
`~/.config/fang/`.

Fang aims for feature parity with KDE's Dolphin over time, but without the Plasma
dependency chain — and with a theming model that **does not fight the desktop** (Fang
owns its own colors, so dark mode always sticks).

## Design pillars

1. **Lightweight.** C++/Qt6 core, custom Qt Quick components, minimal dependencies.
2. **Fully customizable.** The view layer is data-driven QML over a stable API and can be
   wholesale replaced by the user.
3. **Wayland-first.** Designed and tested on Wayland (Hyprland), works on X11 via Qt.
4. **Predictable theming.** Fang's appearance is self-owned; it never silently inherits a
   system color scheme unless you opt in.
5. **Distro-agnostic.** No KDE/Plasma runtime requirement. Built with plain CMake + Qt6.

## Locked technical decisions

| Area | Decision |
|------|----------|
| Core language | C++20 |
| GUI toolkit | Qt 6 (Qt Quick / QML), custom components on bare Qt Quick |
| Process model | Multi-process (one process per launch) |
| GUI override | Full view layer over a stable, versioned QML API; overridable from `~/.config/fang/` |
| Theming | Self-owned theme (bundled dark default), opt-in follow of system via freedesktop appearance portal |
| File ops | Threaded, freedesktop Trash spec, conflict-resolution dialog, multi-level undo/redo |
| Search | Native threaded recursive walk + instant type-to-filter |
| Previews | Qt image thumbnails, read/write shared freedesktop thumbnail cache; text/code peek |
| Remote FS (v1) | Local filesystems + already-mounted gvfs paths only |
| Custom actions | TOML-defined context-menu actions with placeholders |
| App launching | freedesktop `.desktop` / MIME associations (xdg/gio); single-click open |
| Config format | TOML |
| Build | CMake |
| Tests | Qt Test unit tests for core + QML smoke test |

## Documentation map

Read the docs in order. Everything the implementation needs is specified there.

| Doc | Contents |
|-----|----------|
| [docs/00-overview.md](docs/00-overview.md) | Vision, goals, non-goals, glossary |
| [docs/01-architecture.md](docs/01-architecture.md) | System architecture, modules, threading, process model |
| [docs/02-tech-stack.md](docs/02-tech-stack.md) | Languages, Qt version, dependencies, coding standards |
| [docs/03-core-backend.md](docs/03-core-backend.md) | C++ core: classes, models, file-op engine, trash, undo, search, thumbnails, config, mounts, actions |
| [docs/04-qml-api.md](docs/04-qml-api.md) | The stable QML API contract + override & versioning mechanism |
| [docs/05-default-ui.md](docs/05-default-ui.md) | Default QML UI spec: windows, panels, views, interactions, shortcuts |
| [docs/06-config-theming.md](docs/06-config-theming.md) | `config.toml`, `theme`, `actions.toml`, `bookmarks` schemas |
| [docs/07-features-roadmap.md](docs/07-features-roadmap.md) | v1 feature list + phased roadmap toward Dolphin parity |
| [docs/08-build-packaging.md](docs/08-build-packaging.md) | CMake layout, build/run instructions, optional Nix flake |
| [docs/09-testing.md](docs/09-testing.md) | Test plan and manual QA checklist |
| [docs/10-repo-layout.md](docs/10-repo-layout.md) | Full directory tree of the target codebase |
| [docs/PROMPT.md](docs/PROMPT.md) | **Master generation prompt** for Fable 5 (ties all docs together) |

## Building (once generated)

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/fang            # or: cmake --install build
```

See [docs/08-build-packaging.md](docs/08-build-packaging.md) for full details and
NixOS instructions.
