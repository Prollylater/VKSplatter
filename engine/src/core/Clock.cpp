#include "Clock.h"
namespace cico {

	//If we ever had a platform, this would be platform
    Clock::Clock()
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
}