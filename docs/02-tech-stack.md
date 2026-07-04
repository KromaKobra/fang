# 02 — Tech Stack & Standards

## Languages & standards

- **C++20** for all core/controller/service code. Use modern idioms: `std::filesystem`
  where it beats Qt, RAII, `enum class`, `std::optional`, structured bindings, ranges
  where readable. Prefer Qt containers/strings at the QML boundary for zero-copy binding.
- **QML (Qt Quick)** for the entire view layer. JavaScript only for trivial view glue;
  no business logic in QML/JS.
- **TOML** for all user-facing configuration files.
- **CMake** (>= 3.24) as the only build system. Ninja is the recommended generator.

## Qt

- **Qt 6.6 or newer** (develop/test against **Qt 6.8 LTS**). Do not use APIs removed in
  Qt 6, and do not require Qt 6.9+ features.
- Qt modules used:
  - `Qt6::Core`, `Qt6::Gui`, `Qt6::Qml`, `Qt6::Quick`, `Qt6::QuickControls2`
    (QuickControls2 linked only for `ApplicationWindow`/platform integration; the UI uses
    **custom components**, not styled QQC2 widgets — see [05-default-ui.md](05-default-ui.md)).
  - `Qt6::Concurrent` (workers), `Qt6::DBus` (udisks2, appearance portal — optional at
    runtime), `Qt6::Test` (tests).
- Use `qt_add_qml_module` to register the default UI and the C++ QML types into a proper
  QML module (`Fang` URI). Bundle default QML via the Qt Resource System (`qrc`).
- Use `qt_standard_project_setup()` and `CMAKE_AUTOMOC`.

## Dependencies

**Hard dependencies (required to build/run):**

- Qt 6 (modules above).
- A TOML library. **Use `toml++` (tomlplusplus)**, vendored under `third_party/` or found
  via CMake `find_package`/`FetchContent`. Header-only, C++17+, no runtime dep.

**Soft / runtime-optional (feature-detected, degrade gracefully if absent):**

- `udisks2` over D-Bus — mount/unmount removable devices. If unavailable, devices are
  still listed read-only via `QStorageInfo`; mount/unmount actions are hidden.
- `gio`/gvfs — surfaces already-mounted network locations under `/run/user/$UID/gvfs`.
  If absent, the network section is empty.
- `xdg-mime` / `gio` — default-app resolution and "open with". Fall back to
  `QMimeDatabase` + parsing `.desktop` files in XDG data dirs if the CLIs are missing.
- freedesktop **appearance portal** (`org.freedesktop.portal.Settings`) over D-Bus — only
  read when the user opts into "follow system light/dark".

**Explicitly NOT dependencies:** any KDE Frameworks (KIO, KConfig, KService, Baloo,
Solid), Plasma, GTK, Python, Node. Fang must build and run on a minimal Qt6 system.

## Coding standards

- **File I/O only in the service layer.** Controllers orchestrate; QML never does I/O.
- **No blocking on the GUI thread.** Any op that can exceed ~16 ms goes to a worker.
- **Error handling:** services return typed results/`std::optional`/error enums; user-
  facing failures surface as signals the UI turns into notifications. Never crash on a
  filesystem error (permission denied, race, disappearing file) — report and continue.
- **Naming:** classes `PascalCase`, methods/vars `camelCase`, QML files `PascalCase.qml`,
  C++ files `snake_case.cpp/.h` OR `PascalCase.cpp/.h` — pick one and be consistent
  across the repo (**use `PascalCase.cpp/.h` matching the class name**).
- **Namespaces:** all core code in `namespace fang { ... }`.
- **QML type registration:** register controllers/enums with `QML_ELEMENT`/
  `QML_SINGLETON`/`Q_ENUM` so the UI (and overrides) get typed access and autocompletion.
- **No global mutable singletons** beyond the intentional QML singletons; pass
  dependencies explicitly.
- **Logging:** use `QLoggingCategory` per subsystem (`fang.fs`, `fang.ops`, `fang.ui`,
  `fang.search`, `fang.thumbs`). Quiet by default; enable via `QT_LOGGING_RULES`.
- **Format:** provide a `.clang-format` (LLVM-based, 4-space indent, 100 col). Provide a
  `.editorconfig`.

## Platform assumptions

- Linux only. POSIX filesystem semantics. Wayland-first (Qt `wayland` platform plugin),
  X11 supported via Qt with no Fang-specific code.
- XDG Base Directory spec for all paths (see [06-config-theming.md](06-config-theming.md)).
- UTF-8 filesystem paths; handle non-UTF-8 byte paths defensively (display with
  replacement, operate on raw bytes).

## Versioning

- Semantic versioning for the app (`0.1.0` for the first v1 milestone).
- The **QML API has its own integer version** exposed as `Application.apiVersion`
  (starts at `1`). Overrides can check it. Breaking API changes bump it. See
  [04-qml-api.md](04-qml-api.md#versioning--compatibility).
