#include "Logger.h"
#include <cstdarg>
#include <cstring>
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

        
        void output(LogLevel level, const char *message, ...)
        {
            va_list args;
            va_start(args, message);
            //vsnprintf_s in VSVC
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
