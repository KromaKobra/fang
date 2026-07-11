<h1 align=center>Fang</h1>

<div align=center>

![GitHub last commit](https://img.shields.io/github/last-commit/KromaKobra/fang?style=for-the-badge&labelColor=101418&color=9ccbfb)
![GitHub Repo stars](https://img.shields.io/github/stars/KromaKobra/fang?style=for-the-badge&labelColor=101418&color=b9c8da)
![GitHub repo size](https://img.shields.io/github/repo-size/KromaKobra/fang?style=for-the-badge&labelColor=101418&color=d3bfe6)
[![Ko-Fi donate](https://img.shields.io/badge/donate-kofi?style=for-the-badge&logo=ko-fi&logoColor=ffffff&label=ko-fi&labelColor=101418&color=f16061&link=https%3A%2F%2Fko-fi.com%2Fkromakobra)](https://ko-fi.com/kromakobra)
[![Discord invite](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fdiscordapp.com%2Fapi%2Finvites%2FsGgZGGggmE%3Fwith_counts%3Dtrue&query=approximate_member_count&style=for-the-badge&logo=discord&logoColor=ffffff&label=discord&labelColor=101418&color=96f1f1&link=https%3A%2F%2Fdiscord.gg%2sGgZGGggmE)](https://discord.gg/sGgZGGggmE)

</div>

<div align=center>
<b>A lightweight, fully customizable file manager for Linux</b>
</div>

https://github.com/user-attachments/assets/839d1be8-1268-4af9-b791-e4a6fd0656bf

## What Fang is

Fang is a fast, native file manager whose **entire graphical interface is QML** running
on top of a stable, versioned C++ core. The C++ core owns all the "hard" work
(filesystem models, threaded file operations, trash, undo, search, thumbnails) and
exposes it to QML through a documented API. The default UI is bundled with the app, but
**any developer can replace the whole interface** by dropping a QML project into
`~/.config/fang/ui/`. See **[docs/RICING.md](docs/RICING.md)** for the full
customization guide (UI overrides, themes, custom icons, actions).

Fang aims for feature parity with KDE's Dolphin over time, but without the Plasma
dependency chain — and with a theming model that **does not fight the desktop** (Fang
owns its own colors, so dark mode always sticks).

## Design pillars

1. **Lightweight.** C++/Qt6 core, custom Qt Quick components, minimal dependencies
   (Qt 6 + toml++, nothing else).
2. **Fully customizable.** The view layer is data-driven QML over a stable API and can be
   wholesale replaced by the user.
3. **Wayland-first.** Designed and tested on Wayland (Hyprland), works on X11 via Qt.
4. **Predictable theming.** Fang's appearance is self-owned; it never silently inherits a
   system color scheme unless you opt in.
5. **Distro-agnostic.** No KDE/Plasma runtime requirement. Built with plain CMake + Qt6.

## Building

```sh
cmake -B build -G Ninja
cmake --build build
./build/src/fang ~              # open your home directory
ctest --test-dir build          # run the test suite
```

On NixOS use the bundled flake dev shell:

```sh
nix develop -c bash -c 'cmake -B build -G Ninja && cmake --build build'
```

See [docs/08-build-packaging.md](docs/08-build-packaging.md) for full details.

## Customizing (ricing)

Everything lives under `~/.config/fang/`:

| Path | What it does |
|------|--------------|
| `config.toml` | Settings — view defaults, behavior (click-to-open, type-ahead), shortcuts |
| `themes/<name>.toml` | Your color/metric/font themes (select via `appearance.theme`) |
| `icons/<name>.svg` | Override any file/folder icon by freedesktop icon name |
| `actions.toml` | Custom context-menu actions with safe `%f/%F/%d/…` placeholders |
| `ui/main.qml` | **Replace the entire UI** — Fang loads your QML instead of the bundled one |

The complete guide, including the QML API you build against, is in
**[docs/RICING.md](docs/RICING.md)** and [docs/04-qml-api.md](docs/04-qml-api.md).

## Documentation map

| Doc | Contents |
|-----|----------|
| [docs/00-overview.md](docs/00-overview.md) | Vision, goals, non-goals, glossary |
| [docs/01-architecture.md](docs/01-architecture.md) | System architecture, modules, threading, process model |
| [docs/02-tech-stack.md](docs/02-tech-stack.md) | Languages, Qt version, dependencies, coding standards |
| [docs/03-core-backend.md](docs/03-core-backend.md) | C++ core: classes, models, file-op engine, trash, undo, search, thumbnails, config, mounts, actions |
| [docs/04-qml-api.md](docs/04-qml-api.md) | The stable QML API contract + override & versioning mechanism |
| [docs/05-default-ui.md](docs/05-default-ui.md) | Default QML UI spec: windows, panels, views, interactions, shortcuts |
| [docs/06-config-theming.md](docs/06-config-theming.md) | `config.toml`, `theme`, `actions.toml`, `bookmarks` schemas |
| [docs/RICING.md](docs/RICING.md) | **Customization guide** — UI overrides, themes, icons, actions |
| [docs/07-features-roadmap.md](docs/07-features-roadmap.md) | v1 feature list + phased roadmap toward Dolphin parity |
| [docs/08-build-packaging.md](docs/08-build-packaging.md) | CMake layout, build/run instructions, optional Nix flake |
| [docs/09-testing.md](docs/09-testing.md) | Test plan and manual QA checklist |
| [docs/10-repo-layout.md](docs/10-repo-layout.md) | Full directory tree of the codebase |
