# 08 — Build & Packaging

## Requirements

- CMake >= 3.24, Ninja (recommended).
- A C++20 compiler (GCC 12+ or Clang 15+).
- Qt 6.6+ (tested against 6.8 LTS) with modules: Core, Gui, Qml, Quick, QuickControls2,
  Concurrent, DBus, Test.
- `toml++` (fetched via CMake `FetchContent` if not found on the system).
- Optional at runtime only: `udisks2`, `gio`/gvfs, `xdg-utils`.

## CMake structure

Top-level `CMakeLists.txt`:
- `cmake_minimum_required(VERSION 3.24)`; `project(Fang VERSION 0.1.0 LANGUAGES CXX)`.
- `set(CMAKE_CXX_STANDARD 20)`, `CMAKE_CXX_STANDARD_REQUIRED ON`.
- `find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Concurrent DBus)`.
- `qt_standard_project_setup(REQUIRES 6.6)`.
- `FetchContent` for `tomlplusplus` (or `find_package(tomlplusplus)` first).
- `add_subdirectory(src)`, and `enable_testing(); add_subdirectory(tests)`.

`src/CMakeLists.txt`:
- `qt_add_executable(fang main.cpp ...core sources...)`.
- `qt_add_qml_module(fang URI Fang VERSION 1.0 QML_FILES <all ui/*.qml> RESOURCES <themes, icons>)`.
  Registers C++ QML types (controllers/models/enums/singletons) and bundles the default
  UI + bundled themes/default actions into the `Fang` module resources.
- `target_link_libraries(fang PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Concurrent Qt6::DBus tomlplusplus::tomlplusplus)`.
- Install rules: binary to `${CMAKE_INSTALL_BINDIR}`, `dev.kromakobra.Fang.desktop` to
  `applications/`, an app icon to `icons/hicolor/scalable/apps/`, and a `metainfo` XML.

`tests/CMakeLists.txt`:
- One `qt_add_executable` + `add_test` per core test suite using `Qt6::Test`; a QML smoke
  test using `qmltestrunner`/`QQuickView` in offscreen mode.

Split core into a static lib `fangcore` (all non-`main` C++) so tests link it without the
GUI entry point. Layout in [10-repo-layout.md](10-repo-layout.md).

## Build & run

```sh
# Configure + build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run
./build/src/fang ~/Documents

# Run tests
ctest --test-dir build --output-on-failure

# Install
cmake --install build --prefix ~/.local
```

Debug logging: `QT_LOGGING_RULES="fang.*=true" ./build/src/fang`.
Force Wayland: `QT_QPA_PLATFORM=wayland ./build/src/fang`.

## Desktop integration files

- `dev.kromakobra.Fang.desktop` — `Categories=System;FileManager;Utility;`,
  `MimeType=inode/directory;`, `Exec=fang %U`, `Terminal=false`.
- `dev.kromakobra.Fang.metainfo.xml` — AppStream metadata.
- App icon (scalable SVG).

## NixOS (optional, documented for the author's environment)

CMake is the source of truth. A thin `flake.nix` is **optional** and, if added, should:
- Provide a `devShell` with Qt6 (qtbase, qtdeclarative, qtquickcontrols2), cmake, ninja,
  gcc, and `pkg-config`.
- Provide a `packages.default` = `stdenv.mkDerivation` wrapping the CMake build, with
  `qt6.wrapQtAppsHook` so the QML runtime and platform plugins resolve at runtime.
- Include `qtwayland` in `buildInputs` so the Wayland platform plugin is available.

Example dev shell (reference only; not required to ship):

```nix
{
  description = "Fang file manager";
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  outputs = { self, nixpkgs }:
    let pkgs = nixpkgs.legacyPackages.x86_64-linux; in {
      devShells.x86_64-linux.default = pkgs.mkShell {
        packages = with pkgs; [
          cmake ninja gcc pkg-config
          qt6.qtbase qt6.qtdeclarative qt6.qtwayland
          tomlplusplus
        ];
      };
    };
}
```

Since Fang is multi-process and depends only on Qt6 + toml++, packaging for other distros
is a standard CMake/Qt6 recipe. Flatpak/AppImage are possible later but are **not** part
of v1 deliverables (only CMake was chosen for v1).

## CI (recommended, minimal)

- A single Linux job: install Qt6, configure, build, run `ctest`. Fail on warnings in
  Fang's own targets (`-Werror` for `fangcore`, not for third_party).
