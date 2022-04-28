#include "logger.h"
#include <stdarg.h>

// BUG: This can be an issue
static struct LogFileGuard
{
    LogFileGuard()
    {
        f = fopen("latest.log", "w");
    }

    ~LogFileGuard()
    {
        if(f != nullptr)
        {
            fclose(f);
        }
    }

    FILE* f = nullptr;
} _logfile_guard;

static FILE* f = nullptr;
static bool first_init = true;

void logger::loglevel(const std::string& level, const char* msg, ...)
{
    size_t size = strlen(msg);
    if (size < 2048)
    {
        char buffer[2048];
        va_list args;
        va_start(args, msg);
        vsprintf(buffer, msg, args);
        va_end(args);

        std::string data = "[" + level + "]: " + buffer + "\n";

        // Write to console
        printf(("[*]" + data).c_str());

        // Write to file
        fwrite(data.c_str(), 1, data.size(), _logfile_guard.f);
        fflush(f); // ?
    }
}
