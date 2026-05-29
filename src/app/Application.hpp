#pragma once

#include "../capture/windows/DxgiDuplication.hpp"
#include "../platform/windows/WinWindow.hpp"
#include "../render/d3d11/D3D11Renderer.hpp"
#include "../tools/Eyedropper.hpp"
#include "../tools/Screenshot.hpp"
#include "../ui/Theme.hpp"
#include "../util/Geometry.hpp"
#include "../ui/Theme.hpp"
#include "Hotkeys.hpp"

#include <Windows.h>

#include <filesystem>
#include <memory>
#include <string>

namespace magshit {
namespace ui {
class ImGuiBackend;
class MainOverlay;
class SettingsWindow;
} // namespace ui
} // namespace magshit

namespace magshit::app {

/// How the magnifier chooses what area of the desktop to display.
enum class MagnifierMode
{
    Static,        ///< Capture the area covered by the magnifier window.
    FollowMouse,   ///< Capture the area around the cursor; window stays put.
    AttachToMouse, ///< Window follows the cursor and captures around it.
};

/// Serializable runtime state of the magnifier (mirrors the on-disk config).
struct AppState
{
    MagnifierMode mode = MagnifierMode::Static;
    float zoom = 2.0f;
    float zoomMin = 1.0f;
    float zoomMax = 32.0f;
    render::Scaling scaling = render::Scaling::CatmullRom;
    bool pixelPerfectSnap = true;
    bool invert = false;
    bool grayscale = false;
    render::CvdMode cvd = render::CvdMode::None;
    float brightness = 0.0f;
    float contrast = 1.0f;
    float gamma = 1.0f;
    bool gridAuto = true;
    float gridThreshold = 8.0f;
    bool gridOn = false;
    float gridOpacity = 0.5f;
    bool alwaysOnTop = true;
    bool clickThrough = false;
    bool excludeFromCapture = true;
    bool freeze = false;
    bool showOverlay = true;
    bool showSettings = false;
    bool showEyedropper = false;
    bool borderless = true;
    bool showBoundary = false;
    ui::ThemeMode themeMode = ui::ThemeMode::Auto;
    std::string lastScreenshotPath;

    /// Action -> bindings table. Persisted via Config.
    HotkeyMap hotkeys = HotkeyMap::defaults();
};

/// Transient "awaiting next input" state shared between the Settings UI
/// (which initiates a rebind) and Application's input handlers (which
/// capture the next key/wheel and record it).
struct HotkeyCapture
{
    bool active = false;
    HotkeyAction action = HotkeyAction::_Count;
    int slotIdx = -1;        ///< -1 = append, else replace this binding
    bool justCaptured = false;
    HotkeyBinding captured{};
};

/// Top-level magnifier application: owns the window, renderer, capture, UI,
/// and tools, and drives the per-frame loop and Win32 message handling.
class Application
{
public:
    Application();
    ~Application();

    /// Entry point: initialize subsystems, run the message/render loop, and
    /// shut down. Returns the process exit code.
    int run(HINSTANCE hInst, int nCmdShow);

private:
    /// Construct subsystems and show the main window. Returns false on a
    /// fatal init failure.
    bool initialize(HINSTANCE hInst, int nCmdShow);

    /// Tear down subsystems in reverse order and persist user state.
    void shutdown();

    /// Render one frame: acquire capture, draw magnified output, draw UI.
    void frame();

    /// Build and submit the ImGui frame for the overlay and settings panels.
    void renderUi();

    /// In Attach mode, move the window so it tracks the cursor each frame.
    void updateAttachedWindowPosition();

    /// Window procedure dispatch. Sets `handled = true` and returns the
    /// result if the message was consumed; otherwise leaves it for default.
    LRESULT handleMessage(HWND, UINT, WPARAM, LPARAM, bool& handled);

    /// Apply a virtual-key shortcut from WM_KEYDOWN.
    void onKeyDown(WPARAM key);

    /// Zoom in/out by one wheel notch.
    void onWheel(short delta);

    /// Hook for primary-button activation (currently a no-op stub).
    void onLeftClick();

    /// Multiplicatively change zoom and clamp to [zoomMin, zoomMax].
    void adjustZoom(float delta);

    /// Reset zoom to the default level.
    void resetZoom();

    /// Cycle to the next MagnifierMode.
    void cycleMode();

    /// Toggle borderless / standard-frame window styles.
    void toggleBorderless();

    /// Refresh the title bar text from current state.
    void updateTitle();

    /// Save a PNG of the current viewport to the screenshots directory.
    void takeScreenshot();

    /// Map a client-area point to the desktop pixel currently displayed
    /// there (zoom + mode aware) and post the message to whatever window
    /// owns that pixel. Returns true if the event was forwarded.
    bool forwardMouseEvent(POINT clientPt, UINT msg, WPARAM wParam);

    /// Sample the desktop pixel under the cursor and copy its hex
    /// (#RRGGBB) to the clipboard. Invoked by the Ctrl+Shift+C hotkey.
    void copyHexAtCursor();

    /// Dispatch a single `HotkeyAction` to its concrete handler.
    void runAction(HotkeyAction action);

    /// Re-register all `global` actions with the OS (clears any previous
    /// registration). Called at startup and after a rebind.
    void rebindGlobalHotkeys();

    std::unique_ptr<platform::WinWindow> window_;
    std::unique_ptr<render::D3D11Renderer> renderer_;
    std::unique_ptr<capture::DxgiDuplication> capture_;
    std::unique_ptr<ui::ImGuiBackend> imgui_;
    std::unique_ptr<ui::MainOverlay> overlay_;
    std::unique_ptr<ui::SettingsWindow> settings_;
    std::unique_ptr<tools::Eyedropper> eyedropper_;
    std::unique_ptr<tools::Screenshot> screenshot_;
    std::filesystem::path configPath_;
    AppState state_;

    Rect lastSrcDesktop_{};
    Size lastClient_{};
    bool lastFrameValid_ = false;
    bool ctDragging_     = false;
    bool ctDragMoved_    = false;
    POINT ctDownCursor_{};

    HotkeyCapture capture_state_{};
};

} // namespace magshit::app
