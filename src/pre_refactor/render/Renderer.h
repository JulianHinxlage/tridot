//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/util/Ref.h"
#include "core/System.h"
#include "Light.h"
#include "RenderPass.h"
#include "renderer/RenderBatchingContext.h"

namespace tri {

    class Renderer : public System {
    public:
        void startup() override;
        void update() override;
        void shutdown() override;

        void setCamera(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix, const glm::vec3 &eyePosition, Ref<FrameBuffer> frameBuffer = nullptr);
        void setRenderPass(Ref<RenderPass> pass);
        void setEnvironMap(Ref<Texture> radianceMap, Ref<Texture> irradianceMap, float intensity);
        void submit(const glm::vec3& position, const glm::vec3 direction, Light& light);
        void submit(const glm::mat4& transform, const glm::vec3& position, Mesh* mesh = nullptr, Material* material = nullptr, Color color = Color::white, uint32_t id = -1);
        void submit(const glm::mat4& transform, const glm::vec3& position, Mesh* mesh = nullptr, Shader* shader = nullptr, Texture* texture = nullptr, Color color = Color::white, uint32_t id = -1);
        void submitDirect(const glm::mat4& transform, const glm::vec3& position, Mesh* mesh = nullptr, Shader* shader = nullptr, Texture* texture = nullptr, Color color = Color::white);

    private:
        Ref<Mesh> defaultMesh;
        Ref<Texture> defaultTexture;
        Ref<Shader> defaultShader;
        Ref<Material> defaultMaterial;
        Ref<FrameBuffer> frameBuffer;

        RenderBatchingContext* current;
        std::unordered_map<RenderPass*, Ref<RenderBatchingContext>> contexts;

        Ref<FrameBuffer> lightAccumulationBuffer;

        void updateShadowMaps(RenderBatchingContext* context);
        void updateDeferred();
    };

}
