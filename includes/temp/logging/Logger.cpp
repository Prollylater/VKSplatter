#include "Logger.h"

namespace cico
{

    namespace logging
    {
        // Todo: Comments
        bool initialize()
        {
            // Init potential mutltiple loggers,
            // Define log directory path
            // Create Logdirectiory
            // Create logfile

            // Thread stuff maybe
            // Setup time of logging ?
        }

        bool shutdown()
        {
            // Exit the prgoram with writing up thing still left to write, freeing rssources if any
        }

        // Apparently i need std::args ?

        #include <cstdarg>
        #include <cstring>
        //This output to console and that's all
        
        void output(LogLevel level, const char *message, ...)
        {
            static constexpr const char *levelMessages[5] = {"INFO:", "DEBUG:", "WARNING:", "ERROR:", "CRITICAL:"};
            // Temporarily allocate a large enough buffer for message construction
            char buffer[1024];

            va_list args;
            va_start(args, message);
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