#pragma once
#include <iostream>
#include <sstream>
// Bucket List,
/*
LogOutput set for multiples platform and console
Batch writing,
Logging set up
*/
/*
#define LOG_WARNING_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_INFO_ENABLED 1

// If release we should not use the
#define LOG_ERROR_ENABLED 1
#define LOG_CRITICAL_ENABLED 1
*/
namespace cico
{
    namespace logging
    {

        // Trace stuff ?
        enum  LogLevel  
        {
            INFO = 0,
            DEBUG,
            WARNING,
            ERROR,
            CRITICAL,
        };

        bool initialize();
        bool shutdown();

        void output(LogLevel level, const char *message, ...);

        // Variadic template for handling multiple arguments
        template <typename T, typename... Args>
        void formatLog(std::ostringstream &oss, LogLevel level, T &&first, Args &&...rest)
        {
            // Append the first argument
            oss << std::forward<T>(first);
            if constexpr (sizeof...(rest) > 0)
            {
                oss << ' ';
                formatLog(oss, level, std::forward<Args>(rest)...);
            }
        }

        template <typename... Args>
        void _output(LogLevel level, Args &&...rest)
        {
            static constexpr const char *levelMessages[5] = {"INFO:", "DEBUG:", "WARNING:", "ERROR:", "CRITICAL:"};
            // Create a string stream to buffer the log message
            std::ostringstream oss;

            formatLog(oss, level, std::forward<Args>(rest)...);

            std::cout << levelMessages[level] << oss.str() << std::endl;
        }

    }
}
// Define macro that output them ?

#define CINFO(message, ...) ::cico::logging::output(cico::logging::LogLevel::INFO, message, __VA_ARGS__)
#define CDEBUG(message, ...) ::cico::logging::output(cico::logging::LogLevel::DEBUG, message, __VA_ARGS__)
#define CWARNING(message, ...) ::cico::logging::output(cico::logging::LogLevel::WARNING, message, __VA_ARGS__)
#define CERROR(message, ...) ::cico::logging::output(cico::logging::LogLevel::ERROR, message, __VA_ARGS__)
#define CCRITICAL(message, ...) ::cico::logging::output(cico::logging::LogLevel::CRITICAL, message, __VA_ARGS__)

// In a  more C++ fashion, sound more generic

#define _CINFO(...) ::cico::logging::_output(cico::logging::LogLevel::INFO, __VA_ARGS__)
#define _CDEBUG(...) ::cico::logging::_output(cico::logging::LogLevel::DEBUG, __VA_ARGS__)
#define _CWARNING(...) ::cico::logging::_output(cico::logging::LogLevel::WARNING, __VA_ARGS__)
#define _CERROR(...) ::cico::logging::_output(cico::logging::LogLevel::ERROR, __VA_ARGS__)
#define _CCRITICAL(...) ::cico::logging::_output(cico::logging::LogLevel::CRITICAL, __VA_ARGS__)

/*
Would if constexpr be ebtter
Logfilename, With an Output Directory(Would need a platform layer)

if #Defined ndefien
*/
