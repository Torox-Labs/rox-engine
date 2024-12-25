// Updated by the Rox-engine
// Copyright © 2024 Torox Project
//
// This file is part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License: for non-commercial and commercial use under specific conditions.
// 2. Commercial License: for use on proprietary platforms.
// 
// For full licensing terms, please refer to the LICENSE file in the root directory of this project.

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
