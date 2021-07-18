//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_SKYBOX_H
#define TRIDOT_SKYBOX_H

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

#endif //TRIDOT_SKYBOX_H
