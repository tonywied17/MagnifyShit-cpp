#include "MainOverlay.hpp"

#include "../app/Hotkeys.hpp"
#include "../util/Geometry.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <imgui.h>
#include <string>
#include <Windows.h>

namespace magshit::ui {

namespace {

/**
 * @brief Convert a magnifier mode to its short UI label.
 * @param m Mode to format.
 * @return Static label for the mode.
 */
const char* modeLabel(app::MagnifierMode m)
{
    switch (m)
    {
    case app::MagnifierMode::Static:
        return "Static";
    case app::MagnifierMode::FollowMouse:
        return "Follow Cursor";
    case app::MagnifierMode::AttachToMouse:
        return "Attached";
    }
    return "?";
}

/**
 * @brief Draw a hoverable inline help marker.
 * @param desc Tooltip text shown while hovering the marker.
 */
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

/**
 * @brief Draw the compact overlay-reopen chip shown while the panel is hidden.
 * @param ctx Live UI context used to mutate app/window state.
 * @param cs Current client-area size used to anchor the chip.
 */
void drawShowHint(UiContext& ctx, const Size& cs)
{
    const ImVec2 pad{12, 12};
    ImGui::SetNextWindowBgAlpha(0.55f);
    ImGui::SetNextWindowPos(ImVec2(cs.w - pad.x, pad.y), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    const ImGuiWindowFlags chipFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_AlwaysAutoResize |
                                       ImGuiWindowFlags_NoSavedSettings |
                                       ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("##magshit_hint", nullptr, chipFlags))
    {
        HWND const hwnd = ctx.window.handle();
        const bool customTitlebar = ctx.window.borderless();
        const auto& toggleBinds =
            ctx.state.hotkeys.bindings[static_cast<size_t>(app::HotkeyAction::ToggleOverlay)];
        const std::string toggleKey =
            toggleBinds.empty() ? std::string("F1") : app::bindingToString(toggleBinds.front());

        const float btnH = ImGui::GetFrameHeight();
        const float btnW = btnH;
        const float spacing = 2.0f;
        const float groupGap = 8.0f;

        // Left: panel-toggle
        const ImVec4 accentBtn(0.20f, 0.32f, 0.55f, 1.0f);
        const ImVec4 accentBtnHovered(0.28f, 0.42f, 0.68f, 1.0f);
        const ImVec4 accentBtnActive(0.34f, 0.50f, 0.78f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, accentBtn);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, accentBtnHovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, accentBtnActive);
        if (ImGui::Button("^##show", ImVec2(btnW, btnH)))
        {
            ctx.state.showOverlay = true;
        }
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Show control panel (%s).\nMagnifier keeps running.",
                              toggleKey.c_str());

        if (customTitlebar)
        {
            ImGui::SameLine(0.0f, groupGap);
            const ImVec2 dragPos = ImGui::GetCursorScreenPos();
            ImGui::InvisibleButton("##hint_drag", ImVec2(72.0f, btnH));
            const bool dragHovered = ImGui::IsItemHovered();
            const bool dragHeld = ImGui::IsItemActive();
            if (ImGui::IsItemActivated())
            {
                ReleaseCapture();
                PostMessageW(hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
            }
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 dragEnd(dragPos.x + 72.0f, dragPos.y + btnH);
            const ImU32 bgCol = ImGui::GetColorU32(dragHeld      ? ImGuiCol_TitleBgActive
                                                   : dragHovered ? ImGuiCol_HeaderHovered
                                                                 : ImGuiCol_TitleBg);
            dl->AddRectFilled(dragPos, dragEnd, bgCol, 6.0f);
            const ImU32 borderCol =
                ImGui::GetColorU32(dragHovered ? ImGuiCol_SeparatorHovered : ImGuiCol_Border);
            dl->AddRect(dragPos, dragEnd, borderCol, 6.0f, 0, 1.0f);
            const ImU32 gripCol =
                ImGui::GetColorU32(dragHovered ? ImGuiCol_Text : ImGuiCol_TextDisabled);
            const int gripCols = 4;
            const float gripStep = 4.0f;
            const float gripSpan = (gripCols - 1) * gripStep;
            const float gripLeft = dragPos.x + (72.0f - gripSpan) * 0.5f;
            const float gripTop = dragPos.y + 5.0f;
            const float gripBottom = dragEnd.y - 5.0f;
            for (int i = 0; i < gripCols; ++i)
            {
                const float gx = gripLeft + i * gripStep;
                dl->AddLine(ImVec2(gx, gripTop), ImVec2(gx, gripBottom), gripCol, 1.4f);
            }
            if (dragHovered)
            {
                ImGui::SetTooltip("Drag to move window");
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            }
        }

        // Right: X
        ImGui::SameLine(0.0f, customTitlebar ? groupGap : spacing);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.18f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.86f, 0.24f, 0.24f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.12f, 0.12f, 1.0f));
        if (ImGui::Button("X##hint_exit", ImVec2(btnW, btnH)))
        {
            PostMessageW(ctx.window.handle(), WM_CLOSE, 0, 0);
        }
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Exit MagnifyShit");
    }
    ImGui::End();
}

} // namespace

void MainOverlay::draw(UiContext& ctx)
{
    const Size cs = ctx.window.clientSize();

    // transient toast banner (e.g. "Copied #RRGGBB"). Anchored bottom-center.
    if (!ctx.state.toastText.empty() && ImGui::GetTime() < ctx.state.toastUntilTime)
    {
        const double remain = ctx.state.toastUntilTime - ImGui::GetTime();
        const float alpha = static_cast<float>(std::min(1.0, remain / 0.4));
        ImGui::SetNextWindowBgAlpha(0.85f * alpha);
        ImGui::SetNextWindowPos(ImVec2(cs.w * 0.5f, cs.h - 24.0f),
                                ImGuiCond_Always,
                                ImVec2(0.5f, 1.0f));
        const ImGuiWindowFlags tFlags =
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 8));
        if (ImGui::Begin("##magshit_toast", nullptr, tFlags))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, alpha));
            ImGui::TextUnformatted(ctx.state.toastText.c_str());
            ImGui::PopStyleColor();
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
    else if (!ctx.state.toastText.empty())
    {
        ctx.state.toastText.clear();
    }

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

    if (ctx.window.borderless())
    {
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        const float x1 = static_cast<float>(cs.w);
        const float y1 = static_cast<float>(cs.h);
        const ImU32 col = IM_COL32(220, 220, 220, 180);
        const float step = 4.0f;
        const float th = 1.5f;
        for (int i = 1; i <= 4; ++i)
        {
            const float off = i * step;
            fg->AddLine(ImVec2(x1 - off, y1 - 2.0f), ImVec2(x1 - 2.0f, y1 - off), col, th);
        }

        // highlight edge / corner under the cursor
        const int ht = ctx.window.resizeHover();
        if (ht != HTNOWHERE)
        {
            ImDrawList* dl = ImGui::GetForegroundDrawList();
            const ImU32 hi = IM_COL32(120, 170, 240, 230);
            const float w = static_cast<float>(cs.w);
            const float h = static_cast<float>(cs.h);
            const float th2 = 2.5f;
            const float corner = 24.0f;
            const bool left = ht == HTLEFT || ht == HTTOPLEFT || ht == HTBOTTOMLEFT;
            const bool right = ht == HTRIGHT || ht == HTTOPRIGHT || ht == HTBOTTOMRIGHT;
            const bool top = ht == HTTOP || ht == HTTOPLEFT || ht == HTTOPRIGHT;
            const bool bottom = ht == HTBOTTOM || ht == HTBOTTOMLEFT || ht == HTBOTTOMRIGHT;
            const bool isCorner = (left || right) && (top || bottom);
            if (isCorner)
            {
                // 2 short legs meeting at the hovered corner.
                const float cx = left ? 0.5f : w - 0.5f;
                const float cy = top ? 0.5f : h - 0.5f;
                const float lx = left ? corner : -corner;
                const float ly = top ? corner : -corner;
                dl->AddLine(ImVec2(cx, cy), ImVec2(cx + lx, cy), hi, th2);
                dl->AddLine(ImVec2(cx, cy), ImVec2(cx, cy + ly), hi, th2);
            }
            else
            {
                if (left)
                    dl->AddLine(ImVec2(0.5f, 0), ImVec2(0.5f, h), hi, th2);
                if (right)
                    dl->AddLine(ImVec2(w - 0.5f, 0), ImVec2(w - 0.5f, h), hi, th2);
                if (top)
                    dl->AddLine(ImVec2(0, 0.5f), ImVec2(w, 0.5f), hi, th2);
                if (bottom)
                    dl->AddLine(ImVec2(0, h - 0.5f), ImVec2(w, h - 0.5f), hi, th2);
            }
        }
    }

    if (!ctx.state.showOverlay)
    {
        drawShowHint(ctx, cs);
        return;
    }

    const float scale = ImGui::GetIO().FontGlobalScale > 0 ? 1.0f : 1.0f;
    const ImVec2 pad{12 * scale, 12 * scale};
    const ImVec2 size{640 * scale, 0};
    ImGui::SetNextWindowPos(ImVec2(cs.w - size.x - pad.x, pad.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w);

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin("##magshit_overlay", nullptr, flags))
    {
        HWND const hwnd = ctx.window.handle();
        const bool customTitlebar = ctx.window.borderless();

        if (customTitlebar)
        {
            // Custom titlebar: [view-toggles] [drag bar] [window controls]
            const float btnH = ImGui::GetFrameHeight();
            const float btnW = btnH; // square icon buttons
            const float spacing = 2.0f * scale;
            const float groupGap = 8.0f * scale;
            const auto& toggleBinds =
                ctx.state.hotkeys.bindings[static_cast<size_t>(app::HotkeyAction::ToggleOverlay)];
            std::string toggleKey =
                toggleBinds.empty() ? std::string("F1") : app::bindingToString(toggleBinds.front());

            const int nLeft = 3;  // pin, boundary, panel-toggle
            const int nRight = 3; // minimize, maximize, exit
            const float leftW = nLeft * btnW + (nLeft - 1) * spacing;
            const float rightW = nRight * btnW + (nRight - 1) * spacing;
            const float avail = ImGui::GetContentRegionAvail().x;
            const float dragW = avail - leftW - rightW - 2 * groupGap;

            // -------- LEFT: app/view toggles (accent-tinted) --------
            const ImVec4 accentBtn(0.20f, 0.32f, 0.55f, 1.0f);
            const ImVec4 accentBtnHovered(0.28f, 0.42f, 0.68f, 1.0f);
            const ImVec4 accentBtnActive(0.34f, 0.50f, 0.78f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, accentBtnHovered);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, accentBtnActive);

            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ctx.state.alwaysOnTop ? accentBtnActive : accentBtn);
            if (ImGui::Button(ctx.state.alwaysOnTop ? "*" : "o", ImVec2(btnW, btnH)))
            {
                ctx.state.alwaysOnTop = !ctx.state.alwaysOnTop;
                ctx.window.setAlwaysOnTop(ctx.state.alwaysOnTop);
            }
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Always on top");

            ImGui::SameLine(0.0f, spacing);
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ctx.state.showBoundary ? accentBtnActive : accentBtn);
            if (ImGui::Button("[ ]", ImVec2(btnW, btnH)))
            {
                ctx.state.showBoundary = !ctx.state.showBoundary;
            }
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Toggle window boundary outline");

            ImGui::SameLine(0.0f, spacing);
            ImGui::PushStyleColor(ImGuiCol_Button, accentBtn);
            if (ImGui::Button("v##panel_toggle", ImVec2(btnW, btnH)))
            {
                ctx.state.showOverlay = !ctx.state.showOverlay;
            }
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Hide control panel (%s).\nMagnifier keeps running; X to quit.",
                                  toggleKey.c_str());

            ImGui::PopStyleColor(2);

            // -------- MIDDLE: drag region --------
            ImGui::SameLine(0.0f, groupGap);
            const ImVec2 dragPos = ImGui::GetCursorScreenPos();
            const float dragH = btnH;
            ImGui::InvisibleButton("##titlebar_drag", ImVec2(dragW, dragH));
            const bool dragHovered = ImGui::IsItemHovered();
            const bool dragHeld = ImGui::IsItemActive();
            if (ImGui::IsItemActivated())
            {
                ReleaseCapture();
                PostMessageW(hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
            }
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 dragEnd(dragPos.x + dragW, dragPos.y + dragH);
            const ImU32 bgCol = ImGui::GetColorU32(dragHeld      ? ImGuiCol_TitleBgActive
                                                   : dragHovered ? ImGuiCol_HeaderHovered
                                                                 : ImGuiCol_TitleBg);
            dl->AddRectFilled(dragPos, dragEnd, bgCol, 6.0f);
            const ImU32 borderCol =
                ImGui::GetColorU32(dragHovered ? ImGuiCol_SeparatorHovered : ImGuiCol_Border);
            dl->AddRect(dragPos, dragEnd, borderCol, 6.0f, 0, 1.0f);
            const ImU32 gripCol =
                ImGui::GetColorU32(dragHovered ? ImGuiCol_Text : ImGuiCol_TextDisabled);
            const float gripCols = 4.0f;
            const float gripStep = 4.0f;
            const float gripLeft = dragPos.x + 8.0f;
            const float gripTop = dragPos.y + 5.0f;
            const float gripBottom = dragEnd.y - 5.0f;
            for (int i = 0; i < (int)gripCols; ++i)
            {
                const float gx = gripLeft + i * gripStep;
                dl->AddLine(ImVec2(gx, gripTop), ImVec2(gx, gripBottom), gripCol, 1.4f);
            }
            const float gripRight = gripLeft + (gripCols - 1) * gripStep;
            const ImU32 titleCol = ImGui::GetColorU32(ImGuiCol_Text);
            const ImU32 dimCol = ImGui::GetColorU32(ImGuiCol_TextDisabled);
            char status[128];
            std::snprintf(status,
                          sizeof(status),
                          "  %s  %.2fx",
                          modeLabel(ctx.state.mode),
                          ctx.state.zoom);
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

            // -------- RIGHT: window controls (neutral, exit red) --------
            ImGui::SameLine(0.0f, groupGap);
            const ImVec4 chromeBtn(0.22f, 0.22f, 0.24f, 1.0f);
            const ImVec4 chromeBtnHovered(0.32f, 0.32f, 0.34f, 1.0f);
            const ImVec4 chromeBtnActive(0.40f, 0.40f, 0.42f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, chromeBtn);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, chromeBtnHovered);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, chromeBtnActive);

            if (ImGui::Button("_##min", ImVec2(btnW, btnH)))
            {
                ShowWindow(hwnd, SW_MINIMIZE);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Minimize");

            ImGui::SameLine(0.0f, spacing);
            const bool zoomed = IsZoomed(hwnd) != 0;
            if (ImGui::Button(zoomed ? "=##max" : "[]##max", ImVec2(btnW, btnH)))
            {
                ShowWindow(hwnd, zoomed ? SW_RESTORE : SW_MAXIMIZE);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(zoomed ? "Restore" : "Maximize");
            ImGui::PopStyleColor(3); // chrome group

            ImGui::SameLine(0.0f, spacing);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.18f, 0.18f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.86f, 0.24f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.12f, 0.12f, 1.0f));
            if (ImGui::Button("X##exit", ImVec2(btnW, btnH)))
            {
                PostMessageW(hwnd, WM_CLOSE, 0, 0);
            }
            ImGui::PopStyleColor(3);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Exit MagnifyShit");

            ImGui::Separator();
        } // customTitlebar
        else
        {
            ImGui::TextUnformatted("MagnifyShit 2.0");
            ImGui::SameLine();
            ImGui::TextDisabled(" | %s  %.2fx", modeLabel(ctx.state.mode), ctx.state.zoom);
            ImGui::SameLine(ImGui::GetWindowWidth() - 28 * scale);
            if (ImGui::SmallButton("X"))
            {
                ctx.state.showOverlay = false;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Hide control panel (F1).\nThe magnifier keeps running.");
            ImGui::Separator();
        }

        ImGui::SetNextItemWidth(-120 * scale);
        ImGui::SliderFloat("##zoom",
                           &ctx.state.zoom,
                           ctx.state.zoomMin,
                           ctx.state.zoomMax,
                           "%.2fx");
        ImGui::SameLine();
        ImGui::TextUnformatted("Zoom");
        helpMarker("Magnification factor. Ctrl + scroll wheel or Ctrl +/- also work; Ctrl+0 resets "
                   "to 1x.");

        const char* modes[] = {"Static (behind window)", "Follow cursor", "Attached to cursor"};
        int idx = static_cast<int>(ctx.state.mode);
        ImGui::SetNextItemWidth(-120 * scale);
        if (ImGui::Combo("##mode", &idx, modes, IM_ARRAYSIZE(modes)))
        {
            ctx.state.mode = static_cast<app::MagnifierMode>(idx);
            if (ctx.state.mode == app::MagnifierMode::AttachToMouse)
                ctx.state.showOverlay = false;
        }
        ImGui::SameLine();
        ImGui::TextUnformatted("Mode");
        helpMarker("Static: zoom the area behind the window.\n"
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
        helpMarker("Nearest: pixel-perfect, hard edges.\n"
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
        helpMarker("Mouse input lands on whatever is beneath the magnified area at the "
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
            const auto& copyBinds =
                ctx.state.hotkeys.bindings[static_cast<size_t>(app::HotkeyAction::CopyHexAtCursor)];
            const std::string copyHint = copyBinds.empty()
                                             ? std::string("(unbound)")
                                             : app::bindingToString(copyBinds.front());
            const auto& c = ctx.eyedropper->last();
            if (c.has_value())
            {
                const ImVec4 col(c->r / 255.0f, c->g / 255.0f, c->b / 255.0f, 1.0f);
                ImGui::ColorButton("##swatch",
                                   col,
                                   ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoLabel,
                                   ImVec2(28 * scale, 18 * scale));
                ImGui::SameLine();
                char hex[16];
                std::snprintf(hex, sizeof(hex), "#%02X%02X%02X", c->r, c->g, c->b);
                ImGui::Text("%s  rgb(%u,%u,%u)", hex, c->r, c->g, c->b);
                ImGui::SameLine();
                if (ImGui::SmallButton("Copy##eye_copy"))
                {
                    if (OpenClipboard(ctx.window.handle()))
                    {
                        EmptyClipboard();
                        const size_t bytes = std::strlen(hex) + 1;
                        if (HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, bytes); h != nullptr)
                        {
                            if (void* p = GlobalLock(h))
                            {
                                std::memcpy(p, hex, bytes);
                                GlobalUnlock(h);
                                if (SetClipboardData(CF_TEXT, h))
                                {
                                    ctx.state.toastText = std::string("Copied ") + hex;
                                    ctx.state.toastUntilTime = ImGui::GetTime() + 1.8;
                                }
                            }
                            else
                            {
                                GlobalFree(h);
                            }
                        }
                        CloseClipboard();
                    }
                }
                ImGui::TextDisabled("Shortcut: %s (works anywhere)", copyHint.c_str());
            }
            else
            {
                ImGui::TextDisabled("Hover the desktop to sample a pixel.");
                ImGui::TextDisabled("Shortcut: %s (works anywhere)", copyHint.c_str());
            }
        }

        ImGui::Separator();
        if (ImGui::CollapsingHeader("Shortcuts"))
        {
            if (ImGui::BeginTable("##sc",
                                  2,
                                  ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH |
                                      ImGuiTableFlags_SizingFixedFit))
            {
                ImGui::TableSetupColumn("Keys", ImGuiTableColumnFlags_WidthFixed, 170.0f * scale);
                ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 0.0f);
                auto row = [](const char* k, const char* v) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextDisabled("%s", k);
                    ImGui::TableNextColumn();
                    ImGui::TextWrapped("%s", v);
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
                            if (i)
                                keys += "  /  ";
                            keys += app::bindingToString(binds[i]);
                        }
                    }
                    row(keys.c_str(), desc);
                };
                actionRow(app::HotkeyAction::ToggleOverlay, "Toggle this panel");
                actionRow(app::HotkeyAction::FreezeToggle, "Freeze / unfreeze capture");
                row("Esc", "Step back: click-through > Attach > borderless");
                actionRow(app::HotkeyAction::ZoomIn, "Zoom in");
                actionRow(app::HotkeyAction::ZoomOut, "Zoom out");
                actionRow(app::HotkeyAction::ResetZoom, "Reset zoom to 1x");
                actionRow(app::HotkeyAction::CycleMode, "Cycle Static -> Follow -> Attached");
                actionRow(app::HotkeyAction::AttachToggle, "Toggle Attached-to-cursor");
                actionRow(app::HotkeyAction::ToggleBorderless, "Toggle borderless");
                actionRow(app::HotkeyAction::AlwaysOnTopToggle, "Toggle always-on-top");
                actionRow(app::HotkeyAction::ClickThroughToggle, "Toggle click-through (global)");
                actionRow(app::HotkeyAction::Quit, "Quit (global escape)");
                actionRow(app::HotkeyAction::EyedropperToggle, "Toggle eyedropper");
                actionRow(app::HotkeyAction::CopyHexAtCursor,
                          "Copy hex of pixel under cursor (global)");
                actionRow(app::HotkeyAction::Screenshot, "Screenshot (file + clipboard)");
                actionRow(app::HotkeyAction::OpenSettings, "Open settings");
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
