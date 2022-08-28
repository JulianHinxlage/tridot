//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Renderer.h"
#include "entity/World.h"
#include "engine/AssetManager.h"
#include "engine/MeshComponent.h"
#include "engine/Transform.h"
#include "engine/Camera.h"
#include "window/RenderContext.h"
#include "window/Viewport.h"
#include "RenderPipeline.h"
#include "engine/RuntimeMode.h"
#include <GL/glew.h>
#include <tracy/TracyOpenGL.hpp>

namespace tri {

    TRI_SYSTEM(Renderer);

    void Renderer::init() {
        auto* job = env->jobManager->addJob("Renderer");
        job->addSystem<Renderer>();
        env->runtimeMode->setActiveSystem(RuntimeMode::EDIT, "Renderer", true);
        env->runtimeMode->setActiveSystem(RuntimeMode::PAUSED, "Renderer", true);
    }

    void Renderer::startup() {
        env->renderPipeline->addCallbackStep([&]() {
            Image image;
            image.init(1, 1, 4, 8);
            image.set(0, 0, color::white);
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

            defaultShader = env->assetManager->get<Shader>("shaders/geometry.glsl");
            projection = glm::mat4(1);
            envBuffer = Ref<Buffer>::make();
            envBuffer->init(nullptr, 0, sizeof(envData), BufferType::UNIFORM_BUFFER, true);
        });
    }

    void Renderer::shutdown() {
        env->renderPipeline->freeOnThread(defaultTexture);
        env->renderPipeline->freeOnThread(defaultMaterial);
        env->renderPipeline->freeOnThread(defaultMesh);
        env->renderPipeline->freeOnThread(defaultShader);
        env->renderPipeline->freeOnThread(frameBuffer);
        env->renderPipeline->freeOnThread(envBuffer);

        defaultTexture = nullptr;
        defaultMaterial = nullptr;
        defaultMesh = nullptr;
        defaultShader = nullptr;
        frameBuffer = nullptr;
        envBuffer = nullptr;

        drawList.reset();

        for (auto& i : batches.batches) {
            for (auto& j : i.second) {
                env->renderPipeline->freeOnThread(j.second);
            }
        }
        batches.clear();
    }

    void Renderer::tick() {
        if (!defaultShader) {
            return;
        }

        if (!env->viewport->displayInWindow) {
            env->renderPipeline->addCallbackStep([&]() {
                if (!env->viewport->frameBuffer) {
                    setupFrameBuffer(env->viewport->frameBuffer);
                }
                if (env->viewport->size != glm::ivec2(env->viewport->frameBuffer->getSize())) {
                    env->viewport->frameBuffer->resize(env->viewport->size.x, env->viewport->size.y);
                }
                });
            frameBuffer = env->viewport->frameBuffer;
        }
        else {
            frameBuffer = nullptr;
        }

        env->renderPipeline->addCommandStep(RenderPipeline::Command::DEPTH_ON);

        bool hasPrimary = false;
        env->world->each<Camera>([&](Camera& c) {
            if (c.active) {

                if (c.isPrimary && !hasPrimary) {
                    env->renderPipeline->freeOnThread(c.output);
                    c.output = env->viewport->frameBuffer;
                    hasPrimary = true;
                }
                else if (!c.output || c.output == env->viewport->frameBuffer) {
                    env->renderPipeline->addCallbackStep([&]() {
                        setupFrameBuffer(c.output);
                    });
                }
                frameBuffer = c.output;
                if (frameBuffer) {
                    if (env->viewport->size != glm::ivec2(frameBuffer->getSize())) {
                        env->renderPipeline->addCallbackStep([frameBuffer = frameBuffer]() {
                            frameBuffer->resize(env->viewport->size.x, env->viewport->size.y);
                        });
                    }

                    env->renderPipeline->addCallbackStep([frameBuffer = frameBuffer]() {
                        TracyGpuZone("clear");
                        frameBuffer->clear();
                    });
                }

                if (env->viewport->size.y != 0) {
                    c.aspectRatio = (float)env->viewport->size.x / (float)env->viewport->size.y;
                }
                projection = c.viewProjection;

                env->world->each<const Transform, const MeshComponent>([&](EntityId id, const Transform& t, const MeshComponent& m) {
                    submit(t.getMatrix(), m.mesh.get(), m.material.get(), m.color, id);
                });

                
                envData.projection = c.projection;
                envData.viewProjection = c.viewProjection;
                envData.view = c.view;
                Transform eye;
                eye.decompose(c.transform);
                envData.eyePosition = eye.position;
                envData.lightCount = 0;
                envData.radianceMapIndex = -1;
                envData.irradianceMapIndex = -1;
                env->renderPipeline->addCallbackStep([&]() {
                    envBuffer->setData(&envData, sizeof(envData));
                });

                drawList.sort();
                drawList.submit(batches);
                drawList.reset();
                for (auto& i : batches.batches) {
                    for (auto& j : i.second) {
                        if (j.second->isInitialized()) {
                            j.second->environmentBuffer = envBuffer;
                            j.second->defaultTexture = defaultTexture.get();
                            j.second->submit(frameBuffer.get(), RenderPipeline::OPAQUE);
                            j.second->reset();
                        }
                    }
                }
            }
        });
    }

    void Renderer::submit(const glm::mat4& transform, Mesh* mesh, Material* material, Color color, EntityId id) {
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
        if (shader->getId() == 0) {
            return;
        }

        DrawList::Entry entry;
        entry.shader = shader;
        entry.mesh = mesh;
        entry.transform = transform;
        entry.material = material;
        entry.color = color.vec();
        entry.id = id;
        drawList.add(entry);
    }

    void Renderer::setupFrameBuffer(Ref<FrameBuffer> &frameBuffer) {
        FrameBufferAttachmentSpec albedo;
        albedo.type = (TextureAttachment)(COLOR);
        albedo.clearColor = color::transparent;
        albedo.mipMapping = false;
        albedo.name = "Albedo";
        albedo.textureFormat = TextureFormat::RGBA8;

        FrameBufferAttachmentSpec id;
        id.type = (TextureAttachment)(COLOR + 1);
        id.clearColor = color::white;
        id.mipMapping = false;
        id.name = "ID";

        FrameBufferAttachmentSpec normal;
        normal.type = (TextureAttachment)(COLOR + 2);
        normal.clearColor = color::black;
        normal.name = "Normal";
        normal.textureFormat = TextureFormat::RGB8;

        FrameBufferAttachmentSpec position;
        position.type = (TextureAttachment)(COLOR + 3);
        position.clearColor = color::black;
        position.name = "Position";
        position.textureFormat = TextureFormat::RGB32F;
        position.sRepeat = false;
        position.tRepeat = false;
        position.useBorderColor = true;
        position.borderColor = color::black;

        FrameBufferAttachmentSpec rme;
        rme.type = (TextureAttachment)(COLOR + 4);
        rme.clearColor = color::black;
        rme.name = "RME";
        rme.textureFormat = TextureFormat::RGB8;

        FrameBufferAttachmentSpec depth;
        depth.type = (TextureAttachment)(DEPTH);
        depth.clearColor = color::white;
        depth.name = "Depth";
        depth.textureFormat = TextureFormat::DEPTH24STENCIL8;
        
        frameBuffer = Ref<FrameBuffer>::make();
        frameBuffer->init(0, 0, { albedo, id, normal, position, rme, depth });
    }

}
