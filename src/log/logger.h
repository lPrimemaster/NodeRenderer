#pragma once
#include <iostream>

#define L_TRACE(msg, ...) logger::loglevel("TRACE", msg, __VA_ARGS__)
#define L_DEBUG(msg, ...) logger::loglevel("DEBUG", msg, __VA_ARGS__)
#define L_WARNING(msg, ...) logger::loglevel("WARNING", msg, __VA_ARGS__)
#define L_ERROR(msg, ...) logger::loglevel("ERROR", msg, __VA_ARGS__)

namespace logger
{
    void loglevel(const std::string& level, const char* msg, ...);
}
