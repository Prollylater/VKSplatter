#pragma once
#include <iostream>
#include <sstream>

// Bucket List,
/*
Introduce FileSystem + Platform to handle logging output directory
LogOutput set for multiples platform and console
Batch writing,

//Log can be used in an Assert as shown in #03
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


        static constexpr const char *levelMessages[5] = {"INFO: ", "DEBUG: ", "WARNING: ", "ERROR: ", "CRITICAL: "};
        static constexpr size_t bufferSize = 1024;
        //Todo: bad idea long term wise
        //Or put it in Cpp after properly constraining template
        inline char buffer[bufferSize];
        inline size_t bufferPos = 0;

        // Trace stuff ?
        enum LogLevel
        {
            INFO = 0,
            DEBUG,
            WARNING,
            ERROR,
            CRITICAL,
        };

        bool initialize(const std::string &filename);
        // Default value is mainly for test purpose
        void flushBuffer(bool immediate = false);
        bool shutdown();

        void output(LogLevel level, const char *message, ...);

        template <typename T>
        void appendToBuffer(char *buf, size_t &pos, T &&value)
        {
            int written = 0;

            //Might actualy just put "char"
            if constexpr (std::is_same_v<std::decay_t<T>, const char *> ||
                          std::is_same_v<std::decay_t<T>, char *>)
            {
                written = snprintf(buf + pos, bufferSize - pos, "%s", value);
            }
            else if constexpr (std::is_same_v<std::decay_t<T>, std::string>)
            {
                written = snprintf(buf + pos, bufferSize - pos, "%s", value.c_str());
            }
            else if constexpr (std::is_arithmetic_v<std::decay_t<T>>)
            {
                auto str = std::to_string(value);
                written = snprintf(buf + pos, bufferSize - pos, "%s", str.c_str());
            }
            else
            {
                static_assert(sizeof(T) == 0, "Unsupported type for logging");
            }

            if (written > 0)
            {
                pos += static_cast<size_t>(written);
                std::cout << pos << std::endl;
            }
        }

        // Variadic template for handling multiple arguments
        template <typename T, typename... Args>
        void formatLog(char *buffer, size_t &pos, T &&first, Args &&...rest)
        {
            appendToBuffer(buffer, pos, std::forward<T>(first));

            if constexpr (sizeof...(rest) > 0)
            {
                appendToBuffer(buffer, pos, " ");
                formatLog(buffer, pos, std::forward<Args>(rest)...);
            }
        }

        template <typename... Args>
        void _output(LogLevel level, Args &&...rest)
        {
            size_t startPos = bufferPos; //Temp
            appendToBuffer(buffer, bufferPos, levelMessages[level]);
            formatLog(buffer, bufferPos, std::forward<Args>(rest)...);
            appendToBuffer(buffer, bufferPos, "\n");

            //Temp
            std::cout.write(buffer + startPos, bufferPos-startPos);
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
