//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_POSTPROCESSINGEFFECT_H
#define TRIDOT_POSTPROCESSINGEFFECT_H

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

#endif //TRIDOT_POSTPROCESSINGEFFECT_H
