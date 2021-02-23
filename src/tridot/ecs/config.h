//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_ECS_CONFIG_H
#define TRIDOT_ECS_CONFIG_H

#include <cassert>

#if !defined(ECS_DEBUG) && !defined(ECS_RELEASE)
    #define ECS_RELEASE
#endif

#ifdef ECS_DEBUG
    #define ECS_ASSERT(expr, msg) assert((expr) && (msg));
#else
    #define ECS_ASSERT(expr, msg)
#endif

namespace ecs{

    const uint32_t poolPageSizeBits = 8;
    typedef uint32_t EntityId;

}

#endif //TRIDOT_ECS_CONFIG_H
