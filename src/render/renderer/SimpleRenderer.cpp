//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "SimpleRenderer.h"
#include "entity/World.h"
#include "engine/AssetManager.h"
#include "engine/MeshComponent.h"
#include "engine/Transform.h"
#include "engine/Camera.h"
#include "window/RenderContext.h"
#include "window/Viewport.h"
#include "RenderPipeline.h"
#include <GL/glew.h>
#include <tracy/TracyOpenGL.hpp>

namespace tri {

	//TRI_SYSTEM(SimpleRenderer);

    void SimpleRenderer::init() {
        auto* job = env->jobManager->addJob("Renderer");
        job->addSystem<SimpleRenderer>();
    }

	void SimpleRenderer::startup() {
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

            defaultShader = env->assetManager->get<Shader>("shaders/simple.glsl");
            projection = glm::mat4(1);
        });
	}

    void SimpleRenderer::shutdown() {
        env->renderPipeline->freeOnThread(defaultTexture);
        env->renderPipeline->freeOnThread(defaultMaterial);
        env->renderPipeline->freeOnThread(defaultMesh);
        env->renderPipeline->freeOnThread(defaultShader);
        env->renderPipeline->freeOnThread(frameBuffer);

        defaultTexture = nullptr;
        defaultMaterial = nullptr;
        defaultMesh = nullptr;
        defaultShader = nullptr;
        frameBuffer = nullptr;
    }

    void SimpleRenderer::tick() {
        if (!defaultShader) {
            return;
        }

        if (!env->viewport->displayInWindow) {
            env->renderPipeline->addCallbackStep([]() {
                if (!env->viewport->frameBuffer) {
                    env->viewport->frameBuffer = Ref<FrameBuffer>::make();
                    env->viewport->frameBuffer->setAttachment({ COLOR, Color(0, 0, 0, 0) });
                    env->viewport->frameBuffer->setAttachment({ DEPTH, color::white });
                    env->viewport->frameBuffer->setAttachment({ (TextureAttachment)(COLOR + 1), color::white });//ID buffer
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
                        c.output = Ref<FrameBuffer>::make();
                        c.output->setAttachment({ COLOR, Color(0, 0, 0, 0) });
                        c.output->setAttachment({ DEPTH, color::white });
                        c.output->setAttachment({ (TextureAttachment)(COLOR + 1), color::white });//ID buffer
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
            }
        });
    }

    void SimpleRenderer::submit(const glm::mat4& transform, Mesh* mesh, Material* material, Color color, EntityId id) {
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
        Texture* texture = material->texture.get();
        if (!texture) {
            texture = defaultTexture.get();
        }

        if (shader->getId() == 0) {
            return;
        }

        Ref<RenderPipeline::StepDrawCall> dc = env->renderPipeline->addDrawCallStep(RenderPipeline::GEOMETRY);

        dc->shader = shader;
        dc->frameBuffer = frameBuffer.get();
        dc->vertexArray = &mesh->vertexArray;
        dc->textures.push_back(texture);

        int texId = 0;
        Color idColor(id | (0xff << 24));
        dc->shaderState = dc->shaderState.make();
        dc->shaderState->set("uTransform", transform);
        dc->shaderState->set("uProjection", projection);
        dc->shaderState->set("uTextures", &texId, 1);
        dc->shaderState->set("uColor", color.vec() * material->color.vec());
        dc->shaderState->set("uId", idColor.vec());
    }

}
