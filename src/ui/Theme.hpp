#pragma once

namespace magshit::ui {

/// @brief User-selected ImGui styling mode.
enum class ThemeMode
{
    Light,
    Dark,
    Auto,
};

/// @brief Applies the app's flat, minimal ImGui style.
class Theme
{
public:
    /**
     * @brief Apply a theme mode to the current ImGui context.
     * @param mode Requested theme mode; `Auto` resolves from Windows settings.
     */
    static void apply(ThemeMode mode);

    /**
     * @brief Query the requested theme mode.
     * @return Mode last passed to `apply()`, possibly `Auto`.
     */
    static ThemeMode current() noexcept { return current_; }

    /**
     * @brief Query the concrete theme currently in effect.
     * @return Resolved mode, always `Light` or `Dark`.
     */
    static ThemeMode effective() noexcept { return effective_; }

private:
    /**
     * @brief Apply the light ImGui palette and sizing values.
     */
    static void applyLight();

    /**
     * @brief Apply the dark ImGui palette and sizing values.
     */
    static void applyDark();

    /**
     * @brief Read the Windows app-theme preference.
     * @return true when Windows prefers dark app mode.
     */
    static bool systemPrefersDark();

    static inline ThemeMode current_ = ThemeMode::Light;
    static inline ThemeMode effective_ = ThemeMode::Light;
};

} // namespace magshit::ui
