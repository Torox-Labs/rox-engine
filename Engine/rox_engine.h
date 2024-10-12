// rox_engine.h
#pragma once

#ifdef ROX_ENGINE_EXPORTS
#define ROX_ENGINE_API __declspec(dllexport)
#else
#define ROX_ENGINE_API __declspec(dllimport)
#endif

ROX_ENGINE_API void rox_engine_function();