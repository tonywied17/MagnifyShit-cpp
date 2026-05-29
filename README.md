# MagnifyShit 2.0

<p align="center">
  <img src="https://raw.githubusercontent.com/tonywied17/MagnifyShit-cpp/refs/heads/main/.github/assets/logo.svg" alt="MagnifyShit 2.0" width="720"/>
</p>

A modern, performant, zero-dependency Windows magnifier. Built with C++20, Direct3D 11, DXGI Desktop Duplication, and Dear ImGui.

<a href="../../actions/workflows/build.yml"><img src="https://raw.githubusercontent.com/tonywied17/tonywied17/main/.github/badges/magnifyshit-ci-magnify.svg" alt="build" /></a>
<a href="../../releases/latest"><img src="https://raw.githubusercontent.com/tonywied17/tonywied17/main/.github/badges/magnifyshit-release-magnify.svg" alt="release" /></a>
<a href="../../releases"><img src="https://raw.githubusercontent.com/tonywied17/tonywied17/main/.github/badges/magnifyshit-downloads-magnify.svg" alt="downloads" /></a>
<img src="https://raw.githubusercontent.com/tonywied17/tonywied17/main/.github/badges/magnifyshit-last-commit-magnify.svg" alt="last commit" />
<a href="LICENSE"><img src="https://raw.githubusercontent.com/tonywied17/tonywied17/main/.github/badges/magnifyshit-license-magnify.svg" alt="license" /></a>

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

Requirements: Visual Studio 2022 (v17.10+), CMake >= 3.24, Windows 10/11.

```powershell
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release
.\build\windows-msvc-release\bin\Release\MagnifyShit.exe
```

Available presets:
- `windows-msvc-debug`
- `windows-msvc-release`
- `windows-msvc-asan`

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
