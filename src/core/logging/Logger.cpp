#include "Logger.h"
#include "filesystem/Filesystem.h"
#include <cstdarg>
#include <fstream>

namespace
{
    // Could be a struct if we end up making more
    stdfs::path logDir;
    stdfs::path logFilePath;
    std::chrono::milliseconds PeriodicFlush = std::chrono::milliseconds(1);
    std::ofstream logFile;

}

namespace cico
{

    namespace logging
    {

        bool initialize(const std::string &filename)
        {
            // Todo: Not really a good idea
            logDir = cico::fs::root() / "logs";

            if (!stdfs::exists(logDir))
            {
                stdfs::create_directories(logDir);
            }
            logFilePath = logDir / filename;
            return true;
        }

        void flushBuffer(bool immediate)
        {
            //std::cout << "Inserted " << cico::logging::bufferPos << " character" << std::endl;
            if (immediate || bufferPos > 756)
            {
                logFile.open(logFilePath, std::ios::app);
                if (!logFile.is_open())
                {
                    throw std::runtime_error("Failed to open log file: " + logFilePath.string());
                }
                //Todo: other write method
                std::string text(buffer, bufferPos);
                //std::cout << text<<std::endl;
                logFile << text;

                logFile.flush();

                bufferPos = 0;
            }
        }

        bool shutdown()
        {
            // Exit the prgoram with writing up thing still left to write, freeing rssources if any
            flushBuffer(true);
            return true;
        }

        void output(LogLevel level, const char *message, ...)
        {
            va_list args;
            va_start(args, message);
            // vsnprintf_s in VSVC
            vsnprintf(buffer, sizeof(buffer), message, args);
            va_end(args);
            bool error = level > 2U;

            // Output the log message to console
            ::std::cout << levelMessages[level] << " " << buffer << ::std::endl;

            if (error)
            {
                ::std::cerr << "This is an error log." << ::std::endl;
            }
        }

    }
}
