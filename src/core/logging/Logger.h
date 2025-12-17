#pragma once
#include <iostream>
#include <sstream>
// Bucket List,
/*
Introduce FileSystem + Platform to handle logging output directory
LogOutput set for multiples platform and console
Batch writing,

//USe it in an Assert
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

        static constexpr const char *levelMessages[5] = {"INFO:", "DEBUG:", "WARNING:", "ERROR:", "CRITICAL:"};
        static constexpr char bufferSize = 1024;
        static char buffer[bufferSize];

        // Trace stuff ?
        enum LogLevel
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

        template <typename T>
        size_t appendToBuffer(char *buf, size_t &pos, T &&value)
        {
            int written = snprintf(buf + pos, bufferSize - pos, "%s", std::to_string(value).c_str());
            if (written > 0){
                pos += static_cast<size_t>(written);}
            return pos;
        }

        // Variadic template for handling multiple arguments
        template <typename T, typename... Args>
        void formatLog(char *buffer, size_t &pos, T &&first, Args &&...rest)
        {
            appendToBuffer(buffer, pos, std::forward<T>(first));
            if constexpr (sizeof...(rest) > 0)
            {
                appendToBuffer(buffer, pos, " ");
                formatLog(oss,  std::forward<Args>(rest)...);
            }
        }

        template <typename... Args>
        void _output(LogLevel level, Args &&...rest)
        {
            size_t pos = 0;

            appendToBuffer(buffer, pos, levelMessages[level]);
            appendToBuffer(buffer, pos, " ");
            formatLog(buffer, pos, std::forward<Args>(rest)...);

            std::cout.write(buffer, pos);
            std::cout << std::endl;
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
