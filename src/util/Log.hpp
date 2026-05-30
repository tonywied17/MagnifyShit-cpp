#pragma once

#include <format>
#include <string_view>

namespace magshit::log {

/// @brief Severity for a single log line.
enum class Level
{
    Trace,
    Info,
    Warn,
    Error
};

/**
 * @brief Write a pre-formatted log message.
 * @param level Severity for the log line.
 * @param message Message text written to debug output and, in Debug builds, stderr.
 */
void write(Level level, std::string_view message);

/**
 * @brief Format and write an informational log message.
 * @tparam Args Format argument types.
 * @param fmt Compile-time checked `std::format` string.
 * @param args Values consumed by `fmt`.
 */
template <typename... Args>
inline void info(std::format_string<Args...> fmt, Args&&... args)
{
    write(Level::Info, std::format(fmt, std::forward<Args>(args)...));
}

/**
 * @brief Format and write a warning log message.
 * @tparam Args Format argument types.
 * @param fmt Compile-time checked `std::format` string.
 * @param args Values consumed by `fmt`.
 */
template <typename... Args>
inline void warn(std::format_string<Args...> fmt, Args&&... args)
{
    write(Level::Warn, std::format(fmt, std::forward<Args>(args)...));
}

/**
 * @brief Format and write an error log message.
 * @tparam Args Format argument types.
 * @param fmt Compile-time checked `std::format` string.
 * @param args Values consumed by `fmt`.
 */
template <typename... Args>
inline void error(std::format_string<Args...> fmt, Args&&... args)
{
    write(Level::Error, std::format(fmt, std::forward<Args>(args)...));
}

/**
 * @brief Format and write a trace log message.
 * @tparam Args Format argument types.
 * @param fmt Compile-time checked `std::format` string.
 * @param args Values consumed by `fmt`.
 */
template <typename... Args>
inline void trace(std::format_string<Args...> fmt, Args&&... args)
{
    write(Level::Trace, std::format(fmt, std::forward<Args>(args)...));
}

} // namespace magshit::log
