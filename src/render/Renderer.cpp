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

        int drawCallCount = 0;
        int instanceCount = 0;
        int meshCount = 0;
        int materialCount = 0;
        int shaderCount = 0;
        int lightCount = 0;

        class InstanceData {
        public:
            glm::mat4 transform;
            Color color;
            float materialIndex;
            uint32_t id;
        };

        class MaterialData {
        public:
            glm::vec4 color;
            int mapping;
            float roughness;
            float metallic;
            float normalMapFactor;

            int texture;
            int normalMap;
            int roughnessMap;
            int metallicMap;
            int ambientOcclusionMap;
            int displacementMap;
            int align1;
            int align2;

            glm::vec2 textureOffset;
            glm::vec2 textureScale;

            glm::vec2 normalMapOffset;
            glm::vec2 normalMapScale;

            glm::vec2 roughnessMapOffset;
            glm::vec2 roughnessMapScale;

            glm::vec2 metallicMapOffset;
            glm::vec2 metallicMapScale;

            glm::vec2 ambientOcclusionMapOffset;
            glm::vec2 ambientOcclusionMapScale;

            glm::vec2 displacementMapOffset;
            glm::vec2 displacementMapScale;
        };

        class LightData {
        public:
            glm::vec3 positionOrDirection;
            int align1;
            glm::vec3 color;
            int align2;
            float intensity;
            int type;
            int align3;
            int align4;
        };

        class EnvironmentData{
        public:
            glm::mat4 projection;
            glm::vec3 cameraPosition;
            int align1;
            int lightCount;
            float environmentMapIntensity;
            int environmentMapIndex;
            int irradianceMapIndex;
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

        std::unordered_map<Shader*, uint32_t> shaderMap;
        std::unordered_map<Mesh*, uint32_t> meshMap;
        std::unordered_map<Material*, uint32_t> materialMap;
        std::unordered_map<Texture*, uint32_t> textureMap;
        Ref<Shader> defaultShader;

        class MeshBatch {
        public:
            Ref<BatchBuffer> instances;
            Ref<VertexArray> mesh;
            int meshId;
        };
        std::unordered_map<Mesh*, MeshBatch> meshes;
        BatchBuffer materialBuffer;
        BatchBuffer lightBuffer;
        Ref<Buffer> environmentBuffer;
        bool useMaterials = true;

        uint32_t getShaderIndex(Shader* shader) {
            auto x = shaderMap.find(shader);
            if (x == shaderMap.end()) {
                uint32_t index = shaderMap.size();
                shaderMap[shader] = index;
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

        uint32_t getTextureIndex(Texture* texture) {
            if(!texture){
                return -1;
            }
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

        void init() {
            materialBuffer.init(sizeof(MaterialData), UNIFORM_BUFFER);
            lightBuffer.init(sizeof(LightData), UNIFORM_BUFFER);
            environmentBuffer = Ref<Buffer>::make();
            environmentBuffer->init(nullptr, 0, sizeof(EnvironmentData), UNIFORM_BUFFER, true);
        }

        void addLight(const glm::vec3 &positionOrDirection, const Light &light){
            LightData *l = (LightData*)lightBuffer.next();
            l->color = light.color.vec();
            l->type = (int)light.type;
            l->positionOrDirection = positionOrDirection;
            l->intensity = light.intensity;
        }

        void add(const glm::mat4 &transform, const glm::vec3& position, Mesh* mesh, Material* material, Color color, uint32_t id, uint32_t layer = 0) {
            if(mesh->vertexArray.getId() == 0){
                return;
            }

            bool opaque = material->isOpaque() && color.a == 255;
            uint32_t depth = glm::length(eyePosition - position) / 0.0001;
            depth = std::min(depth, (uint32_t)(1 << 24) - 1);

            SortKey key;
            key.v1 = 0;
            key.v1 |= ((uint64_t)layer & 0x3f) << 58;
            key.v1 |= (uint64_t)!opaque << 57;
            if (opaque) {
                key.v1 |= ((uint64_t)getMeshIndex(mesh) & 0xffff) << 40;
                key.v1 |= ((uint64_t)getShaderIndex(material->shader.get()) & 0xffff) << 24;
                key.v1 |= ((uint64_t)depth & 0xffffff) << 0;
            }
            else {
                key.v1 |= ((uint64_t)-depth & 0xffffff) << 32;
                key.v1 |= ((uint64_t)getMeshIndex(mesh) & 0xffff) << 16;
                key.v1 |= ((uint64_t)getShaderIndex(material->shader.get()) & 0xffff) << 0;
                //key.v1 |= ((uint64_t)getMaterialIndex(material) & 0xffff) << 0;
            }
            getMaterialIndex(material);

            calls.push_back({transform, color, material, mesh, id});
            drawList.push_back({ key, (uint32_t)calls.size() - 1 });
        }

        void sort() {
            TRI_PROFILE("render/sort");
            std::sort(drawList.begin(), drawList.end());
        }

        BatchBuffer *prepareMesh(Mesh* mesh) {
            auto x = meshes.find(mesh);
            MeshBatch* batch = nullptr;
            if (x == meshes.end()) {
                batch = &meshes[mesh];
                batch->mesh = Ref<VertexArray>::make(mesh->vertexArray);
                batch->instances = Ref<BatchBuffer>::make();
                batch->instances->init(sizeof(InstanceData), VERTEX_BUFFER);
                batch->mesh->addVertexBuffer(batch->instances->buffer, {
                    {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4}, //transform
                    {UINT8, 4, true}, //color
                    {FLOAT, 1}, //material index
                    {UINT8, 4, true} }, //id
                    1);
                batch->meshId = mesh->vertexArray.getId();
            }else{
                batch = &x->second;
                if(batch->meshId != mesh->vertexArray.getId()){
                    meshes.erase(mesh);
                    return prepareMesh(mesh);
                }
            }
            return batch->instances.get();
        }

        void submit(Mesh* mesh, Shader *shader) {
            MeshBatch& batch = meshes[mesh];
            if(batch.instances->size() > 0) {
                if(useMaterials){
                    materialBuffer.update();
                    shader->set("uMaterials", materialBuffer.buffer.get());
                }

                int textures[30];
                for (int i = 0; i < 30; i++) {
                    textures[i] = i;
                }
                for (auto &tex : textureMap) {
                    if (tex.first) {
                        tex.first->bind(tex.second);
                    }
                }
                shader->set("uTextures", textures, 30);

                batch.instances->update();
                batch.mesh->submit(-1, batch.instances->size());
                batch.instances->reset();
                RenderContext::flush(false);

                materialBuffer.reset();
                materialMap.clear();
                textureMap.clear();
                drawCallCount++;
            }
        }

        void setMaterialData(Material *m, MaterialData *d){
            d->color = m->color.vec();
            d->mapping = (int)m->mapping;

            d->roughness = m->roughness;
            d->metallic = m->metallic;
            d->normalMapFactor = m->normalMapFactor;

            d->texture = getTextureIndex(m->texture.get());
            d->normalMap = getTextureIndex(m->normalMap.get());
            d->roughnessMap = getTextureIndex(m->roughnessMap.get());
            d->metallicMap = getTextureIndex(m->metallicMap.get());
            d->ambientOcclusionMap = getTextureIndex(m->ambientOcclusionMap.get());
            d->displacementMap = getTextureIndex(m->displacementMap.get());

            d->textureOffset = m->textureOffset + m->offset;
            d->textureScale = m->textureScale * m->scale;
            d->normalMapOffset = m->normalMapOffset + m->offset;
            d->normalMapScale = m->normalMapScale * m->scale;
            d->roughnessMapOffset = m->roughnessMapOffset + m->offset;
            d->roughnessMapScale = m->roughnessMapScale * m->scale;
            d->metallicMapOffset = m->metallicMapOffset + m->offset;
            d->metallicMapScale = m->metallicMapScale * m->scale;
            d->ambientOcclusionMapOffset = m->ambientOcclusionMapOffset + m->offset;
            d->ambientOcclusionMapScale = m->ambientOcclusionMapScale * m->scale;
            d->displacementMapOffset = m->displacementMapOffset + m->offset;
            d->displacementMapScale = m->displacementMapScale * m->scale;
        }

        void draw() {
            TRI_PROFILE("render/draw");

            drawCallCount = 0;
            instanceCount = 0;
            materialCount = materialMap.size();
            lightCount = lightBuffer.size();
            shaderCount = shaderMap.size();
            meshCount = meshMap.size();

            Mesh* mesh = nullptr;
            Material* material = nullptr;
            BatchBuffer* instances;
            Shader *shader = nullptr;

            materialMap.clear();
            int materialIndex = 0;
            useMaterials = true;

            for (auto& i : drawList) {
                Call &call = calls[i.index];

                if (call.mesh != mesh) {
                    if (mesh) {
                        submit(mesh, shader);
                        material = nullptr;
                    }
                    mesh = call.mesh;
                    instances = prepareMesh(mesh);
                }

                if (material != call.material) {
                    Shader *s = call.material->shader.get();
                    if(!s){
                        s = defaultShader.get();
                    }
                    if(shader != s){
                        if(shader){
                            submit(mesh, shader);
                            material = nullptr;
                        }

                        shader = s;
                        useMaterials = shader->has("uMaterials");
                        shader->bind();

                        lightBuffer.update();
                        shader->set("uLights", lightBuffer.buffer.get());

                        EnvironmentData e;
                        e.projection = projectionMatrix;
                        e.cameraPosition = eyePosition;
                        e.lightCount = lightBuffer.size();
                        e.environmentMapIndex = -1;
                        e.irradianceMapIndex = -1;
                        e.environmentMapIntensity = 0;
                        environmentBuffer->setData(&e, sizeof(e));
                        shader->set("uEnvironment", environmentBuffer.get());
                    }

                    material = call.material;
                    if(!useMaterials){
                        materialIndex = getTextureIndex(material->texture.get());
                    }else{
                        materialIndex = getMaterialIndex(material);
                        if(materialIndex >= materialBuffer.size()){
                            setMaterialData(material, (MaterialData*)materialBuffer.next());
                        }
                    }
                }

                InstanceData* instance = (InstanceData*)instances->next();
                instance->transform = call.transform;
                instance->color = call.color;
                instance->materialIndex = materialIndex;
                instance->id = call.id;
                instanceCount++;
            }

            if (mesh) {
                submit(mesh, shader);
            }

            materialBuffer.reset();
            lightBuffer.reset();
            materialMap.clear();
            textureMap.clear();
            meshMap.clear();
            shaderMap.clear();
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

    void Renderer::submit(const glm::vec3 &positionOrDirection, const Light &light) {
        drawList->addLight(positionOrDirection, light);
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

        drawCallCount += drawList->drawCallCount;
        instanceCount += drawList->instanceCount;
        meshCount += drawList->meshCount;
        materialCount += drawList->materialCount;
        shaderCount += drawList->shaderCount;
        lightCount += drawList->lightCount;
    }

    void Renderer::resetScene() {
        drawList->clear();
    }

    void Renderer::startup() {
        RenderContext::setDepth(true);
        RenderContext::setBlend(true);
        RenderContext::setCull(true);

        quad = quad.make();
        float quadVertices[] = {
            -0.5, +0.0, -0.5, 0.0, 1.0, 0.0, 0.0, 0.0,
            +0.5, +0.0, -0.5, 0.0, 1.0, 0.0, 1.0, 0.0,
            +0.5, +0.0, +0.5, 0.0, 1.0, 0.0, 1.0, 1.0,
            -0.5, +0.0, +0.5, 0.0, 1.0, 0.0, 0.0, 1.0,
        };
        int quadIndices[] = {
            0, 2, 1,
            0, 3, 2,
        };
        quad->create(quadVertices, sizeof(quadVertices) / sizeof(quadVertices[0]), quadIndices, sizeof(quadIndices) / sizeof(quadIndices[0]), { {FLOAT, 3}, {FLOAT, 3} ,{FLOAT, 2} });

        defaultPipeline = defaultPipeline.make(quad);

        Image image;
        image.init(1, 1, 4, 8);
        image.set(0, 0, Color::white);
        defaultTexture = defaultTexture.make();
        defaultTexture->load(image);

        defaultShader = env->assets->get<Shader>("shaders/pbr.glsl");

        defaultMaterial = defaultMaterial.make();
        defaultMaterial->texture = defaultTexture;
        defaultMaterial->shader = defaultShader;

        drawList = drawList.make();
        drawList->defaultShader = defaultShader;
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
