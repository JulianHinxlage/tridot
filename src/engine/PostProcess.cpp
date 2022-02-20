//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "core/core.h"
#include "render/RenderPipeline.h"
#include "render/ShaderState.h"
#include "engine/AssetManager.h"
#include "render/Window.h"

namespace tri {

	class PostProcess : public System {
	public:
		void startup() override {
            env->signals->postStartup.addCallback("PostProcess", []() {

                if (!env->editor) {
                    env->renderPipeline->defaultFrameBufferSpecs = {
                        {COLOR, env->window->getBackgroundColor()},
                        {DEPTH, Color(0)},
                    };

                    auto draw = env->renderPipeline->getPass("draw to screen");
                    auto call = draw->addDrawCall("draw", true);
                    call->shader = env->assets->get<Shader>("shaders/base.glsl").get();
                    call->textures.push_back(nullptr);

                    draw->addCallback("link frame buffer", [call]() {
                        Ref<FrameBuffer> fb = env->renderPipeline->getPass("geometry")->getOutputFrameBuffer();
                        if (fb) {
                            call->textures[0] = fb->getAttachment(COLOR).get();
                        }
                    }, true);
                }

                Ref<FrameBuffer> tmpFrameBuffer = Ref<FrameBuffer>::make();
                tmpFrameBuffer->init(0, 0, { { COLOR, Color::transparent } });

                auto pp = env->renderPipeline->getPass("post processing");


                //blur
                auto blur = pp->getPass("blur", true);
                blur->active = false;
                blur->addCommand("clear", CLEAR, true)->frameBuffer = tmpFrameBuffer;
                blur->addCommand("resize", RESIZE, true)->frameBuffer = tmpFrameBuffer;
                blur->addCommand("blend on", BLEND_ON, true);
                blur->addCommand("depth off", DEPTH_OFF, true);

                auto hblur = blur->addDrawCall("hblur", true);
                hblur->shader = env->assets->get<Shader>("shaders/blur.glsl").get();
                hblur->shaderState = Ref<ShaderState>::make();
                hblur->shaderState->set("steps", 4);
                hblur->shaderState->set("spread", glm::vec2(0, 1));
                hblur->frameBuffer = tmpFrameBuffer;
                hblur->textures.push_back(nullptr);

                auto vblur = blur->addDrawCall("vblur", true);
                vblur->shader = env->assets->get<Shader>("shaders/blur.glsl").get();
                vblur->shaderState = Ref<ShaderState>::make();
                vblur->shaderState->set("steps", 4);
                vblur->shaderState->set("spread", glm::vec2(1, 0));
                vblur->textures.push_back(tmpFrameBuffer->getAttachment(COLOR).get());
                vblur->frameBuffer = nullptr;

                blur->addCommand("depth on", DEPTH_ON, true);


                //gamma correction
                auto gammaCorrection = pp->getPass("gamma correction");
                gammaCorrection->active = false;
                gammaCorrection->addCommand("clear", CLEAR, true)->frameBuffer = tmpFrameBuffer;
                gammaCorrection->addCommand("resize", RESIZE, true)->frameBuffer = tmpFrameBuffer;
                gammaCorrection->addCommand("blend on", BLEND_ON, true);
                gammaCorrection->addCommand("depth off", DEPTH_OFF, true);

                auto gamma = gammaCorrection->addDrawCall("gamma", true);
                gamma->shader = env->assets->get<Shader>("shaders/gammaCorrection.glsl").get();
                gamma->shaderState = Ref<ShaderState>::make();
                gamma->shaderState->set("gamma", 2.2f);
                gamma->frameBuffer = tmpFrameBuffer;
                gamma->textures.push_back(nullptr);

                auto gammaSwap = gammaCorrection->addDrawCall("gamma swap", true);
                gammaSwap->shader = env->assets->get<Shader>("shaders/base.glsl").get();
                gammaSwap->frameBuffer = nullptr;
                gammaSwap->textures.push_back(tmpFrameBuffer->getAttachment(COLOR).get());
                
                gammaCorrection->addCommand("blend on", DEPTH_ON, true);


                //frame buffer link
                pp->addCallback("link frame buffer", [hblur, vblur, gamma, gammaSwap]() {
                    Ref<FrameBuffer> fb = env->renderPipeline->getPass("geometry")->getOutputFrameBuffer();
                    if (fb) {
                        hblur->textures[0] = fb->getAttachment(COLOR).get();
                        gamma->textures[0] = fb->getAttachment(COLOR).get();
                    }
                    vblur->frameBuffer = fb;
                    gammaSwap->frameBuffer = fb;
                }, true);


            });

		}
	};

	TRI_REGISTER_SYSTEM(PostProcess);

}