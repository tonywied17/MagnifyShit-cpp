#include "Config.hpp"

#include "Hotkeys.hpp"
#include "../util/Log.hpp"

#include <ShlObj.h>
#include <json.hpp>

#include <fstream>

namespace magshit::app {

namespace {

constexpr int kSchemaVersion = 2;

/**
 * @brief Serialize hotkey bindings into a JSON object keyed by action id.
 * @param m Hotkey map to serialize.
 * @return JSON object containing each action's binding array.
 */
nlohmann::json hotkeysToJson(const HotkeyMap& m)
{
    nlohmann::json out = nlohmann::json::object();
    for (const auto& info : hotkeyActionInfos())
    {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& b : m.bindings[static_cast<size_t>(info.action)])
        {
            arr.push_back({
                {"mods", b.mods},
                {"trigger", static_cast<int>(b.trigger)},
                {"vk", b.vk},
            });
        }
        out[info.id] = std::move(arr);
    }
    return out;
}

/**
 * @brief Load hotkey bindings from a JSON object into an existing map.
 * @param j JSON object containing action binding arrays.
 * @param m Hotkey map updated in place for keys found in `j`.
 */
void hotkeysFromJson(const nlohmann::json& j, HotkeyMap& m)
{
    if (!j.is_object()) return;
    for (const auto& info : hotkeyActionInfos())
    {
        auto it = j.find(info.id);
        if (it == j.end() || !it->is_array()) continue;
        auto& vec = m.bindings[static_cast<size_t>(info.action)];
        vec.clear();
        for (const auto& e : *it)
        {
            HotkeyBinding b;
            b.mods    = static_cast<std::uint8_t>(e.value("mods", 0u));
            b.trigger = static_cast<HotkeyTrigger>(e.value("trigger", 0));
            b.vk      = static_cast<std::uint16_t>(e.value("vk", 0u));
            vec.push_back(b);
        }
    }
}

} // namespace

std::filesystem::path Config::defaultPath()
{
    PWSTR p = nullptr;
    std::filesystem::path dir;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &p)))
    {
        dir = p;
        CoTaskMemFree(p);
    }
    else
    {
        dir = std::filesystem::current_path();
    }
    dir /= L"MagnifyShit";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir / L"config.json";
}

bool Config::load(const std::filesystem::path& path, AppState& s)
{
    std::ifstream in(path);
    if (!in.is_open()) return false;

    nlohmann::json j;
    try
    {
        in >> j;
    }
    catch (...)
    {
        log::warn("Config: failed to parse JSON; ignoring file");
        return false;
    }

    if (j.value("version", 0) != kSchemaVersion) return false;

    s.mode             = static_cast<MagnifierMode>(j.value("mode", int(s.mode)));
    s.zoom             = j.value("zoom", s.zoom);
    s.zoomMin          = j.value("zoomMin", s.zoomMin);
    s.zoomMax          = j.value("zoomMax", s.zoomMax);
    s.scaling          = static_cast<render::Scaling>(j.value("scaling", int(s.scaling)));
    s.pixelPerfectSnap = j.value("pixelPerfectSnap", s.pixelPerfectSnap);
    s.invert           = j.value("invert", s.invert);
    s.grayscale        = j.value("grayscale", s.grayscale);
    s.cvd              = static_cast<render::CvdMode>(j.value("cvd", int(s.cvd)));
    s.brightness       = j.value("brightness", s.brightness);
    s.contrast         = j.value("contrast", s.contrast);
    s.gamma            = j.value("gamma", s.gamma);
    s.gridAuto         = j.value("gridAuto", s.gridAuto);
    s.gridThreshold    = j.value("gridThreshold", s.gridThreshold);
    s.gridOn           = j.value("gridOn", s.gridOn);
    s.gridOpacity      = j.value("gridOpacity", s.gridOpacity);
    s.alwaysOnTop      = j.value("alwaysOnTop", s.alwaysOnTop);
    s.clickThrough     = j.value("clickThrough", s.clickThrough);
    s.excludeFromCapture = j.value("excludeFromCapture", s.excludeFromCapture);
    s.showOverlay      = j.value("showOverlay", s.showOverlay);
    s.showBoundary     = j.value("showBoundary", s.showBoundary);
    s.themeMode        = static_cast<ui::ThemeMode>(j.value("themeMode", int(s.themeMode)));
    if (j.contains("hotkeys"))
    {
        hotkeysFromJson(j["hotkeys"], s.hotkeys);
    }
    return true;
}

bool Config::save(const std::filesystem::path& path, const AppState& s)
{
    nlohmann::json j;
    j["version"]          = kSchemaVersion;
    j["mode"]             = static_cast<int>(s.mode);
    j["zoom"]             = s.zoom;
    j["zoomMin"]          = s.zoomMin;
    j["zoomMax"]          = s.zoomMax;
    j["scaling"]          = static_cast<int>(s.scaling);
    j["pixelPerfectSnap"] = s.pixelPerfectSnap;
    j["invert"]           = s.invert;
    j["grayscale"]        = s.grayscale;
    j["cvd"]              = static_cast<int>(s.cvd);
    j["brightness"]       = s.brightness;
    j["contrast"]         = s.contrast;
    j["gamma"]            = s.gamma;
    j["gridAuto"]         = s.gridAuto;
    j["gridThreshold"]    = s.gridThreshold;
    j["gridOn"]           = s.gridOn;
    j["gridOpacity"]      = s.gridOpacity;
    j["alwaysOnTop"]        = s.alwaysOnTop;
    j["clickThrough"]       = s.clickThrough;
    j["excludeFromCapture"] = s.excludeFromCapture;
    j["showOverlay"]        = s.showOverlay;
    j["showBoundary"]       = s.showBoundary;
    j["themeMode"]        = static_cast<int>(s.themeMode);
    j["hotkeys"]          = hotkeysToJson(s.hotkeys);

    const auto tmp = path.wstring() + L".tmp";
    {
        std::ofstream out(tmp);
        if (!out.is_open())
        {
            log::warn("Config: cannot open {} for write", path.string());
            return false;
        }
        out << j.dump(2);
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec)
    {
        std::filesystem::remove(path, ec);
        std::filesystem::rename(tmp, path, ec);
    }
    return !ec;
}

} // namespace magshit::app
