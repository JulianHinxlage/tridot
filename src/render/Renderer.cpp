//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Renderer.h"
#include "core/core.h"
#include "RenderPipeline.h"
#include "ShaderState.h"
#include "engine/AssetManager.h"
#include "RenderThread.h"
#include "engine/Transform.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

namespace tri {

    TRI_REGISTER_SYSTEM_INSTANCE(Renderer, env->renderer);

    //map asstes to an index and keep a list off the assets
    template<typename T>
    class AssetList {
    public:
        std::vector<T*> assets;
        std::unordered_map<T*, int> indexMap;

        int getIndex(T *asset) {
            if (asset == nullptr) {
                return -1;
            }
            auto x = indexMap.find(asset);
            if (x != indexMap.end()) {
                return x->second;
            }
            assets.push_back(asset);
            indexMap[asset] = assets.size() - 1;
            return assets.size() - 1;
        }

        void reset() {
            indexMap.clear();
            assets.clear();
        }
    };

    //insatnce data for shader
    struct InstanceData {
    public:
        glm::mat4 transform;
        Color color;
        float materialIndex;
        uint32_t id;
    };

    //material data for shader
    class MaterialData {
    public:
        glm::vec4 color;
        int mapping;
        float roughness;
        float metallic;
        float normalMapFactor;
        float emissive;

        int texture;
        int normalMap;
        int roughnessMap;
        int metallicMap;
        int ambientOcclusionMap;
        int displacementMap;
        int align1;

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

    //light data for shader
    class LightData {
    public:
        glm::vec3 position;
        int align1;
        glm::vec3 direction;
        int align2;
        glm::vec3 color;
        int align3;
        int type;
        float intensity;
        int shadowMapIndex;
        int align4;
        glm::mat4 projection;
    };

    //environment data for shader
    class EnvironmentData {
    public:
        glm::mat4 projection;
        glm::vec3 cameraPosition;
        int align1;
        int lightCount;
        float environmentMapIntensity;
        int radianceMapIndex;
        int irradianceMapIndex;
    };

    //collection of all instances with the same shader mesh combination
    //includes buffers for instances, textures and materials
    class Batch {
    public:
        Mesh* mesh = nullptr;
        int meshChangeCounter = 0;
        Shader* shader = nullptr;
        Ref<VertexArray> vertexArray;
        Ref<BatchBuffer> instances;
        AssetList<Texture> textures;
        AssetList<Material> materials;
        Ref<BatchBuffer> materialBuffer;
        int instanceCount = 0;
    };

    //a list of instance submit calls to be sorted
    class DrawList {
    public:
        class Entry {
        public:
            InstanceData instance;
            glm::vec3 position;
            Material* material;
            Texture* texture;
            Shader *shader;
            Mesh *mesh;
        };

        class Key {
        public:
            uint64_t key;
            int entryIndex;

            bool operator<(const Key& key) {
                return this->key < key.key;
            }
        };

        std::vector<Entry> entries;
        std::vector<Key> keys;

        void sort() {
            std::sort(keys.begin(), keys.end());
        }

        void clear() {
            entries.clear();
            keys.clear();
        }
    };

    //a list of batches for all shader mesh combinations
    //includes buffers for light, environment, render pass and frame buffer
    class BatchList {
    public:
        AssetList<Shader> shaders;
        AssetList<Mesh> meshes;
        std::vector<std::vector<Ref<Batch>>> batches;
        std::vector<Ref<Batch>> batchesToRemove;

        //lights
        Ref<BatchBuffer> lightBuffer;
        std::vector<Light> lights;
        std::vector<glm::mat4> lightProjections;

        //environments
        Ref<BatchBuffer> environmentBuffer;
        EnvironmentData environment;
        Ref<Texture> radianceMap;
        Ref<Texture> irradianceMap;

        DrawList drawList;

        //the batch list will take the ligths and environment from the parent
        BatchList *parentBatchList;
        
        Ref<RenderPass> renderPass;
        std::string renderPassName;
        Ref<FrameBuffer> frameBuffer;
        bool needsUpdate = true;
        bool frustumCullingEnabled = true;

        void init(Ref<RenderPass> renderPass) {
            lightBuffer = Ref<BatchBuffer>::make();
            environmentBuffer = Ref<BatchBuffer>::make();

            env->renderThread->addTask([&]() {
                lightBuffer->init(sizeof(LightData), UNIFORM_BUFFER);
                environmentBuffer->init(sizeof(EnvironmentData), UNIFORM_BUFFER);
            });

            this->renderPass = renderPass;
            renderPassName = renderPass->name;
            parentBatchList = nullptr;
        }

        Batch* getBatch(Mesh* mesh, Shader* shader) {
            if (batches.size() > shader->getId()) {
                auto& list = batches[shader->getId()];
                if (list.size() > mesh->vertexArray.getId()) {
                    Batch* batch = list[mesh->vertexArray.getId()].get();
                    if (batch && batch->meshChangeCounter == mesh->changeCounter) {
                        return batch;
                    }
                }
                else {
                    list.resize(mesh->vertexArray.getId() + 1);
                }
            }
            else {
                batches.resize(shader->getId() + 1);
                auto& list = batches[shader->getId()];
                list.resize(mesh->vertexArray.getId() + 1);
            }

            //create batch
            Ref<Batch> batch = Ref<Batch>::make();
            batchesToRemove.push_back(batches[shader->getId()][mesh->vertexArray.getId()]);
            batches[shader->getId()][mesh->vertexArray.getId()] = batch;
            batch->mesh = mesh;
            batch->meshChangeCounter = mesh->changeCounter;
            batch->shader = shader;
            env->renderThread->addTask([batch, this]() {
                batch->instances = Ref<BatchBuffer>::make();
                batch->instances->init(sizeof(InstanceData));

                batch->materialBuffer = Ref<BatchBuffer>::make();
                batch->materialBuffer->init(sizeof(MaterialData), UNIFORM_BUFFER);

                batch->vertexArray = Ref<VertexArray>::make(batch->mesh->vertexArray);
                batch->vertexArray->addVertexBuffer(batch->instances->buffer, {
                    {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4}, //transform
                    {UINT8, 4, true}, //color
                    {FLOAT, 1}, //material index
                    {UINT8, 4, true}, //id
                    }, 1);

                batchesToRemove.clear();
            });
            return batch.get();
        }

        void update(Renderer::Statistics &stats) {
            if (!environmentBuffer->buffer) { return; }

            //set environment
            EnvironmentData* e = (EnvironmentData*)environmentBuffer->next();
            *e = environment;
            e->lightCount = lightBuffer->size();
            stats.lightCount = e->lightCount;

            if (radianceMap) {
                e->radianceMapIndex = 0;
                renderPass->addCallback("bind radiance map", [radianceMap = radianceMap]() {
                    radianceMap->bind(30);
                });
            }
            else {
                e->radianceMapIndex = -1;
            }
            if (irradianceMap) {
                e->irradianceMapIndex = 1;
                renderPass->addCallback("bind irradiance map", [irradianceMap = irradianceMap]() {
                    irradianceMap->bind(31);
                });
            }
            else {
                e->irradianceMapIndex = -1;
            }

            environment.radianceMapIndex = -1;
            environment.irradianceMapIndex = -1;
            environment.environmentMapIntensity = 0;
            radianceMap = nullptr;
            irradianceMap = nullptr;


            renderPass->addCallback("environment", [&]() {
                environmentBuffer->update();
            });

            environmentBuffer->swapBuffers();


            renderPass->addCallback("lights", [&]() {
                lightBuffer->update();
            });
            lightBuffer->swapBuffers();

            TRI_PROFILE_NAME(renderPass->name.c_str(), renderPass->name.size());



            //instance shader
            for (auto& list : batches) {
                int meshCounter = 0;
                int materialCounter = 0;
                if (list.size() > 0) {
                    stats.shaderCount++;
                }

                for (auto batch : list) {
                    if (batch && batch->instances && batch->materialBuffer) {
                        if (batch->instances->size() > 0) {
                            meshCounter++;

                            auto file = env->assets->getFile(batch->mesh);
                            TRI_PROFILE_INFO(file.c_str(), file.size());

                            //set materials
                            for (auto& mat : batch->materials.assets) {
                                if (mat != nullptr) {
                                    materialCounter++;

                                    MaterialData* m = (MaterialData*)batch->materialBuffer->next();
                                    m->texture = batch->textures.getIndex(mat->texture.get());
                                    m->normalMap = batch->textures.getIndex(mat->normalMap.get());
                                    m->roughnessMap = batch->textures.getIndex(mat->roughnessMap.get());
                                    m->metallicMap = batch->textures.getIndex(mat->metallicMap.get());
                                    m->ambientOcclusionMap = batch->textures.getIndex(mat->ambientOcclusionMap.get());

                                    m->color = mat->color.vec();
                                    m->mapping = (int)mat->mapping;
                                    m->roughness = mat->roughness;
                                    m->metallic = mat->metallic;
                                    m->normalMapFactor = mat->normalMapFactor;
                                    m->emissive = mat->emissive;

                                    m->textureOffset = mat->textureOffset + mat->offset;
                                    m->textureScale = mat->textureScale * mat->scale;
                                    m->normalMapOffset = mat->normalMapOffset + mat->offset;
                                    m->normalMapScale = mat->normalMapScale * mat->scale;
                                    m->roughnessMapOffset = mat->roughnessMapOffset + mat->offset;
                                    m->roughnessMapScale = mat->roughnessMapScale * mat->scale;
                                    m->metallicMapOffset = mat->metallicMapOffset + mat->offset;
                                    m->metallicMapScale = mat->metallicMapScale * mat->scale;
                                    m->ambientOcclusionMapOffset = mat->ambientOcclusionMapOffset + mat->offset;
                                    m->ambientOcclusionMapScale = mat->ambientOcclusionMapScale * mat->scale;
                                }
                            }
                            batch->materials.reset();


                            //set instances
                            renderPass->addCallback("instances " + file, [batch]() {
                                batch->instances->update();
                                batch->materialBuffer->update();
                            });

                            batch->instanceCount = batch->instances->size();
                            batch->instances->swapBuffers();
                            batch->materialBuffer->swapBuffers();

                            //set draw call
                            auto step = renderPass->addDrawCall("mesh " + file);
                            step->shader = batch->shader;
                            step->frameBuffer = frameBuffer;
                            step->mesh = batch->mesh;
                            step->vertexArray = batch->vertexArray.get();
                            step->instanceCount = batch->instanceCount;


                            //set textures
                            for (auto& tex : batch->textures.assets) {
                                if (tex != nullptr) {
                                    step->textures.push_back(tex);
                                }
                            }
                            batch->textures.reset();

                            step->shaderState = Ref<ShaderState>::make();
                            step->shaderState->set("uMaterials", batch->materialBuffer->buffer.get());
                            step->shaderState->set("uLights", lightBuffer->buffer.get());
                            step->shaderState->set("uEnvironment", environmentBuffer->buffer.get());

                            int cubeTextures[2] = { 30, 31 };
                            step->shaderState->set("uCubeTextures", cubeTextures, 2);
                        }
                    }
                }

                stats.meshCount = std::max(stats.meshCount, meshCounter);
                stats.materialCount = std::max(stats.materialCount, materialCounter);
            }

            lights.clear();
            lightProjections.clear();
            frameBuffer = nullptr;
        }

        bool checkClipSpace(const glm::vec4 &p) {
            if (p.x > p.w) { return false; }
            if (p.x < -p.w) { return false; }
            if (p.y > p.w) { return false; }
            if (p.y < -p.w) { return false; }
            if (p.z > p.w) { return false; }
            if (p.z < -p.w) { return false; }
            return true;
        }

        void submit(const glm::mat4& transform, const glm::vec3& position, Mesh* mesh, Shader *shader, Material* material, Color color, uint32_t id) {
            Batch* batch = getBatch(mesh, shader);
            if (batch->instances) {

                //view frustum culling check
                if (frustumCullingEnabled) {
                    bool cull = true;
                    glm::vec3 scale = transform * glm::vec4(1, 1, 1, 0);
                    if (std::abs(scale.x) > 10) { cull = false; }
                    if (std::abs(scale.y) > 10) { cull = false; }
                    if (std::abs(scale.z) > 10) { cull = false; }

                    if (cull && checkClipSpace(environment.projection * transform * glm::vec4(0, 0, 0, 1.0f))) {
                        cull = false;
                    }
                    if (mesh && cull) {
                        glm::vec3 min = mesh->boundingMin;
                        glm::vec3 max = mesh->boundingMax;
                        for (int i = 0; i < 8; i++) {
                            float x = i % 2;
                            float y = (i / 2) % 2;
                            float z = (i / 4) % 2;

                            if (checkClipSpace(environment.projection * transform * glm::vec4(
                                min.x + x * (max.x - min.x),
                                min.y + y * (max.y - min.y),
                                min.z + z * (max.z - min.z),
                                1.0f))) {
                                cull = false;
                                break;
                            }
                        }
                    }
                    if (cull) { return; }
                }

                InstanceData* i = (InstanceData*)batch->instances->next();
                i->transform = transform;
                i->materialIndex = batch->materials.getIndex(material);
                i->color = color;
                i->id = id;
            }
        }

        void submitDrawList(const glm::mat4& transform, const glm::vec3& position, Mesh* mesh, Shader* shader, Material* material, Color color, uint32_t id) {
            auto& i = drawList.entries.emplace_back();
            i.instance.transform = transform;
            i.instance.color = color;
            i.instance.id = id;
            i.instance.materialIndex = -1;
            
            i.position = position;
            i.material = material;
            i.shader = shader;
            i.mesh = mesh;
            i.texture = nullptr;

            bool opaque = material->isOpaque() && color.a == 255;
            uint32_t depth = glm::length(environment.cameraPosition - position) / 0.0001;
            depth = std::min(depth, (uint32_t)(1 << 24) - 1);

            uint64_t key;
            key = 0;
            //key |= ((uint64_t)layer & 0x3f) << 58;
            key |= (uint64_t)!opaque << 57;
            if (opaque) {
                key |= ((uint64_t)meshes.getIndex(mesh) & 0xffff) << 40;
                key |= ((uint64_t)shaders.getIndex(material->shader.get()) & 0xffff) << 24;
                key |= ((uint64_t)depth & 0xffffff) << 0;
            }
            else {
                key |= ((uint64_t)-depth & 0xffffff) << 32;
                key |= ((uint64_t)meshes.getIndex(mesh) & 0xffff) << 16;
                key |= ((uint64_t)shaders.getIndex(material->shader.get()) & 0xffff) << 0;
            }


            auto& keyEntry = drawList.keys.emplace_back();
            keyEntry.entryIndex = drawList.entries.size() - 1;
            keyEntry.key = key;
        }

        void updateDrawList() {
            TRI_PROFILE("updateDrawList")
            {
                TRI_PROFILE("sort")
                drawList.sort();
            }
            for (auto& key : drawList.keys) {
                auto &entry = drawList.entries[key.entryIndex];
                submit(entry.instance.transform, entry.position, entry.mesh, entry.shader, entry.material, entry.instance.color, entry.instance.id);
            }
            drawList.clear();
            meshes.reset();
            shaders.reset();
        }
    };

    class Renderer::Impl {
    public:
        std::vector<Ref<BatchList>> batchLists;
        BatchList* current;
        BatchList *currentTransparency;
        bool shadowsEnabled = true;
        bool drawListSortingEnabled = true;
        bool transparencyPassEnabled = true;
        bool frustumCullingEnabled = true;

        void startup() {
            env->console->setVariable("shadows", &shadowsEnabled);
            env->console->setVariable("draw_list_sorting", &drawListSortingEnabled);
            env->console->setVariable("transparency_pass", &transparencyPassEnabled);
            env->console->setVariable("frustum_culling", &frustumCullingEnabled);
        }

        Batch *getBatch(Mesh* mesh, Shader* shader) {
            return current->getBatch(mesh, shader);
        }

        void setPass(Ref<RenderPass> pass) {
            for (auto& batchList : batchLists) {
                if (batchList && batchList->renderPassName == pass->name) {
                    current = batchList.get();
                    current->needsUpdate = true;
                    current->renderPass = pass;
                    return;
                }
            }
            auto batchList = batchLists.emplace_back(Ref<BatchList>::make());
            batchList->init(pass);
            current = batchList.get();
        }
    };

    void Renderer::startup() {
        geometryPass = env->renderPipeline->getPass("geometry");
        shadowPass = env->renderPipeline->getPass("shadow");

        opaquePass = geometryPass->getPass("opaque");
        transparencyPass = geometryPass->getPass("transparency");
        
        Image image;
        image.init(1, 1, 4, 8);
        image.set(0, 0, Color::white);
        defaultTexture = Ref<Texture>::make();
        defaultTexture->load(image);

        defaultShader = env->assets->get<Shader>("shaders/pbr.glsl");
        defaultMaterial = Ref<Material>::make();

        defaultMesh = Ref<Mesh>::make();
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
        defaultMesh->create(quadVertices, sizeof(quadVertices) / sizeof(quadVertices[0]), quadIndices, sizeof(quadIndices) / sizeof(quadIndices[0]), { {FLOAT, 3}, {FLOAT, 3} ,{FLOAT, 2} });
    

        impl = Ref<Impl>::make();
        impl->startup();
        setRenderPass(transparencyPass);
        impl->currentTransparency = impl->current;
        setRenderPass(geometryPass);
        impl->currentTransparency->parentBatchList = impl->current;
    }

    void Renderer::setCamera(glm::mat4& projection, glm::vec3 position, Ref<FrameBuffer> frameBuffer) {
        impl->current->environment.projection = projection;
        impl->current->environment.cameraPosition = position;
        this->frameBuffer = frameBuffer;
        impl->current->frameBuffer = frameBuffer;
    }

    void Renderer::setRenderPass(const Ref<RenderPass>& pass) {
        if (!pass || pass == geometryPass) {
            currentPass = geometryPass;
            if (impl->transparencyPassEnabled) {
                impl->setPass(opaquePass);
            }
            else {
                impl->setPass(currentPass);
            }
        }
        else {
            currentPass = pass;
            impl->setPass(pass);
        }
    }

    void Renderer::submit(const glm::vec3& position, const glm::vec3 direction, Light& light) {
        if (!impl->current->lightBuffer || !impl->current->lightBuffer->buffer) { return; }

        LightData* l = (LightData*)impl->current->lightBuffer->next();
        l->type = (int)light.type;
        l->position = position;
        l->direction = direction;
        l->intensity = light.intensity;
        l->color = light.color.vec();
        l->shadowMapIndex = -1;

        if (impl->shadowsEnabled) {
            if (light.shadowMap.get() == nullptr && light.type == DIRECTIONAL_LIGHT) {
                light.shadowMap = Ref<FrameBuffer>::make();
                env->renderThread->addTask([shadowMap = light.shadowMap]() {
                    Ref<Texture> depth = Ref<Texture>::make();
                    depth->create(2048, 2048, DEPTH32, false);
                    depth->setMagMin(false, false);
                    depth->setWrap(false, false);
                    depth->setBorderColor(Color::white);
                    shadowMap->setAttachment({ DEPTH, Color(0) }, depth);
                    shadowMap->resize(2048, 2048);
                });
            }

            if (light.type == DIRECTIONAL_LIGHT) {
                float near = 1.0f;
                float far = 100.0f;
                float size = 20.0f;
                glm::mat4 projection = glm::ortho(-size, size, -size, size, near, far);
                glm::mat4 view = glm::lookAt(position - direction, position, { 0, 1, 0 });
                l->projection = projection * view;
            }
            else {
                l->projection = glm::mat4(1);
            }

            if (light.type == DIRECTIONAL_LIGHT) {
                if (light.shadowMap) {
                    //todo: this should not be done here, move it into update
                    l->shadowMapIndex = 30 - impl->current->lights.size();
                    shadowPass->addCallback("bind shadow map", [shadowMap = light.shadowMap, index = l->shadowMapIndex]() {
                        shadowMap->getAttachment(DEPTH)->bind(index);
                    });
                }
            }
        }

        impl->current->lights.push_back(light);
        impl->current->lightProjections.push_back(l->projection);
    }
    
    void Renderer::setEnvironMap(Ref<Texture> radianceMap, Ref<Texture> irradianceMap, float intensity) {
        impl->current->environment.environmentMapIntensity = intensity;
        impl->current->radianceMap = radianceMap;
        impl->current->irradianceMap = irradianceMap;
    }
    
    void Renderer::submit(const glm::mat4& transform, const glm::vec3& position, Mesh* mesh, Material* material, Color color, uint32_t id) {
        if (!mesh) {
            mesh = defaultMesh.get();
        }
        if (!material) {
            material = defaultMaterial.get();
        }
        Shader* shader = material->shader.get();
        if (!shader) {
            shader = defaultShader.get();
        }
        if (mesh->vertexArray.getId() != 0) {
            if (shader->getId() != 0) {

                if (impl->transparencyPassEnabled && currentPass == geometryPass) {
                    bool opaque = material->isOpaque() && color.a == 255;
                    if (opaque) {
                        if (impl->drawListSortingEnabled) {
                            impl->current->submitDrawList(transform, position, mesh, shader, material, color, id);
                        }
                        else {
                            impl->current->submit(transform, position, mesh, shader, material, color, id);
                        }
                    }
                    else {
                        if (impl->drawListSortingEnabled) {
                            impl->currentTransparency->submitDrawList(transform, position, mesh, shader, material, color, id);
                        }
                        else {
                            impl->currentTransparency->submit(transform, position, mesh, shader, material, color, id);
                        }
                    }
                }
                else {
                    if (impl->drawListSortingEnabled) {
                        impl->current->submitDrawList(transform, position, mesh, shader, material, color, id);
                    }
                    else {
                        impl->current->submit(transform, position, mesh, shader, material, color, id);
                    }
                }

            }
        }   
    }

    void Renderer::submit(const glm::mat4& transform, const glm::vec3& position, Mesh* mesh, Shader* shader, Texture* texture, Color color, uint32_t id) {
        if (!mesh) {
            mesh = defaultMesh.get();
        }
        if (!shader) {
            shader = defaultShader.get();
        }
        if (mesh->vertexArray.getId() != 0) {
            if (shader->getId() != 0) {
                Batch* batch = impl->getBatch(mesh, shader);
                if (batch->instances) {
                    InstanceData* i = (InstanceData*)batch->instances->next();
                    i->transform = transform;
                    i->materialIndex = batch->textures.getIndex(texture);
                    i->color = color;
                    i->id = id;
                }
            }
        }
    }

    void Renderer::submitDirect(const glm::mat4& transform, const glm::vec3& position, Mesh* mesh, Shader* shader, Texture* texture, Color color) {
        if (!mesh) {
            mesh = defaultMesh.get();
        }
        if (mesh->vertexArray.getId() != 0) {
            if (!shader) {
                shader = env->assets->get<Shader>("shaders/base.glsl").get();
            }
            if (shader->getId() != 0) {
                if (!texture) {
                    texture = defaultTexture.get();
                }

                RenderPass* pass = currentPass.get();
                if (!pass) {
                    pass = geometryPass.get();
                }

                auto call = geometryPass->addDrawCall("direct");
                call->shader = shader;
                call->mesh = mesh;
                call->textures.push_back(texture);
                call->frameBuffer = frameBuffer.get();

                Ref<ShaderState> shaderState = Ref<ShaderState>::make();
                shaderState->set("uColor", color.vec());
                shaderState->set("uProjection", impl->current->environment.projection);
                shaderState->set("uTransform", transform);
                call->shaderState = shaderState;
            }
        }
    }

    void Renderer::update() {
        stats.shaderCount = 0;
        stats.meshCount = 0;

        for (auto& list : impl->batchLists) {
            list->frustumCullingEnabled = impl->frustumCullingEnabled;
            if (list && list->needsUpdate) {
                
                if (list->parentBatchList) {
                    list->environment = list->parentBatchList->environment;
                    list->lights = list->parentBatchList->lights;
                    list->lightProjections = list->parentBatchList->lightProjections;
                    list->frameBuffer = list->parentBatchList->frameBuffer;
                    list->irradianceMap = list->parentBatchList->irradianceMap;
                    list->radianceMap = list->parentBatchList->radianceMap;
                }

                if (list->renderPass == geometryPass) {
                    list->renderPass->addCommand("depth on", DEPTH_ON);
                    list->renderPass->addCommand("blend on", BLEND_ON);
                }else if (list->renderPass == opaquePass) {
                    list->renderPass->addCommand("depth on", DEPTH_ON);
                    list->renderPass->addCommand("blend on", BLEND_ON);
                }else if (list->renderPass == transparencyPass) {
                    list->renderPass->addCommand("depth on", DEPTH_ON);
                    list->renderPass->addCommand("blend on", BLEND_ON);
                }
                

                if (impl->drawListSortingEnabled) {
                    list->updateDrawList();
                }

                if (impl->shadowsEnabled && (list->renderPass == geometryPass || list->renderPass == opaquePass)) {
                    impl->current = list.get();
                    updateShadowMaps();
                }

                list->update(stats);
                list->needsUpdate = false;
            }
        }
        
        //set needsUpdate for transparency pass
        if (impl->transparencyPassEnabled) {
            setRenderPass(transparencyPass);
        }

        setRenderPass(nullptr);
        frameBuffer = nullptr;
    }

    void Renderer::shutdown() {

    }

    void Renderer::updateShadowMaps() {
        TRI_PROFILE("shadowMaps");

        Ref<Shader> shader = env->assets->get<Shader>("shaders/shadow.glsl");
        if (shader->getId() == 0) {
            return;
        }
        for (int i = 0; i < impl->current->lights.size(); i++) {
            auto& light = impl->current->lights[i];
            if (light.type != DIRECTIONAL_LIGHT) {
                continue;
            }

            shadowPass->addCommand("clear shadow map", CLEAR)->frameBuffer = light.shadowMap.get();

            for (auto& list : impl->current->batches) {
                for (auto& batch : list) {
                    if (batch) {
                        
                        auto file = env->assets->getFile(batch->mesh);
                        TRI_PROFILE_INFO(file.c_str(), file.size());

                        auto call = shadowPass->addDrawCall("shadow map " + file);
                        call->shader = shader.get();
                        call->frameBuffer = light.shadowMap.get();
                        call->shaderState = Ref<ShaderState>::make();
                        call->shaderState->set("uProjection", impl->current->lightProjections[i]);

                        call->mesh = batch->mesh;
                        call->vertexArray = batch->vertexArray.get();
                        call->instanceCount = batch->instanceCount;
                    }
                }
            }
        }

    }

}
