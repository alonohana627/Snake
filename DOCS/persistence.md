# Persistence: `snake.ini`

The game remembers its **settings** and the **best score** between runs, in one
INI file.

## Where

`snake.ini` in the current user's data folder
([files.cpp](../src/storage/files.cpp), `storage::userDataDirectory`):

| OS | Folder |
|---|---|
| Windows | `%APPDATA%\Snake` (e.g. `C:\Users\you\AppData\Roaming\Snake`) |
| Linux | `$XDG_DATA_HOME/Snake`, usually `~/.local/share/Snake` |
| macOS | `~/Library/Application Support/Snake` |

Never next to the executable: that folder is often read-only (Program Files). On
Windows the path is read with `_wgetenv` (UTF-16), so user names with non-ASCII
letters work.

The **`SNAKE_DATA_DIR`** environment variable overrides the folder - for testing
without touching the real file.

## Format

```ini
; Snake - settings and best score.
; Edit only while the game is closed: it rewrites this file.
version = 1

[settings]
fullscreen = false
speed = normal
walls = wrap
sound = medium

[score]
best = 42
```

| Key | Values | Default |
|---|---|---|
| `settings.fullscreen` | `true`, `false` | `false` |
| `settings.speed` | `slow`, `normal`, `fast` | `normal` |
| `settings.walls` | `wrap`, `solid` | `wrap` |
| `settings.sound` | `off`, `low`, `medium`, `high` | `medium` |
| `score.best` | a whole number, 0 or more | `0` |
| `version` | `1` | |

**`version`** is for the future: bump it only when an existing key's *meaning*
changes, so a newer game can recognize and convert old files. Adding a key doesn't
need it - a missing key just gets its default.

## Reading: forgiving by design

People edit this file by hand, so reading never fails
([save_data.cpp](../src/app/save_data.cpp), `loadSaveData`):

| In the file | Result |
|---|---|
| No file (first run) | All defaults; the game then writes the file |
| A missing key | That key's default |
| A value with different case (`Fast`) | Accepted |
| An unknown value (`walls = bricks`) | That key's default |
| A bad number (`-5`, `12abc`, too big to fit) | Default (`0`) |
| A line that isn't INI, a broken `[header` | Skipped; the rest still loads |
| An empty or garbage file | All defaults |

Numbers are parsed with `std::from_chars`: it never throws and doesn't depend on
the system's language settings (unlike `std::stoi` / `atoi`).

## Writing: never half a file

`storage::writeFileAtomically` ([files.hpp](../include/snake/storage/files.hpp)):

1. Creates the folder if needed.
2. Writes everything to `snake.ini.tmp` (in binary mode, so Windows doesn't change
   line endings).
3. Renames `snake.ini.tmp` over `snake.ini`.

A rename is all-or-nothing: if the game crashes while writing, the old file is
still intact. (It doesn't `fsync`, so a power cut at the wrong moment could still
lose the newest write - acceptable for a game's settings.)

If writing fails (folder not writable, disk full...), the game prints
`Couldn't save settings: ...` and carries on.

## When it's written

Only when something changed since the last write (`Application::save()` compares
with what it last wrote):

- after a settings change, including F11,
- when a round ends (a crash) - the best score may be new,
- when the menu opens,
- when the game closes (menu QUIT or the window's X).

## The INI reader/writer

[ini.hpp](../include/snake/storage/ini.hpp) is about 100 lines, no library:

```cpp
storage::Ini ini = storage::Ini::parse(text);
std::optional<std::string_view> speed = ini.get("settings", "speed");
ini.set("score", "best", "42");
std::string text = ini.toString("header comment");
```

- `[section]` lines start a section; `key = value` lines belong to the last one
  (keys before any header are in section `""`).
- Lines starting with `;` or `#` are comments. Comments at the end of a line aren't
  supported - `best = 5 ; mine` would read the value `5 ; mine` (and fall back to 0).
- Whitespace around keys and values is trimmed, Windows line endings included.
- `toString` writes sections in the order their first key was set.

It knows nothing about Snake; `app/save_data.cpp` decides what goes in it and how
each value is spelled (the `kSpeedNames`, `kWallsNames`, ... tables).
