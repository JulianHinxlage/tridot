//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "OutlineRenderer.h"
#include "engine/AssetManager.h"
#include "window/Viewport.h"
#include "MeshFactory.h"
#include "engine/Transform.h"

namespace tri {

    TRI_SYSTEM(OutlineRenderer);

    void OutlineRenderer::startup() {
        outlineColor = Color(glm::vec4(1.0, 0.5, 0.0, 1));
        outlineShader = env->assetManager->get<Shader>("shaders/outline.glsl");
        meshShader = env->assetManager->get<Shader>("shaders/mesh.glsl");
        material = Ref<Material>::make();
        env->renderPipeline->addCallbackStep([&]() {
            quad = env->systemManager->getSystem<MeshFactory>()->generateQuad();

            envBuffer = Ref<Buffer>::make();
            envBuffer->init(nullptr, 0, sizeof(envData), BufferType::UNIFORM_BUFFER, true);
        });
    }
    
    void OutlineRenderer::shutdown() {
        env->renderPipeline->freeOnThread(outlineShader);
        env->renderPipeline->freeOnThread(meshShader);
        env->renderPipeline->freeOnThread(quad);
        env->renderPipeline->freeOnThread(envBuffer);
        env->renderPipeline->freeOnThread(meshesFrameBuffer);
        env->renderPipeline->freeOnThread(outlineFrameBuffer);

        outlineShader = nullptr;
        meshShader = nullptr;
        quad = nullptr;
        envBuffer = nullptr;
        meshesFrameBuffer = nullptr;
        outlineFrameBuffer = nullptr;

        drawList.reset();
        batches.clear();
    }

    void OutlineRenderer::submit(const glm::mat4& transform, Mesh* mesh) {
        DrawList::Entry entry;
        entry.shader = meshShader.get();
        entry.mesh = mesh;
        entry.transform = transform;
        entry.material = material.get();
        entry.color = outlineColor;
        entry.id = -1;
        
        drawList.add(entry);
    }

    void OutlineRenderer::submitBatches(const glm::mat4& viewProjection) {
        //prepare frame buffer
        if (meshesFrameBuffer == nullptr) {
            env->renderPipeline->freeOnThread(meshesFrameBuffer);
            env->renderPipeline->addCallbackStep([&]() {
                meshesFrameBuffer = Ref<FrameBuffer>::make();
                meshesFrameBuffer->init(0, 0, { { COLOR, Color(0, 0, 0, 0), "Outline Meshes" } });
            });
            return;
        }
        else {
            if (env->viewport->size != glm::ivec2(meshesFrameBuffer->getSize())) {
                env->renderPipeline->addCallbackStep([meshesFrameBuffer = meshesFrameBuffer]() {
                    meshesFrameBuffer->resize(env->viewport->size.x, env->viewport->size.y);
                });
            }
            env->renderPipeline->addCallbackStep([meshesFrameBuffer = meshesFrameBuffer]() {
                meshesFrameBuffer->clear();
            });
        }

        if (outlineFrameBuffer == nullptr) {
            env->renderPipeline->freeOnThread(outlineFrameBuffer);
            env->renderPipeline->addCallbackStep([&]() {
                outlineFrameBuffer = Ref<FrameBuffer>::make();
                outlineFrameBuffer->init(0, 0, { { COLOR, Color(0, 0, 0, 0), "Outlines" } });
                });
            return;
        }
        else {
            if (env->viewport->size != glm::ivec2(outlineFrameBuffer->getSize())) {
                env->renderPipeline->addCallbackStep([outlineFrameBuffer = outlineFrameBuffer]() {
                    outlineFrameBuffer->resize(env->viewport->size.x, env->viewport->size.y);
                });
            }
            env->renderPipeline->addCallbackStep([outlineFrameBuffer = outlineFrameBuffer]() {
                outlineFrameBuffer->clear();
            });
        }

        envData.viewProjection = viewProjection;
        env->renderPipeline->addCallbackStep([&]() {
            envBuffer->setData(&envData, sizeof(envData));
        });

        //submit draw list
        drawList.sort();
        drawList.submit(batches);
        drawList.reset();
        
        //submit batches
        for (auto& i : batches.batches) {
            for (auto& batch : i.second) {
                if (batch.second) {
                    if (batch.second->isInitialized()) {
                        batch.second->defaultTexture = nullptr;
                        batch.second->environmentBuffer = envBuffer;
                        batch.second->submit(meshesFrameBuffer.get(), RenderPipeline::GEOMETRY);
                        batch.second->reset();
                    }
                }
            }
        }

        env->renderPipeline->addCommandStep(RenderPipeline::Command::DEPTH_OFF, RenderPipeline::POST_PROCESSING);
        env->renderPipeline->addCommandStep(RenderPipeline::Command::BLEND_OFF, RenderPipeline::POST_PROCESSING);
        env->renderPipeline->addCommandStep(RenderPipeline::Command::CULL_OFF, RenderPipeline::POST_PROCESSING);

        //submit outline shader pass
        auto dc = env->renderPipeline->addDrawCallStep(RenderPipeline::RenderPass::POST_PROCESSING, false);
        dc->shader = outlineShader.get();
        dc->textures.push_back(meshesFrameBuffer->getAttachment(COLOR).get());
        dc->frameBuffer = outlineFrameBuffer.get();
        dc->vertexArray = &quad->vertexArray;

        Transform quadTransform;
        quadTransform.rotation.x = glm::radians(90.0f);
        quadTransform.scale = { 2, 2, -2 };

        dc->shaderState = Ref<ShaderState>::make();
        dc->shaderState->set("uTransform", quadTransform.calculateLocalMatrix());
        dc->shaderState->set("uProjection", glm::mat4(1));
        dc->shaderState->set("uColor", outlineColor.vec());
        dc->shaderState->set("steps", 1);
    }

    Ref<FrameBuffer>& OutlineRenderer::getFrameBuffer() {
        return outlineFrameBuffer;
    }

}