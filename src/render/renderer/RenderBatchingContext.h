//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "RenderBatch.h"
#include "core/util/Ref.h"
#include "core/System.h"
#include "render/Light.h"
#include "render/RenderPass.h"

namespace tri {

    class RenderBatchingContext {
    public:
        Ref<RenderPass> pass;
        RenderBatchList batches;
        DrawList drawList;

        EnvironmentData environment;
        Ref<Texture> radianceMap;
        Ref<Texture> irradianceMap;
        Ref<BatchBuffer> environmentBuffer;

        Ref<FrameBuffer> frameBuffer;
        FrustumCulling frustum;

        class LightEntry {
        public:
            Light light;
            glm::vec3 position;
            glm::vec3 direction;
            glm::mat4 projection;
        };
        std::vector<LightEntry> lights;
        Ref<BatchBuffer> lightBuffer;

        bool hasShadowContexts;
        Ref<RenderBatchingContext> transparencyContext;
        std::vector<Ref<RenderBatchingContext>> shadowMapContexts;

        void init();
        void updateDrawList();
        void updateLights();
        void update();
        void add(const glm::mat4& transform, Mesh* mesh, Shader* shader, Material* material, Color color, uint32_t id);
    };

}
