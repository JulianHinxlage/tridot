//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_PBRENDERER_H
#define TRIDOT_PBRENDERER_H

#include "Mesh.h"
#include "Texture.h"
#include "Shader.h"
#include "FrameBuffer.h"
#include "MeshRendererBatch.h"
#include "Material.h"
#include "BatchBuffer.h"
#include "Light.h"

namespace tridot {

    class PBRenderer {
    public:
        uint32_t maxInstances;
        uint32_t maxTextures;
        uint32_t maxMaterials;

        Ref<Material> defaultMaterial;

        PBRenderer();

        void init(const Ref<Shader> &shader, uint32_t maxTextures = 32, uint32_t maxMaterials = 1024, uint32_t maxInstances = 100000);
        void begin(const glm::mat4 &projection = glm::mat4(1), const glm::vec3 &cameraPosition = glm::vec3(0, 0, 0), const Ref<FrameBuffer> &frameBuffer = nullptr);
        void submit(const Light &light);
        void submit(const glm::mat4 &transform, Color color = Color::white, Mesh *mesh = nullptr, Material *material = nullptr, int id = -1);
        void end();
    private:
        Ref<Texture> blankTexture;
        Ref<Mesh> defaultMesh;
        Ref<Shader> defaultShader;
        Ref<FrameBuffer> frameBuffer;
        glm::mat4 projection;
        glm::vec3 cameraPosition;
        BatchBuffer lights;

        class PBBatch;
        std::vector<Ref<PBBatch>> batches;

        void initBatch(PBBatch *batch);
        void flushBatch(PBBatch *batch);
    };

}

#endif //TRIDOT_PBRENDERER_H
