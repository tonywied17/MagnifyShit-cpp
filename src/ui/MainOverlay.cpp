#include "MainOverlay.hpp"

#include "../app/Hotkeys.hpp"
#include "../util/Geometry.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <string>

namespace magshit::ui {

namespace {

const char* modeLabel(app::MagnifierMode m)
{
    switch (m)
    {
    case app::MagnifierMode::Static:        return "Static";
    case app::MagnifierMode::FollowMouse:   return "Follow Cursor";
    case app::MagnifierMode::AttachToMouse: return "Attached";
    }
    return "?";
}

void helpMarker(const char* desc)
{
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void drawShowHint(UiContext& ctx, const Size& cs)
{
    const ImVec2 pad{12, 12};
    ImGui::SetNextWindowBgAlpha(0.55f);
    ImGui::SetNextWindowPos(ImVec2(cs.w - pad.x, pad.y), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    const ImGuiWindowFlags chipFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
                                       | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse
                                       | ImGuiWindowFlags_AlwaysAutoResize
                                       | ImGuiWindowFlags_NoSavedSettings
                                       | ImGuiWindowFlags_NoFocusOnAppearing
                                       | ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("##magshit_hint", nullptr, chipFlags))
    {
        const auto& toggleBinds = ctx.state.hotkeys.bindings[
            static_cast<size_t>(app::HotkeyAction::ToggleOverlay)];
        const std::string toggleKey = toggleBinds.empty()
            ? std::string("F1") : app::bindingToString(toggleBinds.front());
        if (ImGui::SmallButton(("show " + toggleKey + "##show").c_str()))
        {
            ctx.state.showOverlay = true;
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.72f, 0.18f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.86f, 0.24f, 0.24f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.55f, 0.12f, 0.12f, 1.0f));
        if (ImGui::SmallButton("exit##hint_exit"))
        {
            PostMessageW(ctx.window.handle(), WM_CLOSE, 0, 0);
        }
        ImGui::PopStyleColor(3);
    }
    ImGui::End();
}

} // namespace

void MainOverlay::draw(UiContext& ctx)
{
    const Size cs = ctx.window.clientSize();

    if (ctx.state.showBoundary)
    {
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        const ImU32 col = IM_COL32(255, 200, 60, 220);
        const float inset = 1.0f;
        const float x0 = inset, y0 = inset;
        const float x1 = static_cast<float>(cs.w) - inset;
        const float y1 = static_cast<float>(cs.h) - inset;
        const float dash = 8.0f, gap = 6.0f, th = 2.0f;
        // top & bottom
        for (float x = x0; x < x1; x += dash + gap)
        {
            const float xe = std::min(x + dash, x1);
            fg->AddLine(ImVec2(x, y0), ImVec2(xe, y0), col, th);
            fg->AddLine(ImVec2(x, y1), ImVec2(xe, y1), col, th);
        }
        // left & right
        for (float y = y0; y < y1; y += dash + gap)
        {
            const float ye = std::min(y + dash, y1);
            fg->AddLine(ImVec2(x0, y), ImVec2(x0, ye), col, th);
            fg->AddLine(ImVec2(x1, y), ImVec2(x1, ye), col, th);
        }
    }

    if (!ctx.state.showOverlay)
    {
        drawShowHint(ctx, cs);
        return;
    }

    const float scale = ImGui::GetIO().FontGlobalScale > 0 ? 1.0f : 1.0f;
    const ImVec2 pad{12 * scale, 12 * scale};
    const ImVec2 size{520 * scale, 0};
    ImGui::SetNextWindowPos(ImVec2(cs.w - size.x - pad.x, pad.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w);

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
                                   | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse
                                   | ImGuiWindowFlags_AlwaysAutoResize
                                   | ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin("##magshit_overlay", nullptr, flags))
    {
        HWND const hwnd = ctx.window.handle();
        const bool customTitlebar = ctx.window.borderless();

        if (customTitlebar)
        {
            // Custom titlebar: drag region + window controls.
            const float btnH = ImGui::GetFrameHeight();
            const float btnW = btnH; // square icon buttons
            const float spacing = 2.0f * scale;
            const auto& toggleBinds = ctx.state.hotkeys.bindings[
                static_cast<size_t>(app::HotkeyAction::ToggleOverlay)];
            std::string toggleKey = toggleBinds.empty()
                                        ? std::string("F1")
                                        : app::bindingToString(toggleBinds.front());
            std::string toggleLabel = "hide " + toggleKey;
            const float togglePad = ImGui::GetStyle().FramePadding.x * 2.0f;
            const float toggleW = ImGui::CalcTextSize(toggleLabel.c_str()).x + togglePad;
            const int nSquare = 5; // pin, boundary, minimize, maximize, exit
            const float btnsW = nSquare * btnW + (nSquare - 1) * spacing
                              + spacing + toggleW;
            const float avail = ImGui::GetContentRegionAvail().x;
            const float dragW = avail - btnsW - spacing;

            const ImVec2 dragPos = ImGui::GetCursorScreenPos();
            const float dragH = btnH;
            ImGui::InvisibleButton("##titlebar_drag", ImVec2(dragW, dragH));
        const bool dragHovered = ImGui::IsItemHovered();
        const bool dragHeld    = ImGui::IsItemActive();
        if (ImGui::IsItemActivated())
        {
            ReleaseCapture();
            PostMessageW(hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
        }
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 dragEnd(dragPos.x + dragW, dragPos.y + dragH);
        const ImU32 bgCol = ImGui::GetColorU32(
            dragHeld    ? ImGuiCol_TitleBgActive
            : dragHovered ? ImGuiCol_HeaderHovered
                          : ImGuiCol_TitleBg);
        dl->AddRectFilled(dragPos, dragEnd, bgCol, 6.0f);
        const ImU32 borderCol = ImGui::GetColorU32(
            dragHovered ? ImGuiCol_SeparatorHovered : ImGuiCol_Border);
        dl->AddRect(dragPos, dragEnd, borderCol, 6.0f, 0, 1.0f);
        // Hatched grip strip on the LEFT end (so it never collides with the status text).
        const ImU32 gripCol = ImGui::GetColorU32(
            dragHovered ? ImGuiCol_Text : ImGuiCol_TextDisabled);
        const float gripCols = 4.0f;
        const float gripStep = 4.0f;
        const float gripLeft   = dragPos.x + 8.0f;
        const float gripTop    = dragPos.y + 5.0f;
        const float gripBottom = dragEnd.y - 5.0f;
        for (int i = 0; i < (int)gripCols; ++i)
        {
            const float gx = gripLeft + i * gripStep;
            dl->AddLine(ImVec2(gx, gripTop), ImVec2(gx, gripBottom), gripCol, 1.4f);
        }
        const float gripRight = gripLeft + (gripCols - 1) * gripStep;
        const ImU32 titleCol = ImGui::GetColorU32(ImGuiCol_Text);
        const ImU32 dimCol   = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        char status[128];
        std::snprintf(status, sizeof(status), "  %s  %.2fx",
                      modeLabel(ctx.state.mode), ctx.state.zoom);
        const float ty = dragPos.y + (dragH - ImGui::GetTextLineHeight()) * 0.5f;
        const float tx = gripRight + 10.0f;
        const float titleW = ImGui::CalcTextSize("MagnifyShit 2.0").x;
        const float statusW = ImGui::CalcTextSize(status).x;
        const float rightLimit = dragEnd.x - 8.0f;
        if (tx + titleW <= rightLimit)
        {
            dl->AddText(ImVec2(tx, ty), titleCol, "MagnifyShit 2.0");
            if (tx + titleW + statusW <= rightLimit)
            {
                dl->AddText(ImVec2(tx + titleW, ty), dimCol, status);
            }
        }
        if (dragHovered)
        {
            ImGui::SetTooltip("Drag to move window");
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        }

        ImGui::SameLine(0.0f, spacing);
        ImGui::PushStyleColor(ImGuiCol_Button,
                              ctx.state.alwaysOnTop ? ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)
                                                    : ImGui::GetStyleColorVec4(ImGuiCol_Button));
        if (ImGui::Button(ctx.state.alwaysOnTop ? "*" : "o", ImVec2(btnW, btnH)))
        {
            ctx.state.alwaysOnTop = !ctx.state.alwaysOnTop;
            ctx.window.setAlwaysOnTop(ctx.state.alwaysOnTop);
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Always on top");

        ImGui::SameLine(0.0f, spacing);
        ImGui::PushStyleColor(ImGuiCol_Button,
                              ctx.state.showBoundary ? ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)
                                                     : ImGui::GetStyleColorVec4(ImGuiCol_Button));
        if (ImGui::Button("[ ]", ImVec2(btnW, btnH)))
        {
            ctx.state.showBoundary = !ctx.state.showBoundary;
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle window boundary outline");

        ImGui::SameLine(0.0f, spacing);
        if (ImGui::Button("_", ImVec2(btnW, btnH)))
        {
            ShowWindow(hwnd, SW_MINIMIZE);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Minimize");

        ImGui::SameLine(0.0f, spacing);
        const bool zoomed = IsZoomed(hwnd) != 0;
        if (ImGui::Button(zoomed ? "=" : "[]", ImVec2(btnW, btnH)))
        {
            ShowWindow(hwnd, zoomed ? SW_RESTORE : SW_MAXIMIZE);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(zoomed ? "Restore" : "Maximize");

        ImGui::SameLine(0.0f, spacing);
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.72f, 0.18f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.86f, 0.24f, 0.24f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.55f, 0.12f, 0.12f, 1.0f));
        if (ImGui::Button("X##exit", ImVec2(btnW, btnH)))
        {
            PostMessageW(hwnd, WM_CLOSE, 0, 0);
        }
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Exit MagnifyShit");

        ImGui::SameLine(0.0f, spacing);
        if (ImGui::Button(toggleLabel.c_str(), ImVec2(toggleW, btnH)))
        {
            ctx.state.showOverlay = !ctx.state.showOverlay;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Toggle panel (%s)", toggleKey.c_str());

        ImGui::Separator();
        } // customTitlebar
        else
        {
            // Bordered window: OS chrome provides the titlebar; just show
            // a compact status line + hide button.
            ImGui::TextUnformatted("MagnifyShit 2.0");
            ImGui::SameLine();
            ImGui::TextDisabled(" | %s  %.2fx", modeLabel(ctx.state.mode), ctx.state.zoom);
            ImGui::SameLine(ImGui::GetWindowWidth() - 28 * scale);
            if (ImGui::SmallButton("X"))
            {
                ctx.state.showOverlay = false;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Hide this panel (F1)");
            ImGui::Separator();
        }

        ImGui::SetNextItemWidth(-120 * scale);
        ImGui::SliderFloat(
            "##zoom", &ctx.state.zoom, ctx.state.zoomMin, ctx.state.zoomMax, "%.2fx");
        ImGui::SameLine();
        ImGui::TextUnformatted("Zoom");
        helpMarker("Magnification factor. Ctrl + scroll wheel or Ctrl +/- also work; Ctrl+0 resets to 1x.");

        const char* modes[] = {"Static (behind window)", "Follow cursor", "Attached to cursor"};
        int idx = static_cast<int>(ctx.state.mode);
        ImGui::SetNextItemWidth(-120 * scale);
        if (ImGui::Combo("##mode", &idx, modes, IM_ARRAYSIZE(modes)))
        {
            ctx.state.mode = static_cast<app::MagnifierMode>(idx);
        }
        ImGui::SameLine();
        ImGui::TextUnformatted("Mode");
        helpMarker(
            "Static: zoom the area behind the window.\n"
            "Follow cursor: window stays put, view follows the cursor.\n"
            "Attached: the window itself follows the cursor (great with borderless).\n"
            "\nCtrl+M toggles Attached <-> Static. In Attached mode, hovering the UI "
            "panel pins the window so you can click controls. In Borderless mode, drag "
            "the window by left-clicking any empty area.");

        const char* scaling[] = {"Nearest", "Bilinear", "Catmull-Rom", "Lanczos 3"};
        int sIdx = static_cast<int>(ctx.state.scaling);
        ImGui::SetNextItemWidth(-120 * scale);
        if (ImGui::Combo("##scaling", &sIdx, scaling, IM_ARRAYSIZE(scaling)))
        {
            ctx.state.scaling = static_cast<render::Scaling>(sIdx);
        }
        ImGui::SameLine();
        ImGui::TextUnformatted("Scaling");
        helpMarker(
            "Nearest: pixel-perfect, hard edges.\n"
            "Bilinear: soft, fast.\n"
            "Catmull-Rom: sharp resamples, great for text.\n"
            "Lanczos 3: highest quality, slowest.\n"
            "Pixel-perfect snap (Settings) will switch to Nearest near integer zooms.");

        ImGui::Separator();

        if (ImGui::Checkbox("Always on top", &ctx.state.alwaysOnTop))
        {
            ctx.window.setAlwaysOnTop(ctx.state.alwaysOnTop);
        }
        ImGui::SameLine();
        ImGui::Checkbox("Freeze", &ctx.state.freeze);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Pause capture (Space)");
        }

        if (ImGui::Checkbox("Exclude from capture", &ctx.state.excludeFromCapture))
        {
            ctx.window.setExcludedFromCapture(ctx.state.excludeFromCapture);
        }
        helpMarker(
            "When ON, this window is invisible to all screen-capture APIs (including our own "
            "magnifier). Keep ON to avoid the feedback-loop / mirror effect you get when you "
            "zoom into yourself. Requires Windows 10 2004+.");

        if (ImGui::Checkbox("Click-through", &ctx.state.clickThrough))
        {
            ctx.window.setClickThrough(ctx.state.clickThrough);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(Ctrl+Shift+T)");
        helpMarker(
            "Mouse input lands on whatever is beneath the magnified area at the "
            "ZOOMED desktop pixel you're pointing at (the cursor will visibly snap "
            "there during the click - that's the trade-off in user-mode without the "
            "Magnification API). Wheel still zooms us; the control panel is still "
            "fully interactive (hover, scroll, drag).\n\n"
            "Drags work via mouse capture - hold a button and the OS keeps "
            "delivering moves to the magnified pixel until you release.\n\n"
            "Global escape: Ctrl+Shift+T toggles click-through, Ctrl+Alt+Q quits.");

        ImGui::Separator();
        if (ImGui::Button("Settings...", ImVec2(-1, 0)))
        {
            ctx.state.showSettings = !ctx.state.showSettings;
        }

        if (ImGui::Checkbox("Eyedropper", &ctx.state.showEyedropper))
        {
        }
        if (ctx.state.showEyedropper && ctx.eyedropper)
        {
            ImGui::SameLine();
            const auto& c = ctx.eyedropper->last();
            if (c.has_value())
            {
                const ImVec4 col(c->r / 255.0f, c->g / 255.0f, c->b / 255.0f, 1.0f);
                ImGui::ColorButton("##swatch", col,
                                   ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoLabel,
                                   ImVec2(28 * scale, 18 * scale));
                ImGui::SameLine();
                char hex[16];
                std::snprintf(hex, sizeof(hex), "#%02X%02X%02X", c->r, c->g, c->b);
                ImGui::Text("%s  rgb(%u,%u,%u)", hex, c->r, c->g, c->b);
                ImGui::SameLine();
                const auto& copyBinds = ctx.state.hotkeys.bindings[
                    static_cast<size_t>(app::HotkeyAction::CopyHexAtCursor)];
                std::string hint = copyBinds.empty()
                                       ? std::string("(unbound)")
                                       : app::bindingToString(copyBinds.front());
                ImGui::TextDisabled("copy: %s", hint.c_str());
            }
            else
            {
                ImGui::TextDisabled("Hover the desktop to sample a pixel.");
            }
        }

        ImGui::Separator();
        if (ImGui::CollapsingHeader("Shortcuts"))
        {
            if (ImGui::BeginTable("##sc", 2,
                                  ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoBordersInBody))
            {
                auto row = [](const char* k, const char* v) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextDisabled("%s", k);
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(v);
                };
                auto actionRow = [&](app::HotkeyAction a, const char* desc) {
                    const auto& binds = ctx.state.hotkeys.bindings[static_cast<size_t>(a)];
                    std::string keys;
                    if (binds.empty())
                    {
                        keys = "(unbound)";
                    }
                    else
                    {
                        for (size_t i = 0; i < binds.size(); ++i)
                        {
                            if (i) keys += "  /  ";
                            keys += app::bindingToString(binds[i]);
                        }
                    }
                    row(keys.c_str(), desc);
                };
                actionRow(app::HotkeyAction::ToggleOverlay,      "Toggle this panel");
                actionRow(app::HotkeyAction::FreezeToggle,       "Freeze / unfreeze capture");
                row("Esc",                                       "Step back: click-through > Attach > borderless");
                actionRow(app::HotkeyAction::ZoomIn,             "Zoom in");
                actionRow(app::HotkeyAction::ZoomOut,            "Zoom out");
                actionRow(app::HotkeyAction::ResetZoom,          "Reset zoom to 1x");
                actionRow(app::HotkeyAction::CycleMode,          "Cycle Static / Follow cursor");
                actionRow(app::HotkeyAction::AttachToggle,       "Toggle Attached-to-cursor");
                actionRow(app::HotkeyAction::ToggleBorderless,   "Toggle borderless");
                actionRow(app::HotkeyAction::AlwaysOnTopToggle,  "Toggle always-on-top");
                actionRow(app::HotkeyAction::ClickThroughToggle, "Toggle click-through (global)");
                actionRow(app::HotkeyAction::Quit,               "Quit (global escape)");
                actionRow(app::HotkeyAction::EyedropperToggle,   "Toggle eyedropper");
                actionRow(app::HotkeyAction::CopyHexAtCursor,    "Copy hex of pixel under cursor (global)");
                actionRow(app::HotkeyAction::Screenshot,         "Screenshot (file + clipboard)");
                actionRow(app::HotkeyAction::OpenSettings,       "Open settings");
                ImGui::EndTable();
            }
        }

        if (!ctx.state.lastScreenshotPath.empty())
        {
            ImGui::Separator();
            ImGui::TextDisabled("Last shot: %s", ctx.state.lastScreenshotPath.c_str());
        }
    }
    ImGui::End();
}

} // namespace magshit::ui
