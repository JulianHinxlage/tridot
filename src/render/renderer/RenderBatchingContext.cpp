//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "engine/AssetManager.h"
#include "engine/Transform.h"
#include "render/RenderPipeline.h"
#include "render/ShaderState.h"
#include "render/RenderThread.h"
#include "render/RenderSettings.h"
#include "RenderBatchingContext.h"
#include "ShaderStructs.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

namespace tri {

    void RenderBatchingContext::init() {
        lightBuffer = Ref<BatchBuffer>::make();
        environmentBuffer = Ref<BatchBuffer>::make();
        env->renderThread->addTask([&]() {
            lightBuffer->init(sizeof(LightData), UNIFORM_BUFFER);
            environmentBuffer->init(sizeof(EnvironmentData), UNIFORM_BUFFER);
        });
        hasShadowContexts = false;
    }

    void RenderBatchingContext::updateDrawList() {
        drawList.sort();
        for (auto& k : drawList.keys) {
            auto& e = drawList.entries[k.entryIndex];
            add(e.instance.transform, e.mesh, e.shader, e.material, e.instance.color, e.instance.id);
        }
        drawList.clear();
    }

    void RenderBatchingContext::updateLights() {
        for (auto& e : lights) {
            LightData* l = (LightData*)lightBuffer->next();
            l->type = (int)e.light.type;
            l->position = e.position;
            l->direction = e.direction;
            l->intensity = e.light.intensity;
            l->color = e.light.color.vec();
            l->radius = e.light.radius;
            l->shadowMapIndex = -1;

            if (env->renderSettings->shadowsEnabled) {
                if (e.light.type == DIRECTIONAL_LIGHT) {
                    float near = 1.0f;
                    float far = 100.0f;
                    float size = 20.0f;
                    glm::mat4 projection = glm::ortho(-size, size, -size, size, near, far);
                    glm::mat4 view = glm::lookAt(e.position - e.direction, e.position, { 0, 1, 0 });
                    l->projection = projection * view;
                    
                    if (e.light.shadowMap) {
                        for (auto& list : batches.batches) {
                            for (auto& batch : list) {
                                if (batch) {
                                    l->shadowMapIndex = batch->textures.getIndex(e.light.shadowMap->getAttachment(DEPTH).get());
                                }
                            }
                        }
                    }
                }
                else {
                    l->projection = glm::mat4(1);
                }

                e.projection = l->projection;
            }
        }
    }

    void RenderBatchingContext::update() {
        if (!environmentBuffer->buffer) { return; }

        pass->addCommand("depth on", DEPTH_ON);
        pass->addCommand("blend on", BLEND_ON);

        //set environment
        EnvironmentData* e = (EnvironmentData*)environmentBuffer->next();
        *e = environment;
        e->lightCount = lightBuffer->size();
        env->renderSettings->stats.lightCount = e->lightCount;

        if (radianceMap) {
            e->radianceMapIndex = 0;
            pass->addCallback("bind radiance map", [radianceMap = radianceMap]() {
                radianceMap->bind(30);
            });
        }
        else {
            e->radianceMapIndex = -1;
        }
        if (irradianceMap) {
            e->irradianceMapIndex = 1;
            pass->addCallback("bind irradiance map", [irradianceMap = irradianceMap]() {
                irradianceMap->bind(31);
            });
        }
        else {
            e->irradianceMapIndex = -1;
        }

        pass->addCallback("environment", [&]() {
            environmentBuffer->update();
        });
        environmentBuffer->swapBuffers();


        pass->addCallback("lights", [&]() {
            lightBuffer->update();
        });
        lightBuffer->swapBuffers();


        TRI_PROFILE_NAME(pass->name.c_str(), pass->name.size());

        for (auto& list : batches.batches) {

            int meshCounter = 0;
            int materialCounter = 0;
            if (batches.batches.size() > 0) {
                env->renderSettings->stats.shaderCount++;
            }

            for (auto& batch : list) {
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

                        batch->instanceCount = batch->instances->size();
                        batch->instances->swapBuffers();
                        batch->materialBuffer->swapBuffers();

                        //set draw call
                        auto step = pass->addDrawCall("mesh " + file);
                        step->shader = batch->shader;
                        step->frameBuffer = frameBuffer;
                        step->mesh = batch->mesh;
                        step->buffers.push_back(batch->instances.get());
                        step->buffers.push_back(batch->materialBuffer.get());
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

            env->renderSettings->stats.meshCount = std::max(env->renderSettings->stats.meshCount, meshCounter);
            env->renderSettings->stats.materialCount = std::max(env->renderSettings->stats.materialCount, materialCounter);
        }

        environment.radianceMapIndex = -1;
        environment.irradianceMapIndex = -1;
        environment.environmentMapIntensity = 0;
        radianceMap = nullptr;
        irradianceMap = nullptr;

        lights.clear();
        frameBuffer = nullptr;
    }

    void RenderBatchingContext::add(const glm::mat4& transform, Mesh* mesh, Shader* shader, Material* material, Color color, uint32_t id) {
        RenderBatch* batch = batches.getBatch(mesh, shader);
        if (batch->instances) {
            //view frustum culling check
            if (env->renderSettings->frustumCullingEnabled) {
                if (!frustum.checkFrustum(transform, mesh)) {
                    return;
                }
            }

            InstanceData* i = (InstanceData*)batch->instances->next();
            i->transform = transform;
            i->materialIndex = batch->materials.getIndex(material);
            i->color = color;
            i->id = id;
        }
    }

}
