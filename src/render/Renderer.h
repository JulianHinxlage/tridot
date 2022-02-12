//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/util/Ref.h"
#include "core/System.h"
#include "FrameBuffer.h"
#include "Mesh.h"
#include "Texture.h"
#include "Material.h"
#include "Light.h"
#include "RenderPass.h"
#include "BatchBuffer.h"

namespace tri {

    class Renderer : public System{
    public:
        class Statistics {
        public:
            int drawCallCount = 0;
            int instanceCount = 0;
            int meshCount = 0;
            int materialCount = 0;
            int shaderCount = 0;
            int lightCount = 0;
            int cameraCount = 0;
        };
        Statistics stats;


        void startup() override;
        void update() override;
        void shutdown() override;


        void setCamera(glm::mat4& projection, glm::vec3 position, Ref<FrameBuffer> frameBuffer = nullptr);
        void setRenderPass(const Ref<RenderPass> &pass);
        void setEnvironMap(Ref<Texture> radianceMap, Ref<Texture> irradianceMap, float intensity);
        void submit(const glm::vec3& position, const glm::vec3 direction, Light& light);
        void submit(const glm::mat4& transform, const glm::vec3& position, Mesh* mesh = nullptr, Material* material = nullptr, Color color = Color::white, uint32_t id = -1);
        void submit(const glm::mat4& transform, const glm::vec3& position, Mesh* mesh = nullptr, Shader* shader = nullptr, Texture* texture = nullptr, Color color = Color::white, uint32_t id = -1);
        void submitDirect(const glm::mat4& transform, const glm::vec3& position, Mesh* mesh = nullptr, Shader* shader = nullptr, Texture *texture = nullptr, Color color = Color::white);

    private:
        Ref<RenderPass> geometryPass;
        Ref<RenderPass> shadowPass;
        Ref<RenderPass> currentPass;
        Ref<Mesh> defaultMesh;
        Ref<Texture> defaultTexture;
        Ref<Shader> defaultShader;
        Ref<Material> defaultMaterial;
        Ref<FrameBuffer> frameBuffer;

        class Impl;
        Ref<Impl> impl;

        void updateShadowMaps();
    };

}
