# 09 — Testing Plan

Scope: **Qt Test unit tests for the non-GUI core + a headless QML smoke test.** GUI
interaction tests are out of scope for v1 (see roadmap). Tests link the `fangcore` static
library so they run without the GUI entry point. Filesystem tests operate inside a
`QTemporaryDir`; never touch the real home directory or system paths.

## Core unit test suites (Qt Test)

**`tst_fileops`** — FileOperationEngine + workers
- Copy file/dir (recursive) preserves content, timestamps, permissions.
- Move same-filesystem (rename fast-path) and cross-filesystem (copy+delete) — simulate
  cross-fs with two temp dirs where possible; otherwise unit-test the code path selection.
- Progress signals: bytes/files counts monotonic; reaches 100%.
- Conflict: overwrite / skip / keep-both / rename / merge(dir); `applyToAll` caches choice.
- Cancel mid-operation leaves no partial destination for the current file (or cleans it).
- Per-file error (permission denied) is collected and reported without aborting the batch
  (default policy).

**`tst_trash`** — TrashManager
- Trash writes `files/` entry + valid `.trashinfo` (Path URL-encoded, ISO DeletionDate).
- List returns original path + date; restore returns file to origin; conflict on restore
  handled.
- Empty trash removes files + info; name collisions in `files/` are suffixed.
- Corrupt/missing `.trashinfo` surfaced as "unknown origin", not a crash.

**`tst_undo`** — UndoStack
- Undo of move/rename/trash/create restores prior state exactly.
- Undo of copy deletes only copy-created files, never pre-existing ones.
- Redo re-applies; bounded stack drops oldest beyond limit; undo/redo text correct.

**`tst_search`** — SearchEngine
- Name glob + substring matches; case sensitivity; regex mode.
- Content search finds text, skips binaries (null-byte heuristic) and oversized files.
- Filters: size/date/mime narrow correctly; hidden inclusion toggle.
- Cancel stops promptly; results stream (model row count grows).
- Pseudo-fs ignore list respected.

**`tst_config`** — ConfigStore/Settings
- Load defaults when file absent; round-trip preserves unknown keys.
- Typed getters/setters read/write expected values; atomic write (temp+rename).
- `schema_version` present; malformed TOML → defaults + error signal, no crash.

**`tst_actions`** — ActionParser/ActionRegistry
- Parse valid `actions.toml`; user `id` overrides bundled; `show_in_menu=false` disables.
- `actionsFor(selection)` filters by count/mime/extension/appliesTo.
- Placeholder expansion `%f/%F/%u/%U/%d/%n/%%` with correct **shell quoting** (paths with
  spaces, quotes, `$`, backticks must be safely escaped) — security-critical.

**`tst_mime`** — MimeResolver
- Content+name detection; human comment lookup; default-app resolution path (mockable);
  `.desktop` Exec field-code expansion.

**`tst_model`** — DirectoryModel
- Populates a temp dir; correct roles; hidden filtering; live `filterText`; natural sort;
  folders-first; reacts to watcher add/remove/change (with debounce).
- Tree lazy `fetchMore`/`canFetchMore`.

**`tst_thumbnails`** — ThumbnailProvider
- Correct freedesktop cache path (md5 of file URI) + size dirs; writes valid PNG with
  `Thumb::URI`/`Thumb::MTime`; staleness detection regenerates; EXIF orientation applied.
- Non-image / oversized returns empty (icon fallback).

## QML smoke test

**`tst_qml_smoke`** — load the default UI headlessly
- `QT_QPA_PLATFORM=offscreen`; register `Fang` module; load `Main.qml`.
- Assert: no QML errors/warnings; a window is created; a `WindowController` with ≥1 tab
  and a `PaneController` whose `model` populates a seeded `QTemporaryDir`.
- Drive minimal flow via exposed objects: `pane.open(tempdir)`, wait for `loading=false`,
  assert `itemCount` matches seeded files; `pane.activate(row)` on a subfolder navigates.
- Load a deliberately broken override to assert the safe-fallback path engages.

## Manual QA checklist (for release, in `docs`/PR description)

- Dark theme applied before first paint; persists across restarts and on a GTK/Plasma
  desktop; toggling `follow_system` swaps with the portal.
- DnD copy/move within pane, between split panes, to sidebar places, and to/from another
  Wayland app.
- Trash/restore/empty round-trip; conflict dialog paths; undo/redo chains.
- Large directory (50k+ files) stays responsive; search cancellable; scroll smooth.
- Override project in `~/.config/fang/ui/` replaces the UI; a broken one falls back.
- Removable USB mount/unmount/eject; capacity bars correct.

## Determinism & CI

- All FS tests confined to `QTemporaryDir`; no network; no reliance on system mime/app
  DBs beyond what can be mocked or is guaranteed by Qt.
- `ctest --output-on-failure` is the single gate. Core targets build with `-Werror`.
