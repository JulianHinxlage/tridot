//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_CONFIG_H
#define TRIDOT_CONFIG_H

#include <cassert>
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

#endif //TRIDOT_CONFIG_H
