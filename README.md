# MagnifyShit 2.1

<p align="center">
  <img src="https://raw.githubusercontent.com/tonywied17/MagnifyShit-cpp/refs/heads/main/.github/assets/logo.svg" alt="MagnifyShit 2.1" width="720"/>
</p>

Mainly created to emulate a bifocal lense as a borderless window for my one monitor that is too far away because i'm blind. 

<a href="../../actions/workflows/build.yml"><img src="https://raw.githubusercontent.com/tonywied17/tonywied17/main/.github/badges/magnifyshit-ci-magnify.svg?v=ef7a239a" alt="build" /></a>
<a href="../../releases/latest"><img src="https://raw.githubusercontent.com/tonywied17/tonywied17/main/.github/badges/magnifyshit-release-magnify.svg?v=9be1eac3" alt="release" /></a>
<a href="../../releases"><img src="https://raw.githubusercontent.com/tonywied17/tonywied17/main/.github/badges/magnifyshit-downloads-magnify.svg?v=aca150d5" alt="downloads" /></a>
<img src="https://raw.githubusercontent.com/tonywied17/tonywied17/main/.github/badges/magnifyshit-last-commit-magnify.svg?v=88f4b1f5" alt="last commit" />
<a href="LICENSE"><img src="https://raw.githubusercontent.com/tonywied17/tonywied17/main/.github/badges/magnifyshit-license-magnify.svg?v=5e4a88bb" alt="license" /></a>

> Built with C++20, Direct3D 11, DXGI Desktop Duplication, and Dear ImGui.


## Features

- **High-quality scaling**: Nearest, Bilinear, Catmull-Rom, and Lanczos-3 GPU shaders. Pixel-perfect snap at near-integer zooms.
- **DXGI Desktop Duplication capture**: tear-free, low-latency.
- **Three modes**: Static (behind the window), Follow cursor, Attach window to cursor.
- **Accessibility filters**: invert, grayscale, brightness/contrast/gamma, color-blindness simulation (Protanopia, Deuteranopia, Tritanopia) via Brettel/Vienot/Mollon matrices.
- **Pixel grid overlay** with auto/manual visibility.
- **Eyedropper**: live pixel sampling with hex / RGB readout and clipboard copy.
- **Screenshot**: PNG saved to `Pictures/MagnifyShit/` and copied to the clipboard.
- **Per-Monitor DPI v2** aware; ImGui fonts rebuild on DPI changes.
- **Custom draggable titlebar** with always-on-top, boundary outline, minimize/maximize, and exit controls.
- **Fully rebindable hotkeys** (keyboard + mouse-wheel chords), per-action global/local toggle.
- **Settings persisted** to `%LOCALAPPDATA%/MagnifyShit/config.json` (atomic writes).
- **Modern light & dark themes** with system-preference auto-mode.

## Controls

Defaults (all rebindable in **Settings -> Hotkeys**):

| Key | Action |
|:---|:---|
| `Scroll` / `Ctrl + Numpad +/-` | Zoom in/out |
| `Ctrl + 0` / `Numpad 0` | Reset zoom |
| `Ctrl + W` | Cycle Static <-> Follow-cursor |
| `Ctrl + B` | Toggle borderless |
| `Ctrl + T` | Toggle always-on-top |
| `Ctrl + E` | Toggle eyedropper |
| `Ctrl + S` | Screenshot (file + clipboard) |
| `Space` | Freeze capture |
| `F1` | Toggle overlay |
| `F11` | Toggle fullscreen |
| `Ctrl + ,` | Open settings |
| `Ctrl + Alt + Q` | Quit |
| `Esc` | Exit borderless / clear focus |

## Download

Prebuilt Windows binaries are produced by CI for every push and attached to GitHub Releases for tagged versions: see the [Releases page](../../releases) or the [latest CI artifacts](../../actions/workflows/build.yml).

## Build from source

Requirements: Visual Studio 2022 Build Tools (v17.10+), [Ninja](https://github.com/ninja-build/ninja/releases) on `PATH`, CMake >= 3.24, Windows 10/11. Run from a **Developer PowerShell for VS 2022** so `cl.exe`/`link.exe` are on `PATH`.

```powershell
cmake --preset windows                  # one binary tree, multi-config
cmake --build --preset release          # build optimized exe
ctest --preset release                  # run unit tests
.\build\Release\MagnifyShit.exe
```

Build a distributable ZIP locally — identical to what CI publishes:

```powershell
cpack --preset release
# -> build\package\MagnifyShit-2.1.1-win64.zip
```

Available presets:
- `windows` — Ninja Multi-Config (Debug + Release in one tree). Build with `--preset release` or `--preset debug`.
- `windows-asan` — Debug + AddressSanitizer (separate tree under `build-asan/`).

All dependencies (Dear ImGui, stb, nlohmann/json, doctest) are vendored under `extern/`. No package manager required.

## Project layout

```
src/
  app/        Application loop, AppState, JSON config
  capture/    DXGI Desktop Duplication
  platform/   Win32 process & window
  render/     D3D11 renderer + HLSL shaders
  tools/      Eyedropper, Screenshot
  ui/         ImGui backend, theme, overlay, settings window
  util/       Geometry, Log, Result, ScopeGuard, ComPtr
extern/       Vendored dependencies (ImGui, stb, json, doctest)
legacy/       Original v1 single-file Win32 magnifier (preserved)
.github/      CI workflows
```

## Screenshots (OLD/1.0)

![Usage Example 1](https://raw.githubusercontent.com/tonywied17/MagnifyShit-cpp/refs/heads/main/legacy/MagnifyShit/Repo%20Assets/use1.png)

![Usage Example 2](https://raw.githubusercontent.com/tonywied17/MagnifyShit-cpp/refs/heads/main/legacy/MagnifyShit/Repo%20Assets/use2.gif)

## License

MIT - see [LICENSE](LICENSE).
