#include "Hotkeys.hpp"

#include <Windows.h>

#include <array>
#include <cstdio>

namespace magshit::app {

namespace {

constexpr std::uint16_t VK_OEM_PLUS_K  = VK_OEM_PLUS;
constexpr std::uint16_t VK_OEM_MINUS_K = VK_OEM_MINUS;
constexpr std::uint16_t VK_OEM_COMMA_K = VK_OEM_COMMA;

/**
 * @brief Construct a keyboard hotkey binding.
 * @param mods Modifier bitmask using `ModBits`.
 * @param vk Win32 virtual-key code.
 * @return Hotkey binding for a key trigger.
 */
HotkeyBinding key(std::uint8_t mods, std::uint16_t vk)
{
    return HotkeyBinding{mods, HotkeyTrigger::Key, vk};
}

/**
 * @brief Construct a wheel-up hotkey binding.
 * @param mods Modifier bitmask using `ModBits`.
 * @return Hotkey binding for a wheel-up trigger.
 */
HotkeyBinding wheelUp(std::uint8_t mods)
{
    return HotkeyBinding{mods, HotkeyTrigger::WheelUp, 0};
}

/**
 * @brief Construct a wheel-down hotkey binding.
 * @param mods Modifier bitmask using `ModBits`.
 * @return Hotkey binding for a wheel-down trigger.
 */
HotkeyBinding wheelDown(std::uint8_t mods)
{
    return HotkeyBinding{mods, HotkeyTrigger::WheelDown, 0};
}

const std::vector<HotkeyActionInfo> kActionInfos = {
    {HotkeyAction::ToggleOverlay,      "toggleOverlay",      "Toggle overlay panel",       false, true,  false},
    {HotkeyAction::ToggleBorderless,   "toggleBorderless",   "Toggle borderless",          false, true,  false},
    {HotkeyAction::FreezeToggle,       "freezeToggle",       "Freeze / unfreeze capture",  false, true,  false},
    {HotkeyAction::ZoomIn,             "zoomIn",             "Zoom in",                    false, true,  true },
    {HotkeyAction::ZoomOut,            "zoomOut",            "Zoom out",                   false, true,  true },
    {HotkeyAction::ResetZoom,          "resetZoom",          "Reset zoom",                 false, true,  false},
    {HotkeyAction::CycleMode,          "cycleMode",          "Cycle Static / Follow",      false, true,  false},
    {HotkeyAction::AttachToggle,       "attachToggle",       "Toggle Attached to cursor",  false, true,  false},
    {HotkeyAction::AlwaysOnTopToggle,  "alwaysOnTopToggle",  "Toggle always on top",       false, true,  false},
    {HotkeyAction::ClickThroughToggle, "clickThroughToggle", "Toggle click-through",       true,  true,  false},
    {HotkeyAction::EyedropperToggle,   "eyedropperToggle",   "Toggle eyedropper",          false, true,  false},
    {HotkeyAction::Screenshot,         "screenshot",         "Screenshot",                 false, true,  false},
    {HotkeyAction::OpenSettings,       "openSettings",       "Open settings",              false, true,  false},
    {HotkeyAction::CopyHexAtCursor,    "copyHexAtCursor",    "Copy hex of pixel at cursor",true,  true,  false},
    {HotkeyAction::Quit,               "quit",               "Quit",                       true,  true,  false},
};

} // namespace

HotkeyMap HotkeyMap::defaults()
{
    HotkeyMap m;
    auto& b = m.bindings;
    b[(size_t)HotkeyAction::ToggleOverlay]      = {key(Mod_None,            VK_F1)};
    b[(size_t)HotkeyAction::ToggleBorderless]   = {key(Mod_None,            VK_F11),
                                                   key(Mod_Ctrl,            'B')};
    b[(size_t)HotkeyAction::FreezeToggle]       = {key(Mod_None,            VK_SPACE)};
    b[(size_t)HotkeyAction::ZoomIn]             = {key(Mod_Ctrl,            VK_OEM_PLUS_K),
                                                   key(Mod_Ctrl,            VK_ADD),
                                                   wheelUp(Mod_Ctrl)};
    b[(size_t)HotkeyAction::ZoomOut]            = {key(Mod_Ctrl,            VK_OEM_MINUS_K),
                                                   key(Mod_Ctrl,            VK_SUBTRACT),
                                                   wheelDown(Mod_Ctrl)};
    b[(size_t)HotkeyAction::ResetZoom]          = {key(Mod_Ctrl,            '0'),
                                                   key(Mod_Ctrl,            VK_NUMPAD0)};
    b[(size_t)HotkeyAction::CycleMode]          = {key(Mod_Ctrl,            'W')};
    b[(size_t)HotkeyAction::AttachToggle]       = {key(Mod_Ctrl,            'M')};
    b[(size_t)HotkeyAction::AlwaysOnTopToggle]  = {key(Mod_Ctrl,            'T')};
    b[(size_t)HotkeyAction::ClickThroughToggle] = {key(Mod_Ctrl | Mod_Shift,'T')};
    b[(size_t)HotkeyAction::EyedropperToggle]   = {key(Mod_Ctrl,            'E')};
    b[(size_t)HotkeyAction::Screenshot]         = {key(Mod_Ctrl,            'S')};
    b[(size_t)HotkeyAction::OpenSettings]       = {key(Mod_Ctrl,            VK_OEM_COMMA_K)};
    b[(size_t)HotkeyAction::CopyHexAtCursor]    = {key(Mod_Ctrl | Mod_Shift,'C')};
    b[(size_t)HotkeyAction::Quit]               = {key(Mod_Ctrl | Mod_Alt,  'Q')};
    return m;
}

HotkeyAction HotkeyMap::matchKey(std::uint8_t mods, std::uint16_t vk) const
{
    for (size_t i = 0; i < (size_t)HotkeyAction::_Count; ++i)
    {
        for (const auto& bnd : bindings[i])
        {
            if (bnd.trigger == HotkeyTrigger::Key && bnd.vk == vk && bnd.mods == mods)
            {
                return static_cast<HotkeyAction>(i);
            }
        }
    }
    return HotkeyAction::_Count;
}

HotkeyAction HotkeyMap::matchWheel(std::uint8_t mods, int delta) const
{
    const HotkeyTrigger want = delta > 0 ? HotkeyTrigger::WheelUp : HotkeyTrigger::WheelDown;
    for (size_t i = 0; i < (size_t)HotkeyAction::_Count; ++i)
    {
        for (const auto& bnd : bindings[i])
        {
            if (bnd.trigger == want && bnd.mods == mods)
            {
                return static_cast<HotkeyAction>(i);
            }
        }
    }
    return HotkeyAction::_Count;
}

const std::vector<HotkeyActionInfo>& hotkeyActionInfos() { return kActionInfos; }

const HotkeyActionInfo& hotkeyActionInfo(HotkeyAction a)
{
    return kActionInfos[static_cast<size_t>(a)];
}

std::uint8_t currentModifierMask()
{
    std::uint8_t m = 0;
    if (GetKeyState(VK_CONTROL) & 0x8000) m |= Mod_Ctrl;
    if (GetKeyState(VK_SHIFT)   & 0x8000) m |= Mod_Shift;
    if (GetKeyState(VK_MENU)    & 0x8000) m |= Mod_Alt;
    if ((GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000)) m |= Mod_Win;
    return m;
}

/**
 * @brief Convert a Win32 virtual-key code into a short UI label.
 * @param vk Virtual-key code to format.
 * @return Friendly key name, or a `VK 0xNN` fallback.
 */
static std::string vkName(std::uint16_t vk)
{
    switch (vk)
    {
    case VK_F1:  return "F1";  case VK_F2:  return "F2";  case VK_F3:  return "F3";
    case VK_F4:  return "F4";  case VK_F5:  return "F5";  case VK_F6:  return "F6";
    case VK_F7:  return "F7";  case VK_F8:  return "F8";  case VK_F9:  return "F9";
    case VK_F10: return "F10"; case VK_F11: return "F11"; case VK_F12: return "F12";
    case VK_SPACE: return "Space";
    case VK_RETURN: return "Enter";
    case VK_ESCAPE: return "Esc";
    case VK_TAB: return "Tab";
    case VK_BACK: return "Backspace";
    case VK_DELETE: return "Del";
    case VK_INSERT: return "Ins";
    case VK_HOME: return "Home";
    case VK_END: return "End";
    case VK_PRIOR: return "PgUp";
    case VK_NEXT: return "PgDn";
    case VK_LEFT: return "Left";
    case VK_RIGHT: return "Right";
    case VK_UP: return "Up";
    case VK_DOWN: return "Down";
    case VK_OEM_PLUS: return "+";
    case VK_OEM_MINUS: return "-";
    case VK_OEM_COMMA: return ",";
    case VK_OEM_PERIOD: return ".";
    case VK_OEM_1: return ";";
    case VK_OEM_2: return "/";
    case VK_OEM_3: return "`";
    case VK_OEM_4: return "[";
    case VK_OEM_5: return "\\";
    case VK_OEM_6: return "]";
    case VK_OEM_7: return "'";
    case VK_ADD: return "Num +";
    case VK_SUBTRACT: return "Num -";
    case VK_MULTIPLY: return "Num *";
    case VK_DIVIDE: return "Num /";
    case VK_DECIMAL: return "Num .";
    case VK_NUMPAD0: return "Num 0"; case VK_NUMPAD1: return "Num 1";
    case VK_NUMPAD2: return "Num 2"; case VK_NUMPAD3: return "Num 3";
    case VK_NUMPAD4: return "Num 4"; case VK_NUMPAD5: return "Num 5";
    case VK_NUMPAD6: return "Num 6"; case VK_NUMPAD7: return "Num 7";
    case VK_NUMPAD8: return "Num 8"; case VK_NUMPAD9: return "Num 9";
    default: break;
    }
    if (vk >= '0' && vk <= '9') return std::string(1, static_cast<char>(vk));
    if (vk >= 'A' && vk <= 'Z') return std::string(1, static_cast<char>(vk));
    char buf[16];
    std::snprintf(buf, sizeof(buf), "VK 0x%02X", vk);
    return buf;
}

std::string bindingToString(const HotkeyBinding& b)
{
    std::string s;
    if (b.mods & Mod_Ctrl)  s += "Ctrl+";
    if (b.mods & Mod_Shift) s += "Shift+";
    if (b.mods & Mod_Alt)   s += "Alt+";
    if (b.mods & Mod_Win)   s += "Win+";
    switch (b.trigger)
    {
    case HotkeyTrigger::Key:       s += vkName(b.vk); break;
    case HotkeyTrigger::WheelUp:   s += "Wheel Up";   break;
    case HotkeyTrigger::WheelDown: s += "Wheel Down"; break;
    }
    return s;
}

bool bindingEquals(const HotkeyBinding& a, const HotkeyBinding& b)
{
    if (a.mods != b.mods || a.trigger != b.trigger) return false;
    if (a.trigger == HotkeyTrigger::Key) return a.vk == b.vk;
    return true;
}

} // namespace magshit::app
