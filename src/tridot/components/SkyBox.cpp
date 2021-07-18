//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "SkyBox.h"
#include "tridot/engine/Engine.h"
#include "tridot/render/Camera.h"
#include "tridot/render/RenderContext.h"

namespace tridot {

    SkyBox::SkyBox() {
        texture = nullptr;
        irradianceTexture = nullptr;
        drawSkybox = true;
        useEnvironmentMap = true;
        intensity = 1.0f;
    }

    TRI_REGISTER_TYPE(SkyBox)
    TRI_REGISTER_MEMBER4(SkyBox, texture, drawSkybox, useEnvironmentMap, intensity)
    TRI_COMPONENT(SkyBox)

    Ref<Texture> createIrradianceMap(Ref<Texture> &texture){
        Ref<Shader> shader = engine.resources.get<Shader>("shaders/blur.glsl", ResourceManager::SYNCHRONOUS);
        Ref<FrameBuffer> frameBuffer1 = Ref<FrameBuffer>::make();
        Ref<FrameBuffer> frameBuffer2 = Ref<FrameBuffer>::make();
        frameBuffer1->setAttachment({COLOR});
        frameBuffer2->setAttachment({COLOR});
        frameBuffer1->resize(texture->getWidth(), texture->getHeight());
        frameBuffer2->resize(texture->getWidth(), texture->getHeight());
        auto blur = [&](Ref<FrameBuffer> &frameBuffer, const Ref<Texture> &texture, bool horizontal) {
            shader->bind();
            shader->set("steps", (int)50);
            if(horizontal){
                shader->set("spread", glm::vec2(1.0f / (float)texture->getWidth(), 0.0f));
            }else{
                shader->set("spread", glm::vec2(0.0f, 1.0f / (float)texture->getHeight()));
            }
            engine.renderer.begin(glm::mat4(1), {0, 0, 0}, frameBuffer);
            engine.renderer.submit({{0, 0, 0}, {2, 2, 2}}, texture.get(), nullptr, shader.get());
            engine.renderer.end();
        };
        blur(frameBuffer1, texture, true);
        blur(frameBuffer2, frameBuffer1->getAttachment(COLOR), false);

        for(int i = 0; i < 10; i++){
            blur(frameBuffer1, frameBuffer2->getAttachment(COLOR), true);
            blur(frameBuffer2, frameBuffer1->getAttachment(COLOR), false);
        }

        return frameBuffer2->getAttachment(COLOR);
    }

    TRI_UPDATE("skybox"){
        auto render = [&](SkyBox &skybox, Ref<FrameBuffer> &target, glm::vec3 position, const glm::mat4 &projection){
            Ref<Mesh> mesh = engine.resources.get<Mesh>("cube");
            Ref<Shader> shader = engine.resources.get<Shader>("shaders/skybox.glsl");
            if(shader->getId() != 0 && skybox.texture.get() != nullptr){
                shader->set("uEnvironmentMap", (int)0);
                skybox.texture->bind(0);
                engine.renderer.begin(projection, {0, 0, 0}, target);
                engine.renderer.submit({position, {100, 100, 100}}, nullptr, mesh.get(), shader.get());
                engine.renderer.end();
            }
        };
        RenderContext::setDepth(false);
        engine.view<SkyBox>().each([&](SkyBox &skybox){
            if(skybox.drawSkybox || skybox.useEnvironmentMap) {
                if (skybox.texture.get() != nullptr) {
                    if (skybox.texture->getId() != 0) {
                        if (skybox.texture->getType() != TEXTURE_CUBE_MAP) {
                            skybox.irradianceTexture = createIrradianceMap(skybox.texture);
                            skybox.texture->setCubeMap(true);
                            skybox.irradianceTexture->setCubeMap(false);
                        }
                    }
                }
            }
            if(skybox.drawSkybox){
                engine.view<PerspectiveCamera, Transform>().each([&](PerspectiveCamera &camera, Transform &transform){
                    render(skybox, camera.target, transform.position, camera.getProjection() * glm::inverse(transform.getMatrix()));
                });
                engine.view<OrthographicCamera, Transform>().each([&](OrthographicCamera &camera, Transform &transform){
                    render(skybox, camera.target, transform.position, camera.getProjection() * glm::inverse(transform.getMatrix()));
                });
            }
        });
        RenderContext::setDepth(true);
        RenderContext::flush(true);
    }

}
