//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <cassert>
#include <cstdint>

#if WIN32
#ifdef TRI_DLL_EXPORT
#define TRI_API __declspec(dllexport)
#else
#define TRI_API __declspec(dllimport)
#endif
#else
#define TRI_API
#endif

#include "Log.h"

#define TRI_STR(x) #x
#define TRI_STR_MACRO(x) TRI_STR(x)

#define TRI_VERSION_MAJOR 0
#define TRI_VERSION_MINOR 0
#define TRI_VERSION_PATH 1
#define TRI_VERSION_SUFFIX "-dev"
#define TRI_VERSION TRI_STR_MACRO(TRI_VERSION_MAJOR) "." TRI_STR_MACRO(TRI_VERSION_MINOR) "." TRI_STR_MACRO(TRI_VERSION_PATH) TRI_VERSION_SUFFIX

#if !defined(TRI_DEBUG) && !defined(TRI_RELEASE)
#define TRI_RELEASE
#endif

#ifdef TRI_DEBUG
#define TRI_ASSERT(expr, msg) if(!(expr)){tridot::Log::critical(msg);} assert((expr) && (msg));
#else
#define TRI_ASSERT(expr, msg)
#endif

#define TRI_UNIQUE_NAME_3(name, line, number) name##line##number
#define TRI_UNIQUE_NAME_2(name, line, number) TRI_UNIQUE_NAME_3(name, line, number)
#define TRI_UNIQUE_NAME(name) TRI_UNIQUE_NAME_2(name, __LINE__, __COUNTER__)

namespace tridot{

    const uint32_t poolPageSizeBits = 8;
    typedef uint32_t EntityId;
    typedef uint64_t SignatureBitMap;

}

