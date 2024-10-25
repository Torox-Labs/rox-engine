#include "RoxPlatformFactory.h"

#ifdef _WIN32
#include "RoxWindowsAdapter.h"
#endif

#ifdef __ANDROID__
#include "RoxAndroidAdapter.h"
#endif

namespace RoxApp
{
	IRoxPlatformAdapter* createPlatformAdapter()
	{
#ifdef _WIN32
		return new RoxWindowsAdapter();
#elif defined(__ANDROID__)
    return new RoxAndroidAdapter();
#else
    return nullptr;
#endif
	}
}
