//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Skybox.h"
#include "AssetManager.h"
#include "render/RenderContext.h"
#include "entity/Scene.h"
#include "render/Renderer.h"
#include "render/Window.h"
#include "Transform.h"
#include "Camera.h"
#include "render/RenderPipeline.h"
#include "render/ShaderState.h"
#include "core/core.h"

namespace tri {

    TRI_REGISTER_COMPONENT(Skybox);
    TRI_REGISTER_MEMBER(Skybox, texture);
    TRI_REGISTER_MEMBER(Skybox, useSky);
    TRI_REGISTER_MEMBER(Skybox, useIBL);
    TRI_REGISTER_MEMBER(Skybox, color);
    TRI_REGISTER_MEMBER(Skybox, intensity);

    TRI_STARTUP_CALLBACK("Skybox") {
        env->signals->update.callbackOrder({"Camera", "Skybox", "MeshComponent"});
    }

    void generateIrradianceMap(Skybox& skybox) {
        env->renderPipeline->getPass("skybox")->addCallback("generate irradiance map", [&]() {
            TRI_PROFILE("Skybox generate irradiance map");
            Ref<Shader> shader = env->assets->get<Shader>("shaders/blur.glsl", AssetManager::SYNCHRONOUS);

            Ref<FrameBuffer> frameBuffer1 = Ref<FrameBuffer>::make();
            Ref<FrameBuffer> frameBuffer2 = Ref<FrameBuffer>::make();

            frameBuffer1->setAttachment({ COLOR });
            frameBuffer2->setAttachment({ COLOR });
            frameBuffer1->resize(skybox.texture->getWidth(), skybox.texture->getHeight());
            frameBuffer2->resize(skybox.texture->getWidth(), skybox.texture->getHeight());

            auto blur = [&](Ref<FrameBuffer>& frameBuffer, const Ref<Texture>& texture, bool horizontal) {
                shader->bind();
                texture->bind(0);
                int i = 0;
                shader->set("uTextures", &i, 1);
                frameBuffer->bind();
                shader->set("steps", 50);
                if (horizontal) {
                    shader->set("spread", glm::vec2(1.0f / (float)texture->getWidth(), 0.0f));
                }
                else {
                    shader->set("spread", glm::vec2(0.0f, 1.0f / (float)texture->getHeight()));
                }
                env->renderPipeline->getQuad()->vertexArray.submit();
            };


            blur(frameBuffer1, skybox.texture, true);
            blur(frameBuffer2, frameBuffer1->getAttachment(COLOR), false);
            for (int i = 0; i < 10; i++) {
                blur(frameBuffer1, frameBuffer2->getAttachment(COLOR), true);
                blur(frameBuffer2, frameBuffer1->getAttachment(COLOR), false);
            }

            skybox.irradianceMap = frameBuffer2->getAttachment(COLOR);
            skybox.texture->setCubeMap(true);
            skybox.irradianceMap->setCubeMap(false);

        });

    }

    TRI_UPDATE_CALLBACK("Skybox") {
        auto pass = env->renderPipeline->getPass("skybox");
        pass->addCommand("cull off", CULL_OFF);
        pass->addCommand("depth off", DEPTH_OFF);

        int skyboxIndex = 0;
        env->scene->view<Camera>().each([&](Camera& camera) {
            if (camera.active) {
                env->scene->view<Skybox>().each([&](EntityId id, Skybox& skybox) {

                    if (skybox.texture) {
                        if (skybox.texture->getId() != 0) {
                            if (skybox.texture->getType() != TextureType::TEXTURE_CUBE_MAP) {
                                generateIrradianceMap(skybox);
                            }
                            else {
                                if (skybox.useSky) {

                                    auto step = pass->addDrawCall("skybox " + std::to_string(skyboxIndex++));
                                    step->shader = env->assets->get<Shader>("shaders/skybox.glsl").get();
                                    step->mesh = env->assets->get<Mesh>("models/cube.obj").get();
                                    step->textures.push_back(skybox.texture.get());
                                    step->shaderState = Ref<ShaderState>::make();

                                    Transform transform;
                                    transform.decompose(camera.transform);
                                    transform.scale = { 1, 1, 1 };
                                    transform.rotation = { 0, 0, 0 };
                                    if (env->scene->hasComponent<Transform>(id)) {
                                        transform.rotation = env->scene->getComponent<Transform>(id).rotation;
                                    }
                                    
                                    step->shaderState->set("uProjection", camera.viewProjection);
                                    step->shaderState->set("uTransform", transform.calculateLocalMatrix());
                                    step->shaderState->set("uColor", skybox.color.vec());
                                    step->frameBuffer = camera.output.get();

                                }
                            }
                        }
                    }

                });
            }
        });

        pass->addCommand("cull on", CULL_ON);
        pass->addCommand("depth on", DEPTH_ON);
    }

}