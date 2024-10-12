// rox_engine.h
#pragma once

#ifdef _WIN32
#ifdef ROX_ENGINE_EXPORTS
#define ROX_ENGINE_API __declspec(dllexport)
#else
#define ROX_ENGINE_API __declspec(dllimport)
#endif
#elif defined(__GNU__)
#ifdef ROX_ENGINE_EXPORTS
#define ROX_ENGINE_API __attribute__((visibility("default"))
#else
#define ROX_ENGINE_API
#endif
#else
#define ROX_ENGINE_API
#endif

ROX_ENGINE_API void rox_engine_function();
