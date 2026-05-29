#pragma once

namespace magshit::ui {

/// User-selected styling. `Auto` tracks the Windows AppsUseLightTheme
/// setting and resolves to either `Light` or `Dark` at apply time.
enum class ThemeMode
{
    Light,
    Dark,
    Auto,
};

/// Applies a flat / minimal ImGui style.
class Theme
{
public:
    /// Apply `mode` to the current ImGui context.
    static void apply(ThemeMode mode);

    /// Mode last passed to `apply` (may be `Auto`).
    static ThemeMode current() noexcept { return current_; }

    /// The concrete mode (`Light` or `Dark`) actually in effect, even if
    /// the user selected `Auto`.
    static ThemeMode effective() noexcept { return effective_; }

private:
    static void applyLight();
    static void applyDark();
    static bool systemPrefersDark();

    static inline ThemeMode current_ = ThemeMode::Light;
    static inline ThemeMode effective_ = ThemeMode::Light;
};

} // namespace magshit::ui
