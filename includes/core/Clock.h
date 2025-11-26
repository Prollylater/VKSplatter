#pragma once

#include <chrono>
namespace cico {

    struct Clock
	{
        using  clockNamespace = std::chrono::high_resolution_clock ;
        using  ms = std::chrono::milliseconds;
        using  s = std::chrono::duration<float> ;

		Clock()
		{
			reset();
		}


		void Clock::reset()
		{
			mStartTime = clockNamespace::now();
		}

		float Clock::elapsed()
		{
			  return std::chrono::duration<float>(clockNamespace::now() - mStartTime).count();
		}

        float Clock::elapsedMs()
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(clockNamespace::now() - mStartTime).count();
		}

        private:
		std::chrono::time_point<clockNamespace> mStartTime;

	};

}
