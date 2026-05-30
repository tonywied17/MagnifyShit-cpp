#pragma once

#include <Windows.h>

#include <cstdint>
#include <string>
#include <vector>

namespace magshit::app {

/// @brief Every user-triggerable action that can be bound to one or more inputs.
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

/// @brief Modifier-key bitmask used by `HotkeyBinding`.
enum ModBits : std::uint8_t
{
    Mod_None  = 0,
    Mod_Ctrl  = 1 << 0,
    Mod_Shift = 1 << 1,
    Mod_Alt   = 1 << 2,
    Mod_Win   = 1 << 3,
};

/// @brief Trigger type for a binding. Wheel bindings fire on `WM_MOUSEWHEEL`.
enum class HotkeyTrigger : std::uint8_t
{
    Key       = 0,
    WheelUp   = 1,
    WheelDown = 2,
};

/// @brief One way to invoke an action.
struct HotkeyBinding
{
    std::uint8_t mods = 0;          // bitwise OR of `ModBits`
    HotkeyTrigger trigger = HotkeyTrigger::Key;
    std::uint16_t vk = 0;           // Virtual-key code when `trigger == Key`
};

/// @brief Full action-to-bindings table.
struct HotkeyMap
{
    std::vector<HotkeyBinding> bindings[static_cast<size_t>(HotkeyAction::_Count)];

    /**
     * @brief Build the built-in default bindings table.
     * @return HotkeyMap populated with default shortcuts.
     */
    static HotkeyMap defaults();

    /**
     * @brief Find the action matching a keyboard input.
     * @param mods Modifier bitmask using `ModBits`.
     * @param vk Win32 virtual-key code.
     * @return Matching action, or `HotkeyAction::_Count` when no binding matches.
     */
    HotkeyAction matchKey(std::uint8_t mods, std::uint16_t vk) const;

    /**
     * @brief Find the action matching a mouse-wheel input.
     * @param mods Modifier bitmask using `ModBits`.
     * @param delta Win32 wheel delta; positive values mean wheel-up.
     * @return Matching action, or `HotkeyAction::_Count` when no binding matches.
     */
    HotkeyAction matchWheel(std::uint8_t mods, int delta) const;
};

/// @brief Per-action metadata for UI labels and global-registration policy.
struct HotkeyActionInfo
{
    HotkeyAction action;
    const char* id;         // stable key for config serialization
    const char* label;      // user-facing label
    bool global;            // register with RegisterHotKey so it works without focus
    bool allowKey;          // accept keyboard bindings
    bool allowWheel;        // accept mouse-wheel bindings
};

/**
 * @brief Enumerate hotkey actions in UI display order.
 * @return Immutable action metadata table.
 */
const std::vector<HotkeyActionInfo>& hotkeyActionInfos();

/**
 * @brief Look up metadata for one action.
 * @param a Action to inspect.
 * @return Metadata for `a`, or a sentinel entry for invalid actions.
 */
const HotkeyActionInfo& hotkeyActionInfo(HotkeyAction a);

/**
 * @brief Read currently held keyboard modifiers.
 * @return Modifier bitmask using `ModBits`, queried via Win32 `GetKeyState`.
 */
std::uint8_t currentModifierMask();

/**
 * @brief Render a binding as `Ctrl+Shift+Wheel Up`-style text.
 * @param b Binding to format.
 * @return Human-readable shortcut label.
 */
std::string bindingToString(const HotkeyBinding& b);

/**
 * @brief Compare two bindings by their input identity.
 * @param a First binding.
 * @param b Second binding.
 * @return true when both bindings represent the same trigger, modifiers, and key.
 */
bool bindingEquals(const HotkeyBinding& a, const HotkeyBinding& b);

} // namespace magshit::app
