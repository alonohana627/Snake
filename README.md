# Snake

Toy project. C++26, CMake + Ninja.

## Build

```bash
Scripts/build.sh [debug|release] [--tests] [--clean]
```

```powershell
Scripts\build.ps1 [-Config Debug|Release] [-Tests] [-Clean]
```

Or directly with presets: `cmake --preset debug && cmake --build --preset debug`.

Output: `build/<preset>/bin/snake`.

## Tests

Off by default. `--tests` / `-Tests` (or the `*-tests` presets) turns on
`SNAKE_BUILD_TESTS`, which adds `tests/` and runs `ctest`. No tests exist yet.
