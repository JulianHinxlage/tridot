//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_ECS_CONFIG_H
#define TRIDOT_ECS_CONFIG_H

#include <cassert>
#include <cstdint>

#if !defined(ECS_DEBUG) && !defined(ECS_RELEASE)
    #define ECS_RELEASE
#endif

#ifdef ECS_DEBUG
    #define ECS_ASSERT(expr, msg) if(!(expr)){ assert((expr) && (msg)); fprintf(stderr, "%s\n", msg); exit(-1);}
#else
    #define ECS_ASSERT(expr, msg)
#endif

namespace ecs{

    const uint32_t poolPageSizeBits = 8;
    typedef uint32_t EntityId;
    typedef uint64_t SignatureBitMap;

}

#endif //TRIDOT_ECS_CONFIG_H
