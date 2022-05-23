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
#include "RenderSettings.h"
#include "renderer/ShaderStructs.h"
#include "Window.h"

namespace tri {

    TRI_REGISTER_SYSTEM_INSTANCE(Renderer, env->renderer);

    void Renderer::startup() {
        if (env->renderSettings->deferredShadingEnabled) {
            defaultShader = env->assets->get<Shader>("shaders/geometry.glsl");
        }
        else {
            defaultShader = env->assets->get<Shader>("shaders/forwardPBR.glsl");
        }

        Image image;
        image.init(1, 1, 4, 8);
        image.set(0, 0, Color::white);
        defaultTexture = Ref<Texture>::make();
        defaultTexture->load(image);
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

        setRenderPass(nullptr);


        FrameBufferAttachmentSpec albedo;
        albedo.type = (TextureAttachment)(COLOR);
        albedo.clearColor = env->window->getBackgroundColor();
        albedo.mipMapping = false;
        albedo.name = "albedo";
        albedo.textureFormat = TextureFormat::RGBA8;

        FrameBufferAttachmentSpec emissive;
        emissive.type = (TextureAttachment)(COLOR + 1);
        emissive.clearColor = Color::black;
        emissive.name = "emissive";
        emissive.textureFormat = TextureFormat::RGB8;

        lightAccumulationBuffer = Ref<FrameBuffer>::make();
        lightAccumulationBuffer->init(0, 0, { albedo, emissive });
    }

    void Renderer::setCamera(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix, const glm::vec3& eyePosition, Ref<FrameBuffer> frameBuffer) {
        current->environment.eyePosition = eyePosition;
        current->environment.projection = projectionMatrix;
        current->environment.view = viewMatrix;
        current->environment.viewProjection = projectionMatrix * viewMatrix;
        current->frameBuffer = frameBuffer;
        current->drawList.eyePosition = eyePosition;

        if (current->transparencyContext) {
            current->transparencyContext->environment.eyePosition = eyePosition;
            current->transparencyContext->environment.projection = projectionMatrix;
            current->transparencyContext->environment.view = viewMatrix;
            current->transparencyContext->environment.viewProjection = projectionMatrix * viewMatrix;
            current->transparencyContext->frameBuffer = frameBuffer;
            current->transparencyContext->drawList.eyePosition = eyePosition;
        }
    }

    void Renderer::setRenderPass(Ref<RenderPass> pass) {
        if (!pass) {
            pass = env->renderPipeline->getPass("geometry");
        }
        if (!contexts.contains(pass.get())) {
            auto context = Ref<RenderBatchingContext>::make();
            context->init();
            if (pass == env->renderPipeline->getPass("geometry")) {
                context->hasShadowContexts = true;
                context->transparencyContext = Ref<RenderBatchingContext>::make();
                context->transparencyContext->init();
                context->transparencyContext->pass = env->renderPipeline->getPass("transparency");
            }
            contexts[pass.get()] = context;
            current = context.get();
            current->pass = pass.get();
        }
        else {
            current = contexts[pass.get()].get();
            current->pass = pass.get();
        }
    }

    void Renderer::submit(const glm::vec3& position, const glm::vec3 direction, Light& light) { 
        if (env->renderSettings->shadowsEnabled) {
            if (light.type == DIRECTIONAL_LIGHT) {
                if (light.shadowMap.get() == nullptr) {
                    light.shadowMap = Ref<FrameBuffer>::make();
                    env->renderThread->addTask([shadowMap = light.shadowMap]() {
                        int res = env->renderSettings->shadowMapResolution;
                        Ref<Texture> depth = Ref<Texture>::make();
                        depth->create(res, res, DEPTH32, false);
                        depth->setMagMin(false, false);
                        depth->setWrap(false, false);
                        depth->setBorderColor(Color::white);
                        shadowMap->setAttachment({ DEPTH, Color(0) }, depth);
                        shadowMap->resize(res, res);
                    });
                }
            }
        }

        auto &l = current->lights.emplace_back();
        l.light = light;
        l.position = position;
        l.direction = direction;
    }

    void Renderer::setEnvironMap(Ref<Texture> radianceMap, Ref<Texture> irradianceMap, float intensity) {
        current->environment.environmentMapIntensity = intensity;
        current->radianceMap = radianceMap;
        current->irradianceMap = irradianceMap;
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
                bool opaque = material->isOpaque() && color.a == 255;
                if (!opaque && current->transparencyContext) {
                    if (env->renderSettings->drawListSortingEnabled) {
                        current->transparencyContext->drawList.add(transform, position, mesh, shader, material, color, id, 0, 0);
                    }
                    else {
                        current->transparencyContext->add(transform, mesh, shader, material, color, id);
                    }
                }
                else {
                    if (env->renderSettings->drawListSortingEnabled) {
                        current->drawList.add(transform, position, mesh, shader, material, color, id, 0, 0);
                    }
                    else {
                        current->add(transform, mesh, shader, material, color, id);
                    }
                }
            }
        }
    }

    void Renderer::submit(const glm::mat4& transform, const glm::vec3& position, Mesh* mesh, Shader* shader, Texture* texture, Color color, uint32_t id) {
        //todo
    }

    void Renderer::submitDirect(const glm::mat4& transform, const glm::vec3& position, Mesh* mesh, Shader* shader, Texture* texture, Color color) {
        //todo
    }

    void Renderer::update() {
        if (env->renderSettings->deferredShadingEnabled) {
            defaultShader = env->assets->get<Shader>("shaders/geometry.glsl");
        }
        else {
            defaultShader = env->assets->get<Shader>("shaders/forwardPBR.glsl");
        }

        for (auto &i : contexts) {
            auto* context = i.second.get();
            
            context->frustum.viewProjectionMatrix = context->environment.viewProjection;

            context->updateLights();



            //transparency 
            if (context->transparencyContext) {
                context->transparencyContext->frameBuffer = context->frameBuffer;
                context->transparencyContext->environment = context->environment;
                context->transparencyContext->radianceMap = context->radianceMap;
                context->transparencyContext->irradianceMap = context->irradianceMap;
                context->transparencyContext->frustum.viewProjectionMatrix = context->environment.viewProjection;
                context->transparencyContext->lights = context->lights;
                context->transparencyContext->lightBuffer = context->lightBuffer;
                context->transparencyContext->drawList.eyePosition = context->drawList.eyePosition;

                if (env->renderSettings->deferredShadingEnabled) {
                    for (auto& list : context->transparencyContext->batches.batches) {
                        for (auto& batch : list) {
                            if (batch) {
                                if (batch->shader == defaultShader.get()) {
                                    batch->shader = env->assets->get<Shader>("shaders/forwardPBR.glsl").get();
                                }
                            }
                        }
                    }
                }
            }

            if (env->renderSettings->drawListSortingEnabled) {
                TRI_PROFILE("sortDrawList");
                context->updateDrawList();
            }

            //shadows
            if (env->renderSettings->shadowsEnabled && context->hasShadowContexts) {
                TRI_PROFILE("shadowMaps");
                updateShadowMaps(context);
            }

            context->update();

            if (context->transparencyContext) {
                TRI_PROFILE("transparency");
                if (env->renderSettings->drawListSortingEnabled) {
                    TRI_PROFILE("sortDrawList");
                    context->transparencyContext->updateDrawList();
                }
                context->transparencyContext->updateLights();
                context->transparencyContext->update();
            }

        }

        if (env->renderSettings->deferredShadingEnabled) {
            updateDeferred();
        }
        setRenderPass(nullptr);
    }

    void Renderer::updateShadowMaps(RenderBatchingContext* context) {

        auto pass = env->renderPipeline->getPass("shadow");
        auto shader = env->assets->get<Shader>("shaders/shadow.glsl");

        pass->addCommand("depth on", DEPTH_ON);
        pass->addCommand("blend on", BLEND_ON);

        for (int i = 0; i < context->lights.size(); i++) {
            auto& l = context->lights[i];
            if (l.light.type == DIRECTIONAL_LIGHT) {


                auto call = pass->addCommand("clear", CLEAR)->frameBuffer = l.light.shadowMap;
                for (auto& list : context->batches.batches) {
                    for (auto& batch : list) {
                        if (batch && batch->instances) {
                            auto file = env->assets->getFile(batch->mesh);

                            auto call = pass->addDrawCall(file);
                            call->shader = shader.get();
                            call->frameBuffer = l.light.shadowMap;
                            call->shaderState = Ref<ShaderState>::make();
                            call->shaderState->set("uProjection", l.projection);
                            call->mesh = batch->mesh;
                            call->vertexArray = batch->vertexArray.get();
                            call->instanceCount = batch->instances->size();
                        }
                    }
                }

            }
        }
    }

    void Renderer::updateDeferred() {
        auto geometry = env->renderPipeline->getPass("geometry");
        auto deferred = env->renderPipeline->getPass("deferred");
        auto ssao = deferred->getPass("ssao");

        deferred->addCommand("clear", CLEAR)->frameBuffer = lightAccumulationBuffer;
        deferred->addCommand("resize", RESIZE)->frameBuffer = lightAccumulationBuffer;
        deferred->addCommand("depth off", DEPTH_OFF);

        auto lighting = deferred->getPass("lighting", true, true)->addDrawCall("lighting");
        lighting->shader = env->assets->get<Shader>("shaders/deferredPBR.glsl").get();
        lighting->frameBuffer = lightAccumulationBuffer;
        lighting->textures.resize(5);
        lighting->shaderStateInput = geometry.get();

        lighting->inputs.emplace_back((TextureAttachment)(COLOR + 0), geometry.get());
        lighting->inputs.emplace_back((TextureAttachment)(COLOR + 2), geometry.get());
        lighting->inputs.emplace_back((TextureAttachment)(COLOR + 3), geometry.get());
        lighting->inputs.emplace_back((TextureAttachment)(COLOR + 4), geometry.get());
        lighting->inputs.emplace_back((TextureAttachment)(COLOR), ssao.get());
        lighting->inputs.emplace_back((TextureAttachment)(DEPTH), geometry.get());

        auto swap = deferred->addDrawCall("swap");
        swap->shader = env->assets->get<Shader>("shaders/base.glsl").get();
        swap->textures.push_back(lightAccumulationBuffer->getAttachment(COLOR).get());
        swap->output = geometry.get();
    }

    void Renderer::shutdown() {
        defaultMesh = nullptr;
        defaultTexture = nullptr;
        defaultShader = nullptr;
        defaultMaterial = nullptr;
        frameBuffer = nullptr;

        current = nullptr;
        contexts.clear();
    }

}
