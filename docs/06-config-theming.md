# 06 — Config, Theming & Actions

All user files use **TOML** and follow the XDG Base Directory spec. Files are created with
sensible defaults on first run; unknown keys are preserved on rewrite; every file carries
a `schema_version` for migration.

## File locations (XDG)

| Path | Purpose |
|------|---------|
| `$XDG_CONFIG_HOME/fang/config.toml` | Main settings |
| `$XDG_CONFIG_HOME/fang/actions.toml` | Custom context-menu actions |
| `$XDG_CONFIG_HOME/fang/bookmarks.toml` | Sidebar bookmarks (order preserved) |
| `$XDG_CONFIG_HOME/fang/themes/<name>.toml` | User themes |
| `$XDG_CONFIG_HOME/fang/ui/` | **UI override QML project** (`main.qml` = root) |
| `$XDG_STATE_HOME/fang/session.toml` | Session restore (tabs/panes/locations) |
| `$XDG_CACHE_HOME/thumbnails/` | Shared freedesktop thumbnail cache (read+write) |
| `$XDG_DATA_HOME/fang/` | Reserved for future data (templates, indexes) |

`$XDG_CONFIG_HOME` defaults to `~/.config`, `$XDG_STATE_HOME` to `~/.local/state`,
`$XDG_CACHE_HOME` to `~/.cache`, `$XDG_DATA_HOME` to `~/.local/share`.

Bundled defaults (dark theme, default actions) ship inside the binary (`qrc`) and are used
when the corresponding user file is absent.

---

## `config.toml` — full schema with defaults

```toml
schema_version = 1

[general]
default_location   = "~"          # where new windows/tabs open
restore_session    = true         # reopen last tabs/panes on launch
confirm_delete     = true         # confirm permanent delete
confirm_trash      = false        # confirm move-to-trash
single_instance    = false        # FIXED false in v1 (multi-process); reserved
terminal_command   = "auto"       # "auto" detects; or e.g. "kitty", "foot -e"

[view]
default_mode       = "details"    # icons | list | details | tree
show_hidden        = false
folders_first      = true
sort_role          = "name"       # name|size|type|modified|created|accessed|extension|owner
sort_descending    = false
case_sensitive_sort = false
icon_size          = 48           # px, icons view
columns            = ["name", "size", "type", "modified"]  # details/tree
column_widths      = { name = 320, size = 90, type = 160, modified = 160 }
preview_pane       = false        # open on start
preview_pane_width = 280

[behavior]
open_with          = "single"     # single | double  (click to open)
select_with        = "double"     # derived: opposite of open_with when single-open
type_ahead         = "filter"     # filter | jump
drag_default       = "auto"       # auto | copy | move (auto = move same-fs, copy cross-fs)
new_tab_position   = "after"      # after | end
close_confirm_multiple_tabs = true

[previews]
enabled            = true
images             = true         # png/jpg/webp/svg/gif(first frame)/bmp/tiff/...
max_file_size_mb   = 64           # skip thumbnailing above this
text_peek_kb       = 128          # bytes shown in text preview
use_shared_cache   = true         # read/write ~/.cache/thumbnails

[search]
content_search_default = false
follow_symlinks    = false
max_depth          = 0            # 0 = unlimited
ignore_hidden      = true
ignore_paths       = ["/proc", "/sys", "/dev", "/run"]

[appearance]
theme              = "dark"       # bundled: "dark","light"; or a user theme name
follow_system      = false        # opt-in: follow freedesktop portal light/dark
light_theme        = "light"      # used when follow_system flips to light
dark_theme         = "dark"       # used when follow_system flips to dark
icon_theme         = "auto"       # "auto" = system default; or a theme name
font_family        = "auto"
font_size          = 10

[shortcuts]
# Any entry here overrides the built-in default (see 05-default-ui.md table).
# Example: new_tab = "Ctrl+T"
```

The `Settings` controller exposes each group as typed properties; changing a property
writes back atomically and emits a change signal so the UI updates live.

---

## Default theme

Bundled `dark` theme (`qrc:/Fang/themes/dark.toml`). A theme TOML maps directly onto the
`Theme` properties in [04-qml-api.md §Theme](04-qml-api.md#theme).

```toml
schema_version = 1
name = "dark"
is_dark = true

[colors]
background           = "#1e1e2e"
background_alt       = "#181825"
surface              = "#252537"
surface_hover        = "#313145"
surface_active       = "#3a3a52"
foreground           = "#cdd6f4"
foreground_muted     = "#9399b2"
accent               = "#89b4fa"
accent_foreground    = "#11111b"
border               = "#313244"
selection_background  = "#45475a"
selection_foreground  = "#cdd6f4"
error                = "#f38ba8"
warning              = "#f9e2af"
success              = "#a6e3a1"
shadow               = "#00000066"
scrollbar            = "#45475a"
scrollbar_hover      = "#585b70"

[metrics]
radius_small = 4
radius_medium = 8
radius_large = 12
spacing_small = 4
spacing_medium = 8
spacing_large = 16
row_height = 28
toolbar_height = 40
sidebar_width = 220
icon_size_small = 16
icon_size_medium = 24
icon_size_large = 48

[typography]
font_family       = "Inter, sans-serif"
font_family_mono  = "JetBrains Mono, monospace"
font_size_small   = 9
font_size_normal  = 10
font_size_large   = 13
```

A bundled `light` theme mirrors this with light values. Users add themes under
`~/.config/fang/themes/<name>.toml` and select via `appearance.theme`. When
`follow_system = true`, `Theme` subscribes to `org.freedesktop.portal.Settings`
(`color-scheme`) and swaps between `light_theme`/`dark_theme`. When `false`, the chosen
theme is fixed regardless of desktop — **this is what makes dark mode always stick.**

---

## `actions.toml` — custom actions

```toml
schema_version = 1

[[action]]
id          = "open-terminal-here"
label       = "Open Terminal Here"
icon        = "utilities-terminal"
command     = "kitty --directory %d"   # %d = current directory
applies_to  = "background"             # background | files | both
show_in_menu = true

[[action]]
id          = "compress-zip"
label       = "Compress to ZIP"
icon        = "package-x-generic"
command     = "sh -c 'cd %d && zip -r archive.zip %F'"
applies_to  = "files"
min_selection = 1

[[action]]
id          = "open-vscode"
label       = "Open with VS Code"
icon        = "com.visualstudio.code"
command     = "code %F"
applies_to  = "both"
mime_types  = ["inode/directory", "text/*"]

[[action]]
id          = "extract-here"
label       = "Extract Here"
icon        = "archive-extract"
command     = "sh -c 'cd %d && file-roller -h %f || tar xf %f'"
applies_to  = "files"
extensions  = ["zip", "tar", "gz", "xz", "7z", "rar"]
max_selection = 1
```

**Fields:** `id` (unique), `label`, `icon` (theme icon name), `command` (template),
`applies_to` (`files`/`background`/`both`), optional `mime_types` (glob like `text/*`),
`extensions`, `min_selection`, `max_selection`, `show_in_menu`, `run_in_terminal` (bool),
`submenu` (group label).

**Placeholders** (shell-quoted by Fang; never inject raw): `%f` single file path,
`%F` all selected paths, `%u`/`%U` URLs, `%d` current directory, `%n` basename, `%%`
literal `%`.

Fang ships a bundled default `actions.toml` (open-terminal-here, compress, extract,
open-with-editor) merged with the user file; a user entry with the same `id` overrides the
bundled one, and setting `show_in_menu = false` disables a built-in.

---

## `bookmarks.toml`

```toml
schema_version = 1

[[bookmark]]
label = "Projects"
url   = "file:///home/kroma/Development"
icon  = "folder-development"

[[bookmark]]
label = "NixOS Config"
url   = "file:///etc/nixos"
icon  = "folder"
```

Order in the file is the display order; the sidebar drag-reorder rewrites this file.

## `session.toml` (state)

Written on clean exit when `general.restore_session = true`; restored on next launch.
Captures per-window: window geometry, tabs, each tab's pane location(s), view mode, sort,
columns, split state, active tab/pane.
