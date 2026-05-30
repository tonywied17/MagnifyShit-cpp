#include "Theme.hpp"

#include <Windows.h>
#include <imgui.h>

namespace magshit::ui {

namespace {

/**
 * @brief Convert integer sRGB components to an ImGui color.
 * @param r Red channel in 0..255.
 * @param g Green channel in 0..255.
 * @param b Blue channel in 0..255.
 * @param a Alpha channel in 0..1.
 * @return ImGui RGBA color with normalized channels.
 */
ImVec4 rgb(int r, int g, int b, float a = 1.0f)
{
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a);
}

/**
 * @brief Apply ImGui spacing, rounding, and border defaults shared by themes.
 */
void applyCommonStyle()
{
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 8.0f;
    s.ChildRounding = 6.0f;
    s.FrameRounding = 6.0f;
    s.PopupRounding = 6.0f;
    s.GrabRounding = 4.0f;
    s.TabRounding = 6.0f;
    s.ScrollbarRounding = 8.0f;
    s.WindowPadding = ImVec2(12, 10);
    s.FramePadding = ImVec2(10, 6);
    s.ItemSpacing = ImVec2(8, 6);
    s.ItemInnerSpacing = ImVec2(6, 4);
    s.WindowBorderSize = 1.0f;
    s.FrameBorderSize = 1.0f;
    s.PopupBorderSize = 1.0f;
    s.WindowTitleAlign = ImVec2(0.0f, 0.5f);
}

} // namespace

void Theme::apply(ThemeMode mode)
{
    current_ = mode;
    applyCommonStyle();

    ThemeMode resolved = mode;
    if (mode == ThemeMode::Auto)
    {
        resolved = systemPrefersDark() ? ThemeMode::Dark : ThemeMode::Light;
    }
    effective_ = resolved;

    if (resolved == ThemeMode::Dark)
    {
        applyDark();
    }
    else
    {
        applyLight();
    }
}

void Theme::applyLight()
{
    ImVec4* c = ImGui::GetStyle().Colors;
    c[ImGuiCol_WindowBg]            = rgb(247, 248, 250, 0.92f);
    c[ImGuiCol_ChildBg]             = rgb(255, 255, 255);
    c[ImGuiCol_PopupBg]             = rgb(255, 255, 255);
    c[ImGuiCol_Border]              = rgb(227, 230, 234);
    c[ImGuiCol_BorderShadow]        = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_Text]                = rgb(27, 31, 35);
    c[ImGuiCol_TextDisabled]        = rgb(110, 118, 129);
    c[ImGuiCol_FrameBg]             = rgb(255, 255, 255);
    c[ImGuiCol_FrameBgHovered]      = rgb(241, 243, 245);
    c[ImGuiCol_FrameBgActive]       = rgb(231, 235, 240);
    c[ImGuiCol_TitleBg]             = rgb(255, 255, 255);
    c[ImGuiCol_TitleBgActive]       = rgb(247, 248, 250);
    c[ImGuiCol_TitleBgCollapsed]    = rgb(255, 255, 255);
    c[ImGuiCol_MenuBarBg]           = rgb(255, 255, 255);
    c[ImGuiCol_ScrollbarBg]         = rgb(255, 255, 255);
    c[ImGuiCol_ScrollbarGrab]       = rgb(225, 228, 233);
    c[ImGuiCol_ScrollbarGrabHovered]= rgb(199, 205, 212);
    c[ImGuiCol_ScrollbarGrabActive] = rgb(171, 179, 188);
    c[ImGuiCol_Button]              = rgb(255, 255, 255);
    c[ImGuiCol_ButtonHovered]       = rgb(241, 243, 245);
    c[ImGuiCol_ButtonActive]        = rgb(231, 235, 240);
    c[ImGuiCol_Header]              = rgb(238, 242, 255);
    c[ImGuiCol_HeaderHovered]       = rgb(224, 231, 255);
    c[ImGuiCol_HeaderActive]        = rgb(199, 210, 254);
    c[ImGuiCol_CheckMark]           = rgb(37, 99, 235);
    c[ImGuiCol_SliderGrab]          = rgb(37, 99, 235);
    c[ImGuiCol_SliderGrabActive]    = rgb(29, 78, 216);
    c[ImGuiCol_Separator]           = rgb(227, 230, 234);
    c[ImGuiCol_SeparatorHovered]    = rgb(199, 210, 254);
    c[ImGuiCol_SeparatorActive]     = rgb(37, 99, 235);
    c[ImGuiCol_Tab]                 = rgb(247, 248, 250);
    c[ImGuiCol_TabHovered]          = rgb(238, 242, 255);
    c[ImGuiCol_TabActive]           = rgb(255, 255, 255);
    c[ImGuiCol_TabUnfocused]        = rgb(247, 248, 250);
    c[ImGuiCol_TabUnfocusedActive]  = rgb(255, 255, 255);
    c[ImGuiCol_NavHighlight]        = rgb(37, 99, 235);
}

void Theme::applyDark()
{
    ImVec4* c = ImGui::GetStyle().Colors;
    c[ImGuiCol_WindowBg]            = rgb(22, 24, 29, 0.94f);
    c[ImGuiCol_ChildBg]             = rgb(30, 33, 39);
    c[ImGuiCol_PopupBg]             = rgb(30, 33, 39);
    c[ImGuiCol_Border]              = rgb(48, 53, 61);
    c[ImGuiCol_BorderShadow]        = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_Text]                = rgb(232, 234, 237);
    c[ImGuiCol_TextDisabled]        = rgb(140, 146, 154);
    c[ImGuiCol_FrameBg]             = rgb(38, 42, 49);
    c[ImGuiCol_FrameBgHovered]      = rgb(48, 53, 61);
    c[ImGuiCol_FrameBgActive]       = rgb(58, 64, 73);
    c[ImGuiCol_TitleBg]             = rgb(22, 24, 29);
    c[ImGuiCol_TitleBgActive]       = rgb(30, 33, 39);
    c[ImGuiCol_TitleBgCollapsed]    = rgb(22, 24, 29);
    c[ImGuiCol_MenuBarBg]           = rgb(30, 33, 39);
    c[ImGuiCol_ScrollbarBg]         = rgb(22, 24, 29);
    c[ImGuiCol_ScrollbarGrab]       = rgb(60, 66, 75);
    c[ImGuiCol_ScrollbarGrabHovered]= rgb(82, 89, 99);
    c[ImGuiCol_ScrollbarGrabActive] = rgb(105, 113, 124);
    c[ImGuiCol_Button]              = rgb(38, 42, 49);
    c[ImGuiCol_ButtonHovered]       = rgb(48, 53, 61);
    c[ImGuiCol_ButtonActive]        = rgb(58, 64, 73);
    c[ImGuiCol_Header]              = rgb(45, 56, 87);
    c[ImGuiCol_HeaderHovered]       = rgb(56, 70, 110);
    c[ImGuiCol_HeaderActive]        = rgb(67, 84, 133);
    c[ImGuiCol_CheckMark]           = rgb(96, 165, 250);
    c[ImGuiCol_SliderGrab]          = rgb(96, 165, 250);
    c[ImGuiCol_SliderGrabActive]    = rgb(59, 130, 246);
    c[ImGuiCol_Separator]           = rgb(48, 53, 61);
    c[ImGuiCol_SeparatorHovered]    = rgb(96, 165, 250);
    c[ImGuiCol_SeparatorActive]     = rgb(59, 130, 246);
    c[ImGuiCol_Tab]                 = rgb(30, 33, 39);
    c[ImGuiCol_TabHovered]          = rgb(56, 70, 110);
    c[ImGuiCol_TabActive]           = rgb(45, 56, 87);
    c[ImGuiCol_TabUnfocused]        = rgb(30, 33, 39);
    c[ImGuiCol_TabUnfocusedActive]  = rgb(38, 42, 49);
    c[ImGuiCol_NavHighlight]        = rgb(96, 165, 250);
}

bool Theme::systemPrefersDark()
{
    HKEY key{};
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                      0,
                      KEY_READ,
                      &key)
        != ERROR_SUCCESS)
    {
        return false;
    }
    DWORD value = 1;
    DWORD size = sizeof(value);
    DWORD type = REG_DWORD;
    const LONG r = RegQueryValueExW(
        key, L"AppsUseLightTheme", nullptr, &type, reinterpret_cast<LPBYTE>(&value), &size);
    RegCloseKey(key);
    if (r != ERROR_SUCCESS || type != REG_DWORD)
    {
        return false;
    }
    return value == 0;
}

} // namespace magshit::ui
