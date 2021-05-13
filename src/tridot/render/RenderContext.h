//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_RENDERCONTEXT_H
#define TRIDOT_RENDERCONTEXT_H

#include "tridot/core/config.h"

namespace tridot {

    class TRI_API RenderContext {
    public:
        static void *create();
        static void *get();
        static void set(void *context);
        static void destroy();

        static void setDepth(bool enabled);
        static void setBlend(bool enabled);
        static void setCull(bool enabled, bool front = false);
        static void flush(bool synchronous = false);
    private:
        static void *context;
    };

}

#endif //TRIDOT_RENDERCONTEXT_H
