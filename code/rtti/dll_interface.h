#pragma once

#ifdef BX_DLL_rtti
#define RTTI_EXPORT __declspec(dllexport)
#else
#define RTTI_EXPORT __declspec(dllimport)
#endif
