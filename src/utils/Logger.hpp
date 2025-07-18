#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <cstdarg>
#include <cstdio>

#define RED "\033[31m"
#define GREEN "\033[32m"
#define LIGHTMAGENTA "\033[95m"
#define YELLOW "\033[33m"
#define CONSOLE_OUTPUT 1

class Logger
{
public:
    static void logMsg(const char *color, int output, const char *format, ...)
    {
        va_list args;
        va_start(args, format);
        if (output == CONSOLE_OUTPUT)
        {
            // Use std::cerr and vfprintf to avoid mixing iostream and stdio
            std::cerr << color;
            vfprintf(stderr, format, args);
            std::cerr << "\033[0m" << std::endl;
        }
        va_end(args);
    }
};

#endif // LOGGER_HPP