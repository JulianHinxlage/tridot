//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "core/core.h"
#include "Renderer.h"
#include "engine/AssetManager.h"
#include "BatchBuffer.h"
#include "render/RenderContext.h"

namespace tri {

    TRI_REGISTER_SYSTEM_INSTANCE(Renderer, env->renderer);

    class Renderer::DrawList {
    public:
        glm::mat4 projectionMatrix;
        glm::vec3 eyePosition;

        class Instance {
        public:
            glm::mat4 transform;
            Color color;
            float materialIndex;
            uint32_t id;
        };

        class SortKey {
        public:
            uint64_t v1;

            bool operator<(const SortKey &key) {
                return v1 < key.v1;
            }
        };

        class Entry {
        public:
            SortKey key;
            uint32_t index;

            bool operator<(const Entry& entry) {
                return key < entry.key;
            }
        };

        std::vector<Entry> drawList;

        class Call {
        public:
            glm::mat4 transform;
            Color color;
            Material* material;
            Mesh *mesh;
            uint32_t id;
        };
        std::vector<Call> calls;

        std::unordered_map<Material*, uint32_t> materialMap;
        std::unordered_map<Texture*, uint32_t> textureMap;
        std::unordered_map<Mesh*, uint32_t> meshMap;

        class MeshBatch {
        public:
            Ref<BatchBuffer> instances;
            Ref<VertexArray> mesh;
        };
        std::unordered_map<Mesh*, MeshBatch> instancedMeshes;

        uint32_t getMaterialIndex(Material* material) {
            auto x = materialMap.find(material);
            if (x == materialMap.end()) {
                uint32_t index = materialMap.size();
                materialMap[material] = index;
                return index;
            }
            else {
                return x->second;
            }
        }

        uint32_t getMeshIndex(Mesh* mesh) {
            auto x = meshMap.find(mesh);
            if (x == meshMap.end()) {
                uint32_t index = meshMap.size();
                meshMap[mesh] = index;
                return index;
            }
            else {
                return x->second;
            }
        }

        uint32_t getTextureIndex(Texture* texture) {
            auto x = textureMap.find(texture);
            if (x == textureMap.end()) {
                uint32_t index = textureMap.size();
                textureMap[texture] = index;
                return index;
            }
            else {
                return x->second;
            }
        }

        void init() {}

        void add(const glm::mat4 &transform, const glm::vec3& position, Mesh* mesh, Material* material, Color color, uint32_t id, uint32_t layer = 0) {

            bool opaque = material->isOpaque() && color.a == 255;
            uint32_t depth = glm::length(eyePosition - position) / 0.0001;
            depth = std::min(depth, (uint32_t)(1 << 24) - 1);

            SortKey key;
            key.v1 = 0;
            key.v1 |= ((uint64_t)layer && 0x3f) << 58;
            key.v1 |= (uint64_t)!opaque << 57;
            if (opaque) {
                key.v1 |= ((uint64_t)getMaterialIndex(material) & 0xffff) << 40;
                key.v1 |= ((uint64_t)getMeshIndex(mesh) & 0xffff) << 24;
                key.v1 |= ((uint64_t)depth & 0xffffff) << 0;
            }
            else {
                key.v1 |= ((uint64_t)-depth & 0xffffff) << 0;
                key.v1 |= ((uint64_t)getMaterialIndex(material) & 0xffff) << 40;
                key.v1 |= ((uint64_t)getMeshIndex(mesh) & 0xffff) << 24;
            }

            calls.push_back({transform, color, material, mesh, id});
            drawList.push_back({ key, (uint32_t)calls.size() - 1 });
        }

        void sort() {
            TRI_PROFILE("render/sort");
            std::sort(drawList.begin(), drawList.end());
        }

        void prepareMesh(Mesh* mesh) {
            auto x = instancedMeshes.find(mesh);
            MeshBatch* batch = nullptr;
            if (x == instancedMeshes.end()) {
                batch = &instancedMeshes[mesh];
                batch->mesh = Ref<VertexArray>::make(mesh->vertexArray);
                batch->instances = Ref<BatchBuffer>::make();
                batch->instances->init(sizeof(Instance), VERTEX_BUFFER);
                batch->mesh->addVertexBuffer(batch->instances->buffer, {
                    {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4}, //transform
                    {UINT8, 4, true}, //color
                    {FLOAT, 1}, //material index
                    {UINT8, 4, true} }, //id
                    1);
            }
        }

        void submit(Mesh* mesh) {
            MeshBatch& batch = instancedMeshes[mesh];
            batch.instances->update();
            batch.mesh->submit(-1, batch.instances->size());
            batch.instances->reset();
        }

        void draw() {
            TRI_PROFILE("render/draw");
            Mesh* mesh = nullptr;
            Material* material = nullptr;
            BatchBuffer* instances;

            for (auto& i : drawList) {
                Call &call = calls[i.index];

                if (material != call.material) {
                    material = call.material;
                    material->shader->bind();
                    material->shader->set("uProjection", projectionMatrix);
                    material->shader->set("uEyePosition", eyePosition);
                }

                if (call.mesh != mesh) {
                    if (mesh) {
                        submit(mesh);
                    }
                    mesh = call.mesh;
                    prepareMesh(mesh);
                    instances = instancedMeshes[mesh].instances.get();
                }

                Instance* instance = (Instance*)instances->next();

                instance->transform = call.transform;
                instance->color = call.color;
                instance->materialIndex = materialMap[call.material];
                instance->id = call.id;
            }

            if (mesh) {
                submit(mesh);
            }
        }

        void clear() {
            drawList.clear();
            calls.clear();
        }

    };

    void Renderer::beginScene(glm::mat4 projectionMatrix, glm::vec3 eyePosition){
        drawList->projectionMatrix = projectionMatrix;
        drawList->eyePosition = eyePosition;
    }

    void Renderer::submit(const glm::mat4& transform, const glm::vec3& position, Mesh* mesh, Material* material, Color color, uint32_t id) {
        if (!material) {
            material = defaultMaterial.get();
        }
        if (!mesh) {
            mesh = quad.get();
        }
        drawList->add(transform, position, mesh, material, color, id);
    }

    void Renderer::drawScene(Ref<FrameBuffer> frameBuffer, Ref<RenderPipeline> pipeline) {
        if (pipeline.get() == nullptr) {
            pipeline = defaultPipeline;
        }
        if (pipeline->input) {
            pipeline->input->bind();
        }
        else {
            if (frameBuffer) {
                frameBuffer->bind();
            }
            else {
                FrameBuffer::unbind();
            }
        }
        drawList->sort();
        drawList->draw();
        pipeline->executePipeline(frameBuffer);
    }

    void Renderer::resetScene() {
        drawList->clear();
    }

    void Renderer::startup() {
        RenderContext::setDepth(true);
        RenderContext::setBlend(true);

        quad = quad.make();
        float quadVertices[] = {
            -0.5, -0.5, +0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
            +0.5, -0.5, +0.0, 0.0, 0.0, 1.0, 1.0, 0.0,
            +0.5, +0.5, +0.0, 0.0, 0.0, 1.0, 1.0, 1.0,
            -0.5, +0.5, +0.0, 0.0, 0.0, 1.0, 0.0, 1.0,
        };
        int quadIndices[] = {
            0, 1, 2,
            0, 2, 3,
        };
        quad->create(quadVertices, sizeof(quadVertices) / sizeof(quadVertices[0]), quadIndices, sizeof(quadIndices) / sizeof(quadIndices[0]), { {FLOAT, 3}, {FLOAT, 3} ,{FLOAT, 2} });

        defaultPipeline = defaultPipeline.make(quad);

        Image image;
        image.init(1, 1, 4, 8);
        image.set(0, 0, Color::white);
        defaultTexture = defaultTexture.make();
        defaultTexture->load(image);

        defaultShader = env->assets->get<Shader>("shaders/mesh.glsl");

        defaultMaterial = defaultMaterial.make();
        defaultMaterial->texture = defaultTexture;
        defaultMaterial->shader = defaultShader;

        drawList = drawList.make();
        drawList->init();
    }

    void Renderer::update() {

    }

    void Renderer::shutdown() {
        defaultMaterial = nullptr;
        defaultShader = nullptr;
        defaultTexture = nullptr;
        defaultPipeline = nullptr;
        drawList = nullptr;
    }

}
