#pragma once

#include <format>
#include <string_view>

namespace magshit::log {

/// Severity for a single log line.
enum class Level
{
    Trace,
    Info,
    Warn,
    Error
};

/// Write a pre-formatted message at `level` to the debug output and (in
/// Debug builds) stderr.
void write(Level level, std::string_view message);

/// std::format-style helper for `Level::Info`.
template <typename... Args>
inline void info(std::format_string<Args...> fmt, Args&&... args)
{
    write(Level::Info, std::format(fmt, std::forward<Args>(args)...));
}

/// std::format-style helper for `Level::Warn`.
template <typename... Args>
inline void warn(std::format_string<Args...> fmt, Args&&... args)
{
    write(Level::Warn, std::format(fmt, std::forward<Args>(args)...));
}

/// std::format-style helper for `Level::Error`.
template <typename... Args>
inline void error(std::format_string<Args...> fmt, Args&&... args)
{
    write(Level::Error, std::format(fmt, std::forward<Args>(args)...));
}

/// std::format-style helper for `Level::Trace`.
template <typename... Args>
inline void trace(std::format_string<Args...> fmt, Args&&... args)
{
    write(Level::Trace, std::format(fmt, std::forward<Args>(args)...));
}

} // namespace magshit::log
