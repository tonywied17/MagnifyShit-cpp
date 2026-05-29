#include "SettingsWindow.hpp"

#include "Theme.hpp"
#include "../app/Hotkeys.hpp"

#include <imgui.h>

#include <Windows.h>
#include <shellapi.h>

#include <cstdio>

namespace magshit::ui {

namespace {

void drawGeneralTab(UiContext& ctx)
{
    ImGui::SliderFloat("Min zoom", &ctx.state.zoomMin, 1.0f, 4.0f, "%.2fx");
    ImGui::SliderFloat("Max zoom", &ctx.state.zoomMax, 4.0f, 64.0f, "%.2fx");
    if (ctx.state.zoomMax < ctx.state.zoomMin + 0.1f)
    {
        ctx.state.zoomMax = ctx.state.zoomMin + 0.1f;
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Scaling");
    ImGui::Checkbox("Pixel-perfect snap at integer zooms", &ctx.state.pixelPerfectSnap);

    ImGui::Spacing();
    ImGui::TextDisabled("Pixel grid");
    ImGui::Checkbox("Auto show when zoom is high enough", &ctx.state.gridAuto);
    if (ctx.state.gridAuto)
    {
        ImGui::SliderFloat("Auto-show threshold", &ctx.state.gridThreshold, 2.0f, 32.0f, "%.1fx");
    }
    else
    {
        ImGui::Checkbox("Show grid", &ctx.state.gridOn);
    }
    ImGui::SliderFloat("Grid opacity", &ctx.state.gridOpacity, 0.0f, 1.0f, "%.2f");

    ImGui::Spacing();
    ImGui::TextDisabled("Window");

    if (ImGui::Checkbox("Always on top", &ctx.state.alwaysOnTop))
    {
        ctx.window.setAlwaysOnTop(ctx.state.alwaysOnTop);
    }
    bool bl = ctx.window.borderless();
    if (ImGui::Checkbox("Borderless", &bl))
    {
        ctx.window.setBorderless(bl);
    }
    ImGui::Checkbox("Freeze capture", &ctx.state.freeze);
}

void drawFiltersTab(UiContext& ctx)
{
    ImGui::TextDisabled("Color");
    ImGui::Checkbox("Invert", &ctx.state.invert);
    ImGui::SameLine();
    ImGui::Checkbox("Grayscale", &ctx.state.grayscale);

    ImGui::Spacing();
    ImGui::SliderFloat("Brightness", &ctx.state.brightness, -0.5f, 0.5f, "%+.2f");
    ImGui::SliderFloat("Contrast",   &ctx.state.contrast,    0.5f, 2.5f, "%.2f");
    ImGui::SliderFloat("Gamma",      &ctx.state.gamma,       0.4f, 2.5f, "%.2f");
    if (ImGui::Button("Reset"))
    {
        ctx.state.brightness = 0.0f;
        ctx.state.contrast = 1.0f;
        ctx.state.gamma = 1.0f;
        ctx.state.invert = false;
        ctx.state.grayscale = false;
        ctx.state.cvd = render::CvdMode::None;
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Color vision simulation");
    const char* cvd[] = {"None", "Protanopia", "Deuteranopia", "Tritanopia"};
    int cvdIdx = static_cast<int>(ctx.state.cvd);
    if (ImGui::Combo("CVD", &cvdIdx, cvd, IM_ARRAYSIZE(cvd)))
    {
        ctx.state.cvd = static_cast<render::CvdMode>(cvdIdx);
    }
}

void drawAppearanceTab(UiContext& ctx)
{
    const char* themes[] = {"Light", "Dark", "Auto (follow Windows)"};
    int idx = static_cast<int>(ctx.state.themeMode);
    if (ImGui::Combo("Theme", &idx, themes, IM_ARRAYSIZE(themes)))
    {
        ctx.state.themeMode = static_cast<ThemeMode>(idx);
        Theme::apply(ctx.state.themeMode);
    }
    ImGui::SameLine();
    ImGui::TextDisabled(
        "(effective: %s)", Theme::effective() == ThemeMode::Dark ? "Dark" : "Light");

    ImGui::Spacing();
    ImGui::Checkbox("Show overlay panel", &ctx.state.showOverlay);
}

void drawHotkeysTab(UiContext& ctx)
{
    using namespace app;
    HotkeyMap& map = ctx.state.hotkeys;
    HotkeyCapture* cap = ctx.hotkeyCapture;

    if (cap && cap->justCaptured && cap->action != HotkeyAction::_Count)
    {
        auto& vec = map.bindings[static_cast<size_t>(cap->action)];
        if (cap->slotIdx >= 0 && cap->slotIdx < static_cast<int>(vec.size()))
        {
            vec[static_cast<size_t>(cap->slotIdx)] = cap->captured;
        }
        else
        {
            vec.push_back(cap->captured);
        }
        if (hotkeyActionInfo(cap->action).global && ctx.hotkeyGlobalsDirty)
        {
            *ctx.hotkeyGlobalsDirty = true;
        }
        cap->action = HotkeyAction::_Count;
        cap->slotIdx = -1;
    }

    if (ImGui::Button("Reset all to defaults"))
    {
        map = HotkeyMap::defaults();
        if (ctx.hotkeyGlobalsDirty) *ctx.hotkeyGlobalsDirty = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Click \"+ Add\" then press the desired key, modifiers, or Ctrl+wheel. Esc cancels.");

    if (!ImGui::BeginTable(
            "hk", 2,
            ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_SizingStretchProp))
    {
        return;
    }
    ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 0.45f);
    ImGui::TableSetupColumn("Bindings", ImGuiTableColumnFlags_WidthStretch, 0.55f);
    ImGui::TableHeadersRow();

    for (const auto& info : hotkeyActionInfos())
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(info.label);
        if (info.global)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("(global)");
        }

        ImGui::TableNextColumn();
        ImGui::PushID(info.id);
        auto& vec = map.bindings[static_cast<size_t>(info.action)];

        if (cap && cap->active && cap->action == info.action)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f),
                               "Press a key or Ctrl+wheel... (Esc to cancel)");
        }
        else
        {
            for (size_t i = 0; i < vec.size(); ++i)
            {
                std::string label = bindingToString(vec[i]);
                if (label.empty()) label = "(unbound)";
                ImGui::SmallButton(label.c_str());
                ImGui::SameLine(0.0f, 2.0f);
                ImGui::PushID(static_cast<int>(i));
                if (ImGui::SmallButton("x"))
                {
                    vec.erase(vec.begin() + static_cast<long long>(i));
                    if (info.global && ctx.hotkeyGlobalsDirty)
                    {
                        *ctx.hotkeyGlobalsDirty = true;
                    }
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
                ImGui::SameLine(0.0f, 8.0f);
            }
            if (ImGui::SmallButton("+ Add"))
            {
                if (cap)
                {
                    cap->active = true;
                    cap->action = info.action;
                    cap->slotIdx = -1;
                    cap->justCaptured = false;
                }
            }
        }
        ImGui::PopID();
    }
    ImGui::EndTable();
}

void drawAboutTab()
{
    ImGui::Text("MagnifyShit 2.0");
    ImGui::TextDisabled("A modern GPU-accelerated desktop magnifier.");
    ImGui::Spacing();
    ImGui::TextWrapped("DXGI Desktop Duplication -> D3D11 fullscreen pass -> Dear ImGui.");
    ImGui::Spacing();
    ImGui::TextDisabled("Vendored: Dear ImGui, stb, nlohmann/json, doctest.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextUnformatted("by Tony Wiedman");
    ImGui::Spacing();
    const char* url = "https://github.com/tonywied17/MagnifyShit-cpp";
    ImGui::TextDisabled("Source:");
    ImGui::SameLine();
    const ImVec4 linkCol(0.40f, 0.70f, 1.00f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, linkCol);
    ImGui::TextUnformatted(url);
    ImGui::PopStyleColor();
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(min.x, max.y - 1), ImVec2(max.x, max.y - 1),
        ImGui::GetColorU32(linkCol));
    if (ImGui::IsItemHovered())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        ImGui::SetTooltip("Open in browser");
    }
    if (ImGui::IsItemClicked())
    {
        ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
    }
}

} // namespace

void SettingsWindow::draw(UiContext& ctx)
{
    if (!ctx.state.showSettings)
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(780, 520), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", &ctx.state.showSettings))
    {
        if (ImGui::BeginTabBar("settings_tabs"))
        {
            if (ImGui::BeginTabItem("General"))
            {
                drawGeneralTab(ctx);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Filters"))
            {
                drawFiltersTab(ctx);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Hotkeys"))
            {
                drawHotkeysTab(ctx);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Appearance"))
            {
                drawAppearanceTab(ctx);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("About"))
            {
                drawAboutTab();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

} // namespace magshit::ui
