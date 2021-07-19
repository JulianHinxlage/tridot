//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "tridot/render/Shader.h"
#include "tridot/render/FrameBuffer.h"

namespace tridot {

    class PostProcessingEffect {
    public:
        Ref<Shader> shader;
        Ref<FrameBuffer> frameBuffer;

        PostProcessingEffect();
    };

}

