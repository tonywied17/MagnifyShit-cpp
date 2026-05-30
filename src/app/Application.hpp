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

    // Transient (not persisted) toast banner shown briefly in the overlay.
    std::string toastText;
    double toastUntilTime = 0.0;

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

    /**
     * @brief Construct a new Application object.
     */
    Application();

    /**
     * @brief Destroy the Application object and shut down subsystems.
     */
    ~Application();

    /**
     * @brief Entry point: initialize subsystems, run the message/render loop, and shut down.
     * @param hInst The Win32 instance handle.
     * @param nCmdShow The window show command.
     * @return int The process exit code.
     */
    int run(HINSTANCE hInst, int nCmdShow);

private:

    /**
     * @brief Construct subsystems and show the main window.
     * @param hInst The Win32 instance handle.
     * @param nCmdShow The window show command.
     * @return true on success, false on fatal init failure.
     */
    bool initialize(HINSTANCE hInst, int nCmdShow);

    /**
     * @brief Tear down subsystems in reverse order and persist user state.
     */
    void shutdown();

    /**
     * @brief Render one frame: acquire capture, draw magnified output, draw UI.
     */
    void frame();

    /**
     * @brief Build and submit the ImGui frame for the overlay and settings panels.
     */
    void renderUi();

    /**
     * @brief In Attach mode, move the window so it tracks the cursor each frame.
     */
    void updateAttachedWindowPosition();

    /**
     * @brief Window procedure dispatch.
     * @param hwnd The window handle.
     * @param msg The message.
     * @param wParam WPARAM.
     * @param lParam LPARAM.
     * @param handled Set to true if the message was consumed.
     * @return LRESULT The result if handled, otherwise 0.
     */
    LRESULT handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, bool& handled);

    /**
     * @brief Apply a virtual-key shortcut from WM_KEYDOWN.
     * @param key The virtual key code.
     */
    void onKeyDown(WPARAM key);

    /**
     * @brief Zoom in/out by one wheel notch.
     * @param delta The wheel delta.
     */
    void onWheel(short delta);

    /**
     * @brief Hook for primary-button activation (currently a no-op stub).
     */
    void onLeftClick();

    /**
     * @brief Multiplicatively change zoom and clamp to [zoomMin, zoomMax].
     * @param delta The zoom delta.
     */
    void adjustZoom(float delta);

    /**
     * @brief Reset zoom to the default level.
     */
    void resetZoom();

    /**
     * @brief Cycle to the next MagnifierMode.
     */
    void cycleMode();

    /**
     * @brief Toggle borderless / standard-frame window styles.
     */
    void toggleBorderless();

    /**
     * @brief Refresh the title bar text from current state.
     */
    void updateTitle();

    /**
     * @brief Save a PNG of the current viewport to the screenshots directory.
     */
    void takeScreenshot();

    /**
     * @brief Map a client-area point to the desktop pixel currently displayed there (zoom + mode aware) and post the message to whatever window owns that pixel.
     * @param clientPt The client-area point.
     * @param msg The mouse message.
     * @param wParam WPARAM.
     * @return true if the event was forwarded, false otherwise.
     */
    bool forwardMouseEvent(POINT clientPt, UINT msg, WPARAM wParam);

    /**
     * @brief Sample the desktop pixel under the cursor and copy its hex (#RRGGBB) to the clipboard. Invoked by the Ctrl+Shift+C hotkey.
     */
    void copyHexAtCursor();

    /**
     * @brief Dispatch a single HotkeyAction to its concrete handler.
     * @param action The hotkey action.
     */
    void runAction(HotkeyAction action);

    /**
     * @brief Re-register all global actions with the OS (clears any previous registration). Called at startup and after a rebind.
     */
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
