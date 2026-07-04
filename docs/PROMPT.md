# Master Generation Prompt (for Fable 5)

Copy everything in the fenced block below into Fable 5 to generate Fang. It assumes the
`docs/` specification files in this repository are available to the model; if the model
cannot read files, paste `docs/00`–`docs/10` ahead of this prompt. Every design decision
is already made in those docs — **do not invent or deviate.**

---

```
You are implementing "Fang", a lightweight, fully customizable Wayland-first file manager
for Linux. The COMPLETE specification is in docs/00-overview.md through docs/10-repo-layout.md
in this repository. Read them fully before writing code. They are authoritative: every
architectural decision is already made. Do NOT deviate, substitute technologies, or leave
TODOs where the spec is concrete. Where the spec says "fixed", it is non-negotiable.

LOCKED DECISIONS (see docs for full detail):
- Core: C++20, namespace fang. GUI: Qt 6 (6.6+, target 6.8 LTS), Qt Quick/QML.
- UI is 100% QML built from CUSTOM Qt Quick components (no styled QQC2 widgets). It calls
  into C++ controllers ONLY; QML never performs filesystem I/O.
- Multi-process (no daemon, no single-instance, no D-Bus activation).
- The entire view layer is overridable: on startup, load ~/.config/fang/ui/main.qml if it
  exists (add that dir to the QML import path), else the bundled qrc:/Fang/ui/Main.qml.
  A broken override must log the error and fall back to the bundled UI — never brick.
- Stable, versioned QML API in module "Fang" exactly as specified in docs/04-qml-api.md
  (Application.apiVersion == 1). Additive-only within the version.
- Self-owned theming: bundled dark theme applied before first paint and it STAYS; Fang
  does not inherit the system color scheme unless appearance.follow_system=true (opt-in
  freedesktop appearance portal). All colors/metrics come from Application.theme tokens.
- File ops: threaded FileOperationEngine with progress/pause/cancel, conflict dialog
  (overwrite/skip/keep-both/rename/merge + apply-to-all), freedesktop Trash spec, and
  multi-level Undo/Redo. Never block the GUI thread; never crash on FS errors.
- Search: native threaded recursive walk + separate instant type-to-filter.
- Previews: Qt image/SVG thumbnails read/written to the shared freedesktop cache
  (~/.cache/thumbnails); text/code peek. No video/PDF in v1 but keep generators pluggable.
- Remote FS: local + already-mounted gvfs paths only in v1.
- Custom actions: TOML (actions.toml) with %f/%F/%u/%U/%d/%n/%% placeholders, SHELL-QUOTED
  safely. App launching via xdg/gio + .desktop. Single-click open by default (configurable).
- Config: TOML under XDG paths (docs/06). Build: CMake only (docs/08). License: GPL-3.0.
- Dependencies: Qt6 + toml++ ONLY. No KDE Frameworks, GTK, Python, Node. udisks2/gio/xdg
  are runtime-optional and must degrade gracefully.

DELIVERABLES:
1. The full repository exactly per docs/10-repo-layout.md: CMake build (fangcore static lib
   + fang executable via qt_add_qml_module), all C++ classes in docs/03 with the QML API
   surface in docs/04, the complete default QML UI in docs/05, bundled dark+light themes
   and default actions.toml per docs/06, desktop/metainfo files, and the tests in docs/09.
2. Every C++ QML type registered (QML_ELEMENT/QML_SINGLETON/Q_ENUM) with the EXACT
   property/method/signal/role/enum names listed in docs/04. Roles in docs/03 §4 are the
   canonical names — match them character-for-character.
3. A working app: build with `cmake -B build -G Ninja && cmake --build build`, run
   `./build/src/fang ~`, browse/copy/move/trash/undo/search, switch view modes, use
   split view and tabs, see image thumbnails, and replace the UI via ~/.config/fang/ui/.
4. Passing tests: `ctest --test-dir build --output-on-failure` (suites in docs/09).

ENGINEERING RULES:
- GUI thread never blocks: directory scans, file ops, search, and thumbnailing run on
  workers; results delivered via queued signals. Debounce watcher events (~100ms).
- All user-facing failures become signals -> UI notifications; the app continues.
- Placeholder/command expansion for custom actions MUST be shell-safe (see util/ShellQuote
  and tst_actions) — treat this as security-critical.
- Use QLoggingCategory per subsystem; quiet by default.
- Provide .clang-format (LLVM base, 4-space, 100 col) and .editorconfig; core targets build
  with -Werror.
- Keep QML components small/single-purpose so overrides can reuse them.

PROCESS:
- Implement in this order so it stays buildable at each step: (1) CMake skeleton + main.cpp
  + Application singleton + minimal Main.qml window; (2) DirectoryModel + PaneController +
  Navigation + a working DetailsView listing a directory; (3) Selection + view modes +
  sidebar/Places + toolbar/breadcrumbs + tabs + split; (4) FileOperationEngine + Trash +
  Undo + Clipboard + conflict dialog + progress overlay; (5) ThumbnailProvider + preview
  pane; (6) Search + filter; (7) ConfigStore/Settings/Theme + theming + shortcuts; (8)
  ActionRegistry + MimeResolver + open-with + properties; (9) override loading + fallback;
  (10) tests. Build and keep it runnable after each stage.
- If any ambiguity arises, RE-READ the relevant doc; do not ask, do not guess against the
  spec. The spec was written to eliminate open decisions.

Begin by reading all docs, then produce the CMake skeleton and stage 1, then continue
through stage 10.
```

---

## Tips for driving the generation

- Generate in the staged order above; verify a build after each stage rather than
  requesting the whole app in one shot. Qt/QML apps fail fast at the QML-registration
  boundary, so early stages catch API-name mismatches.
- If the model omits a role, enum value, or method name, point it back to
  [04-qml-api.md](04-qml-api.md) / [03-core-backend.md](03-core-backend.md) — those names
  are the contract.
- The two highest-risk areas to review closely: (1) **shell-safe placeholder expansion**
  in custom actions, and (2) **thread-safety** of the file-op/search/thumbnail workers vs.
  the GUI-thread models. The tests in [09-testing.md](09-testing.md) target both.
- After a working v1, extend strictly additively so existing `~/.config/fang/ui/`
  overrides keep working (bump `apiVersion` only on true breaks).
