# Installing what Snake needs

What you need to build and run Snake, and how to get it. In short: a C++26
compiler, CMake, Ninja, the Vulkan headers and loader, and `glslc` (the shader
compiler). GLFW and miniaudio come with the source as git submodules - nothing to
install for those.

| Platform | Status |
|---|---|
| Windows 10/11 with MSYS2 (CLANG64) | Tested - this is what the game is developed on |
| Linux (X11 or Wayland) | Should work (the code is portable) but hasn't been tested |
| macOS | Not supported yet: Vulkan there runs through MoltenVK, which needs a portability flag the renderer doesn't set yet |

## What you need

### To build

| Tool | Version | What it's for |
|---|---|---|
| Git | any recent | Getting the code, including the `deps/` submodules |
| CMake | 3.28 or newer | Generates the build (`CMakeLists.txt`, `CMakePresets.json`) |
| Ninja | any recent | Runs the build (the presets use the Ninja generator) |
| C++ compiler with C++26 support | Clang 23 + libc++ tested | The code uses C++23/26 library features (`std::print`, `std::ranges::contains`, ...). GCC 14+ should work; MSVC is untested |
| A C compiler | comes with the C++ one | GLFW and miniaudio are written in C |
| Vulkan headers + loader | 1.3 or newer | The renderer talks to the GPU through Vulkan |
| `glslc` (from shaderc) | any recent | Compiles the GLSL shaders in `src/gfx/shaders/` to SPIR-V during the build |
| Vulkan validation layers | optional | Debug builds turn them on (if found) to check every Vulkan call |

### To run

- **A GPU and driver with Vulkan 1.3**, including the `dynamicRendering` and
  `synchronization2` features. Practically any GPU from the last several years with
  an up-to-date driver (NVIDIA, AMD, Intel). If none qualifies, the game exits with
  "No GPU supports Vulkan 1.3 with dynamic rendering and synchronization2".
- **An audio output** is optional. Without one, the game prints
  "Audio: no output device" and runs silently.

## Windows (MSYS2 CLANG64)

1. Install MSYS2 from <https://www.msys2.org> (default location `C:\msys64`).

2. Open the **MSYS2 CLANG64** shell (from the Start menu) and install the tools:

   ```bash
   pacman -Syu
   pacman -S --needed git \
       mingw-w64-clang-x86_64-toolchain \
       mingw-w64-clang-x86_64-cmake \
       mingw-w64-clang-x86_64-ninja \
       mingw-w64-clang-x86_64-shaderc \
       mingw-w64-clang-x86_64-vulkan-headers \
       mingw-w64-clang-x86_64-vulkan-loader \
       mingw-w64-clang-x86_64-vulkan-validation-layers
   ```

   - `toolchain` is a group: Clang, the lld linker, libc++ and friends.
     It also offers `clangd` and `lldb` - handy for editors and debugging.
   - `shaderc` provides `glslc`.
   - `pacman -Syu` may ask you to close the shell and run it again; do so.

3. **To build from PowerShell or cmd too**, add `C:\msys64\clang64\bin` to your
   `PATH` (Windows Settings > "Edit environment variables for your account"). The
   MSYS2 CLANG64 shell has it already.

4. **Validation layers (debug builds):** the Vulkan loader doesn't look inside
   MSYS2 by itself. Tell it where the layer is, once:

   ```bash
   setx VK_ADD_LAYER_PATH "C:\msys64\clang64\bin"
   ```

   Then open a new terminal. Without this, debug builds still run - just without
   the checks.

5. **GPU driver:** install the latest driver from NVIDIA, AMD or Intel. Windows
   drivers include Vulkan support.

## Linux (untested)

Install the compiler, CMake, Ninja, Vulkan and `glslc`, plus the X11/Wayland
development headers GLFW needs to build. Audio needs nothing extra: miniaudio loads
PulseAudio/PipeWire or ALSA at runtime.

Debian / Ubuntu (22.04 or newer):

```bash
sudo apt install git cmake ninja-build clang libc++-dev \
    libvulkan-dev vulkan-validationlayers glslc \
    libwayland-dev libxkbcommon-dev xorg-dev wayland-protocols
```

Fedora:

```bash
sudo dnf install git cmake ninja-build clang libcxx-devel \
    vulkan-loader-devel vulkan-headers vulkan-validation-layers glslc \
    wayland-devel libxkbcommon-devel libXcursor-devel libXi-devel \
    libXinerama-devel libXrandr-devel wayland-protocols-devel
```

Arch:

```bash
sudo pacman -S git cmake ninja clang libc++ \
    vulkan-icd-loader vulkan-headers vulkan-validation-layers shaderc \
    wayland libxkbcommon libxcursor libxi libxinerama libxrandr wayland-protocols
```

Also install your GPU's Vulkan driver (Mesa's `mesa-vulkan-drivers` /
`vulkan-radeon` / `vulkan-intel`, or NVIDIA's driver). If your distribution's
compiler is too old for C++26, the [LunarG Vulkan SDK](https://vulkan.lunarg.com)
plus a newer Clang or GCC will do.

If you use GCC instead of Clang, drop `clang`/`libc++` above and make sure it's
GCC 14 or newer (`g++ --version`); pass `-DCMAKE_CXX_COMPILER=g++` when configuring.

## Get the code and build

```bash
git clone --recurse-submodules <repository-url> Snake
cd Snake
```

Already cloned without the submodules? Run `git submodule update --init`.
(If `deps/glfw` or `deps/miniaudio` is empty, CMake stops and says so.)

Then build with a script or with the presets directly:

```bash
Scripts/build.sh            # debug; also: release, --tests, --clean
```

```powershell
Scripts\build.ps1           # -Config Release, -Tests, -Clean
```

```bash
cmake --preset debug
cmake --build --preset debug
```

The game ends up in `build/<preset>/bin/` (`snake.exe` on Windows). Run it from
anywhere: the shaders are built into the executable and the sounds are generated
in code, so there are no files to ship next to it.

## Check that it works

- **It starts** in a window in the middle of the screen, showing the menu.
- **Debug build:** the console prints `GPU: <your GPU name>`. Any `[Vulkan] ...`
  lines are validation messages and point to a bug - please report them.
- **Settings are saved** in `snake.ini` in your user data folder (Windows:
  `%APPDATA%\Snake`). Delete it to reset everything.

## Troubleshooting

| Problem | Fix |
|---|---|
| CMake: `Could NOT find Vulkan`, mentioning `glslc` | Install shaderc (MSYS2: `mingw-w64-clang-x86_64-shaderc`; Debian/Ubuntu: `glslc`) and make sure `glslc` is on `PATH` |
| CMake: `deps/ is missing glfw/...` | `git submodule update --init` |
| Compile errors about `<print>` or `std::ranges::contains` | The compiler or standard library is too old; see the versions above |
| `No GPU supports Vulkan 1.3 ...` | Update the GPU driver; on a laptop, make sure the dedicated GPU isn't disabled |
| Debug build doesn't mention validation | Windows/MSYS2: set `VK_ADD_LAYER_PATH` (step 4); Linux: install the validation layers package |
| `Audio: no output device, the game will be silent` | No sound device was found (or it's in use exclusively by another app). The game still works |
| PowerShell: `cmake` / `ninja` / `clang++` not found | Add `C:\msys64\clang64\bin` to `PATH`, or build from the MSYS2 CLANG64 shell |
