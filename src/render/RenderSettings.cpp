//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "RenderSettings.h"

namespace tri {

    TRI_REGISTER_CALLBACK() {
        env->renderSettings = new RenderSettings();
        env->console->setVariable("shadows", &env->renderSettings->shadowsEnabled);
        env->console->setVariable("draw_list_sorting", &env->renderSettings->drawListSortingEnabled);
        env->console->setVariable("transparency_pass", &env->renderSettings->transparencyPassEnabled);
        env->console->setVariable("frustum_culling", &env->renderSettings->frustumCullingEnabled);
        env->console->setVariable("deferred_shading", &env->renderSettings->deferredShadingEnabled);
    }

}