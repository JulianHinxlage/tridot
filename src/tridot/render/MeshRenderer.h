//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "Mesh.h"
#include "Texture.h"
#include "Shader.h"
#include "FrameBuffer.h"
#include "MeshRendererBatch.h"

namespace tridot {

    class MeshRenderer {
    public:
        int maxBatchSize;

        MeshRenderer();

        class SubmitCall {
        public:
            glm::vec3 position = {0, 0, 0};
            glm::vec3 scale = {1, 1, 1};
            glm::vec3 rotation = {0, 0, 0};
            Color color = Color::white;
            glm::vec2 texCoordsTopLeft = {0, 0};
            glm::vec2 texCoordsBottomRight = {1, 1};
        };

        void init(const Ref<Shader> &shader, uint32_t maxBatchSize = 10000);
        void begin(const glm::mat4 &projection = glm::mat4(1), const glm::vec3 &cameraPosition = {0, 0, 0}, const Ref<FrameBuffer> &frameBuffer = nullptr);
        void submit(const SubmitCall &call, Texture *texture = nullptr, Mesh *mesh = nullptr, Shader *shader = nullptr);
        void end();
    private:
        Ref<Mesh> quadMesh;
        Ref<Texture> blankTexture;
        Ref<Shader> defaultShader;
        Ref<FrameBuffer> frameBuffer;
        glm::mat4 projection;
        glm::vec3 cameraPosition;
        std::vector<Ref<MeshRendererBatch>> batches;

        void flushBatch(MeshRendererBatch *batch);
    };

}

