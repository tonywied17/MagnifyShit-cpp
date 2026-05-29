#pragma once

#include "Application.hpp"

#include <filesystem>

namespace magshit::app {

/// Loads and saves the AppState to/from a JSON file in %LOCALAPPDATA%.
class Config
{
public:
    /// Returns the canonical config path
    /// (`%LOCALAPPDATA%\\MagnifyShit\\config.json`), creating the directory
    /// if it does not exist.
    static std::filesystem::path defaultPath();

    /// Reads `path` into `outState`. Returns false if the file is missing,
    /// unparseable, or has an unsupported schema version.
    static bool load(const std::filesystem::path& path, AppState& outState);

    /// Atomically writes `state` to `path`. Returns false on I/O failure.
    static bool save(const std::filesystem::path& path, const AppState& state);
};

} // namespace magshit::app
