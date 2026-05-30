#pragma once

#include "Application.hpp"

#include <filesystem>

namespace magshit::app {

/// @brief Loads and saves the AppState to/from a JSON file in %LOCALAPPDATA%.
class Config
{
public:
    /**
     * @brief Resolve the canonical configuration file path.
     * @return Path to `%LOCALAPPDATA%\\MagnifyShit\\config.json`; the parent
     * directory is created if it does not already exist.
     */
    static std::filesystem::path defaultPath();

    /**
     * @brief Load application state from disk.
     * @param path JSON configuration file to read.
     * @param outState Destination state populated on success.
     * @return true when a supported configuration was loaded; false when the
     * file is missing, invalid, or has an unsupported schema version.
     */
    static bool load(const std::filesystem::path& path, AppState& outState);

    /**
     * @brief Atomically write application state to disk.
     * @param path JSON configuration file to replace.
     * @param state State snapshot to serialize.
     * @return true on success, false on I/O or serialization failure.
     */
    static bool save(const std::filesystem::path& path, const AppState& state);
};

} // namespace magshit::app
