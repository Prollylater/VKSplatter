#pragma once
#include <chrono>

namespace cico {

	//If we ever had a platform, this would be platform instead of std::chrono 
    struct Clock
	{
        using  clockNamespace = std::chrono::high_resolution_clock ;
        using  ms = std::chrono::milliseconds;
        using  s = std::chrono::duration<float> ;

		Clock();
		void reset();
		float elapsed();
		float elapsedMs();

        private:
		std::chrono::time_point<clockNamespace> mStartTime;

	};

}
