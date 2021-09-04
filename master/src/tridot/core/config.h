//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <cassert>
#include <cstdint>
#include <string>

#if WIN32
#include <windows.h>
#undef ERROR
#undef min
#undef max
#undef near
#undef far
#endif

#if WIN32
#ifdef TRI_DLL_EXPORT
#define TRI_API __declspec(dllexport)
#else
#define TRI_API __declspec(dllimport)
#endif
#else
#define TRI_API
#endif

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

namespace tridot::impl{
    void assertLog(const std::string &message);
}
#ifdef TRI_DEBUG
#define TRI_ASSERT(expr, msg) if(!(expr)){tridot::impl::assertLog(msg);}assert((expr) && (msg));
#else
#define TRI_ASSERT(expr, msg)
#endif

#ifdef WIN32
#define TRI_ENABLE_AUDIO 0
#else
#define TRI_ENABLE_AUDIO 1
#endif

namespace tridot{

    const uint32_t poolPageSizeBits = 8;
    typedef uint32_t EntityId;
    typedef uint64_t SignatureBitMap;

}

