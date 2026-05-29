#pragma once

#include <Windows.h>

#include <cstdint>
#include <string>
#include <vector>

namespace magshit::app {

/// Every user-triggerable action that can be bound to one or more inputs.
enum class HotkeyAction : std::uint8_t
{
    ToggleOverlay,
    ToggleBorderless,
    FreezeToggle,
    ZoomIn,
    ZoomOut,
    ResetZoom,
    CycleMode,
    AttachToggle,
    AlwaysOnTopToggle,
    ClickThroughToggle,
    EyedropperToggle,
    Screenshot,
    OpenSettings,
    CopyHexAtCursor,
    Quit,
    _Count,
};

/// Modifier-key bitmask used by `HotkeyBinding`.
enum ModBits : std::uint8_t
{
    Mod_None  = 0,
    Mod_Ctrl  = 1 << 0,
    Mod_Shift = 1 << 1,
    Mod_Alt   = 1 << 2,
    Mod_Win   = 1 << 3,
};

/// Trigger type for a binding. Wheel bindings fire on `WM_MOUSEWHEEL`.
enum class HotkeyTrigger : std::uint8_t
{
    Key       = 0,
    WheelUp   = 1,
    WheelDown = 2,
};

/// One way to invoke an action.
struct HotkeyBinding
{
    std::uint8_t mods = 0;          // bitwise OR of `ModBits`
    HotkeyTrigger trigger = HotkeyTrigger::Key;
    std::uint16_t vk = 0;           // Virtual-key code when `trigger == Key`
};

/// Full action -> bindings table.
struct HotkeyMap
{
    std::vector<HotkeyBinding> bindings[static_cast<size_t>(HotkeyAction::_Count)];

    /// Built-in default bindings.
    static HotkeyMap defaults();

    /// Find which action (if any) matches the given keyboard input.
    /// Returns `HotkeyAction::_Count` if none.
    HotkeyAction matchKey(std::uint8_t mods, std::uint16_t vk) const;

    /// Find which action (if any) matches the given wheel input. `delta`
    /// follows Win32 conventions (positive = wheel-up).
    HotkeyAction matchWheel(std::uint8_t mods, int delta) const;
};

/// Per-action metadata for UI labels and global-registration policy.
struct HotkeyActionInfo
{
    HotkeyAction action;
    const char* id;         // stable key for config serialization
    const char* label;      // user-facing label
    bool global;            // register with RegisterHotKey so it works without focus
    bool allowKey;          // accept keyboard bindings
    bool allowWheel;        // accept mouse-wheel bindings
};

/// All actions in display order.
const std::vector<HotkeyActionInfo>& hotkeyActionInfos();

/// Look up metadata for one action.
const HotkeyActionInfo& hotkeyActionInfo(HotkeyAction a);

/// Bit-mask of currently held modifiers (uses Win32 `GetKeyState`).
std::uint8_t currentModifierMask();

/// Render a binding as `Ctrl+Shift+Wheel Up`-style text.
std::string bindingToString(const HotkeyBinding& b);

/// True when `a` and `b` represent the same input.
bool bindingEquals(const HotkeyBinding& a, const HotkeyBinding& b);

} // namespace magshit::app
