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

    class Renderer::Impl {
    public:
        class Batch {
        public:
            Mesh* mesh = nullptr;
            Shader* shader = nullptr;
            Ref<VertexArray> vertexArray;
            Ref<BatchBuffer> instances;
            AssetList<Texture> textures;
            AssetList<Material> materials;
            Ref<BatchBuffer> materialBuffer;
            int instanceCount = 0;
        };

        std::vector<std::vector<Ref<Batch>>> batches;
        Ref<BatchBuffer> lightBuffer;
        std::vector<Light> lights;
        std::vector<glm::mat4> lightProjections;
        Ref<BatchBuffer> environmentBuffer;
        EnvironmentData environment;
        Ref<Texture> radianceMap;
        Ref<Texture> irradianceMap;
        bool shadowsEnabled = true;

        void startup() {
            lightBuffer = Ref<BatchBuffer>::make();
            lightBuffer->init(sizeof(LightData), UNIFORM_BUFFER);

            environmentBuffer = Ref<BatchBuffer>::make();
            environmentBuffer->init(sizeof(EnvironmentData), UNIFORM_BUFFER);

            env->console->setVariable("shadows", &shadowsEnabled);
        }

        Batch *getBatch(Mesh* mesh, Shader* shader) {
            if (batches.size() > shader->getId()) {
                auto& list = batches[shader->getId()];
                if (list.size() > mesh->vertexArray.getId()) {
                    Batch *batch = list[mesh->vertexArray.getId()].get();
                    if (batch) {
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
            batches[shader->getId()][mesh->vertexArray.getId()] = batch;
            batch->mesh = mesh;
            batch->shader = shader;
            env->renderThread->addTask([batch]() {
                batch->instances = Ref<BatchBuffer>::make();
                batch->instances->init(sizeof(InstanceData));

                batch->materialBuffer= Ref<BatchBuffer>::make();
                batch->materialBuffer->init(sizeof(MaterialData), UNIFORM_BUFFER);

                batch->vertexArray = Ref<VertexArray>::make(batch->mesh->vertexArray);
                batch->vertexArray->addVertexBuffer(batch->instances->buffer, {
                    {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4}, //transform
                    {UINT8, 4, true}, //color
                    {FLOAT, 1}, //material index
                    {UINT8, 4, true}, //id
                }, 1);
            });
            return batch.get();
        }
    };

    void Renderer::startup() {
        geometryPass = env->pipeline->getOrAddRenderPass("geometry");
        shadowPass = env->pipeline->getOrAddRenderPass("shadow");
        
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
    }

    void Renderer::setCamera(glm::mat4& projection, glm::vec3 position, Ref<FrameBuffer> frameBuffer) {
        impl->environment.projection = projection;
        impl->environment.cameraPosition = position;
        this->frameBuffer = frameBuffer;
    }

    void Renderer::setRenderPass(const Ref<RenderPass>& pass) {
        currentPass = pass;
    }

    void Renderer::submit(const glm::vec3& position, const glm::vec3 direction, Light& light) {
        LightData* l = (LightData*)impl->lightBuffer->next();
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
                    l->shadowMapIndex = 30 - impl->lights.size();
                    shadowPass->addCallback([shadowMap = light.shadowMap, index = l->shadowMapIndex]() {
                        shadowMap->getAttachment(DEPTH)->bind(index);
                    }).name = "bind shadow map";
                }
            }
        }

        impl->lights.push_back(light);
        impl->lightProjections.push_back(l->projection);
    }
    
    void Renderer::setEnvironMap(Ref<Texture> radianceMap, Ref<Texture> irradianceMap, float intensity) {
        impl->environment.environmentMapIntensity = intensity;
        impl->radianceMap = radianceMap;
        impl->irradianceMap = irradianceMap;
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
                Impl::Batch *batch = impl->getBatch(mesh, shader);
                if (batch->instances) {
                    InstanceData *i = (InstanceData*)batch->instances->next();
                    i->transform = transform;
                    i->materialIndex = batch->materials.getIndex(material);
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

                auto &call = pass->addDrawCall("direct");
                call.shader = shader;
                call.mesh = mesh;
                call.textures.push_back(texture);
                call.frameBuffer = frameBuffer;

                Ref<ShaderState> shaderState = Ref<ShaderState>::make();
                shaderState->set("uColor", color.vec());
                shaderState->set("uProjection", impl->environment.projection);
                shaderState->set("uTransform", transform);
                call.shaderState = shaderState;
            }
        }
    }

    void Renderer::update() {
        if (currentPass == nullptr) {
            currentPass = geometryPass;
            currentPass->addCommand(DEPTH_ON).name = "depth on";
            currentPass->addCommand(BLEND_ON).name = "blend on";
        }


        //set environment
        EnvironmentData* e = (EnvironmentData*)impl->environmentBuffer->next();
        *e = impl->environment;
        e->lightCount = impl->lightBuffer->size();
        stats.lightCount = e->lightCount;

        if (impl->radianceMap) {
            e->radianceMapIndex = 0;
            currentPass->addCallback([radianceMap = impl->radianceMap]() {
                radianceMap->bind(30);
            }).name = "bind radiance map";
        }
        else {
            e->radianceMapIndex = -1;
        }
        if (impl->irradianceMap) {
            e->irradianceMapIndex = 1;
            currentPass->addCallback([irradianceMap = impl->irradianceMap]() {
                irradianceMap->bind(31);
            }).name = "bind irradiance map";
        }
        else {
            e->irradianceMapIndex = -1;
        }

        impl->environment.radianceMapIndex = -1;
        impl->environment.irradianceMapIndex = -1;
        impl->environment.environmentMapIntensity = 0;
        impl->radianceMap = nullptr;
        impl->irradianceMap = nullptr;


        currentPass->addCallback([&]() {
            impl->lightBuffer->update();
            impl->environmentBuffer->update();
        }).name = "environment";

        impl->lightBuffer->swapBuffers();
        impl->environmentBuffer->swapBuffers();
        impl->lightBuffer->reset();
        impl->environmentBuffer->reset();

        if (impl->shadowsEnabled) {
            updateShadowMaps();
        }

        TRI_PROFILE("geometry");

        stats.shaderCount = 0;
        stats.meshCount = 0;
        stats.instanceCount = 0;

        int meshCounter = 0;
        int materialCounter = 0;

        //instance shader
        for (auto& list : impl->batches) {
            if (list.size() > 0) {
                stats.shaderCount++;
            }
            for (auto batch : list) {
                if (batch && batch->instances) {
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
                        currentPass->addCallback([batch]() {
                            batch->instances->update();
                            batch->materialBuffer->update();
                        }).name = "instances " + file;

                        batch->instanceCount = batch->instances->size();
                        batch->instances->swapBuffers();
                        batch->materialBuffer->swapBuffers();
                        batch->instances->reset();
                        batch->materialBuffer->reset();

                        stats.instanceCount += batch->instanceCount;

                        //set draw call
                        auto& step = currentPass->addDrawCall("mesh " + file);
                        step.shader = batch->shader;
                        step.frameBuffer = frameBuffer;
                        step.mesh = batch->mesh;
                        step.vertexArray = batch->vertexArray.get();
                        step.insatnceCount = batch->instanceCount;


                        //set textures
                        for (auto& tex : batch->textures.assets) {
                            if (tex != nullptr) {
                                step.textures.push_back(tex);
                            }
                        }
                        batch->textures.reset();

                        step.shaderState = Ref<ShaderState>::make();
                        step.shaderState->set("uMaterials", batch->materialBuffer->buffer.get());
                        step.shaderState->set("uLights", impl->lightBuffer->buffer.get());
                        step.shaderState->set("uEnvironment", impl->environmentBuffer->buffer.get());

                        int cubeTextures[2] = { 30, 31 };
                        step.shaderState->set("uCubeTextures", cubeTextures, 2);
                    }
                }
            }
            stats.meshCount = std::max(stats.meshCount, meshCounter);
            stats.materialCount = std::max(stats.materialCount, materialCounter);
        }
        currentPass = nullptr;
    }

    void Renderer::shutdown() {

    }

    void Renderer::updateShadowMaps() {
        TRI_PROFILE("shadowMaps");

        Ref<Shader> shader = env->assets->get<Shader>("shaders/shadow.glsl");
        if (shader->getId() == 0) {
            return;
        }
        for (int i = 0; i < impl->lights.size(); i++) {
            auto& light = impl->lights[i];
            if (light.type != DIRECTIONAL_LIGHT) {
                continue;
            }

            auto &command = shadowPass->addCommand(CLEAR);
            command.name = "clear shadow map";
            command.frameBuffer = light.shadowMap;

            for (auto& list : impl->batches) {
                for (auto& batch : list) {
                    if (batch) {
                        
                        auto file = env->assets->getFile(batch->mesh);
                        TRI_PROFILE_INFO(file.c_str(), file.size());

                        auto& call = shadowPass->addDrawCall("shadow map " + file);
                        call.shader = shader;
                        call.frameBuffer = light.shadowMap;
                        call.shaderState = Ref<ShaderState>::make();
                        call.shaderState->set("uProjection", impl->lightProjections[i]);

                        call.mesh = batch->mesh;
                        call.vertexArray = batch->vertexArray.get();
                        call.insatnceCount = batch->instanceCount;
                    }
                }
            }
        }

        impl->lights.clear();
        impl->lightProjections.clear();
    }

}
