#include "Log.hpp"

#include <Windows.h>

#include <cstdio>
#include <string>

namespace magshit::log {

namespace {

/**
 * @brief Convert a log level into its fixed text prefix.
 * @param lvl Severity to render.
 * @return Static prefix string for the severity.
 */
const char* prefix(Level lvl)
{
    switch (lvl)
    {
    case Level::Trace: return "[trace] ";
    case Level::Info:  return "[info]  ";
    case Level::Warn:  return "[warn]  ";
    case Level::Error: return "[error] ";
    }
    return "";
}

} // namespace

void write(Level level, std::string_view message)
{
    std::string line;
    line.reserve(message.size() + 16);
    line.append(prefix(level));
    line.append(message);
    line.push_back('\n');

    std::fputs(line.c_str(), stderr);
    OutputDebugStringA(line.c_str());
}

} // namespace magshit::log
