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

    Ref<Texture> createIrradianceMap(Ref<Texture>& texture) {
        Ref<Shader> shader = env->assets->get<Shader>("shaders/blur.glsl", true);
        Ref<FrameBuffer> frameBuffer1 = Ref<FrameBuffer>::make();
        Ref<FrameBuffer> frameBuffer2 = Ref<FrameBuffer>::make();
        frameBuffer1->setAttachment({ COLOR });
        frameBuffer2->setAttachment({ COLOR });
        frameBuffer1->resize(texture->getWidth(), texture->getHeight());
        frameBuffer2->resize(texture->getWidth(), texture->getHeight());
        auto blur = [&](Ref<FrameBuffer>& frameBuffer, const Ref<Texture>& texture, bool horizontal) {
            shader->bind();
            shader->set("steps", (int)50);
            if (horizontal) {
                shader->set("spread", glm::vec2(1.0f / (float)texture->getWidth(), 0.0f));
            }
            else {
                shader->set("spread", glm::vec2(0.0f, 1.0f / (float)texture->getHeight()));
            }
            env->renderer->beginScene(glm::mat4(1), { 0, 0, 0 });

            Material mat;
            mat.shader = shader;
            mat.texture = texture;
            Ref<Mesh> plane = env->assets->get<Mesh>("models/plane.obj", true);

            Transform transform;
            transform.scale = { 2, 2, 2 };
            transform.rotation = { glm::radians(90.0f), 0, 0 };

            env->renderer->submit(transform.calculateLocalMatrix(), { 0, 0, 0 }, plane.get(), &mat);
            env->renderer->drawScene(frameBuffer);
            env->renderer->resetScene();
        };
        blur(frameBuffer1, texture, true);
        blur(frameBuffer2, frameBuffer1->getAttachment(COLOR), false);
        
        for (int i = 0; i < 10; i++) {
            blur(frameBuffer1, frameBuffer2->getAttachment(COLOR), true);
            blur(frameBuffer2, frameBuffer1->getAttachment(COLOR), false);
        }
        
        return frameBuffer2->getAttachment(COLOR);
    }

    TRI_UPDATE_CALLBACK("Skybox") {
        RenderContext::setCull(false);
        RenderContext::setDepth(false);
        env->scene->view<Camera, Transform>().each([](Camera& camera, Transform &cameraTransform) {
            if (camera.active) {
                env->scene->view<Skybox>().each([&](Skybox& skybox) {

                    if (skybox.texture) {
                        if (skybox.texture->getId() != 0) {
                            if (skybox.texture->getType() != TextureType::TEXTURE_CUBE_MAP) {
                                skybox.irradianceMap = createIrradianceMap(skybox.texture);
                                skybox.texture->setCubeMap(true);
                                skybox.irradianceMap->setCubeMap(false);
                            }
                            else {
                                if (skybox.useSky) {
      
                                    Ref<Mesh> cube = env->assets->get<Mesh>("models/cube.obj");
                                    Ref<Shader> shader = env->assets->get<Shader>("shaders/skybox.glsl");
                                    if (shader->getId() != 0) {
                                        shader->bind();
                                        shader->set("uEnvironmentMap", (int)0);
                                        skybox.texture->bind(0);

                                        env->renderer->beginScene(camera.projection, cameraTransform.position);

                                        Material mat;
                                        mat.shader = shader;
                                        Transform transform;
                                        transform.decompose(cameraTransform.getMatrix());
                                        transform.scale = { 1, 1, 1 };
                                        transform.rotation = { 0, 0, 0 };
                            
                                        env->renderer->submit(transform.calculateLocalMatrix(), transform.position, cube.get(), &mat, skybox.color);
                                        env->renderer->drawScene(camera.output);
                                        env->renderer->resetScene();
                                    }
                                    
                                }
                            }
                        }
                    }

                });
            }
        });
        RenderContext::setCull(true);
        RenderContext::setDepth(true);
    }

}
