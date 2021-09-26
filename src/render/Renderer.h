//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "core/util/Ref.h"
#include "FrameBuffer.h"
#include "Mesh.h"
#include "Texture.h"
#include "Material.h"
#include "RenderPipeline.h"
#include "Light.h"

namespace tri {

    class Renderer : public System {
    public:
        int drawCallCount = 0;
        int instanceCount = 0;
        int meshCount = 0;
        int materialCount = 0;
        int shaderCount = 0;
        int lightCount = 0;
        int cameraCount = 0;

        void beginScene(glm::mat4 projectionMatrix, glm::vec3 eyePosition);
        void submit(const glm::mat4& transform, const glm::vec3& position, Mesh* mesh = nullptr, Material* material = nullptr, Color color = Color::white, uint32_t id = -1);
        void submit(const glm::vec3 &positionOrDirection, const Light &light);
        void drawScene(Ref<FrameBuffer> frameBuffer = nullptr, Ref<RenderPipeline> pipeline = nullptr);
        void resetScene();

        void startup() override;
        void update() override;
        void shutdown() override;

    private:
        Ref<Mesh> quad;
        Ref<Shader> defaultShader;
        Ref<Texture> defaultTexture;
        Ref<Material> defaultMaterial;
        Ref<RenderPipeline> defaultPipeline;
        class DrawList;
        Ref<DrawList> drawList;
    };

}
