//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "tridot/util/Ref.h"
#include "tridot/render/Texture.h"

namespace tridot {

    class SkyBox {
    public:
        Ref<Texture> texture;
        Ref<Texture> irradianceTexture;
        bool drawSkybox;
        bool useEnvironmentMap;
        float intensity;
        SkyBox();
    };

}

