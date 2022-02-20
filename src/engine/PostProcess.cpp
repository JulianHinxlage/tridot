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

                FrameBufferAttachmentSpec ids;
                ids.type = (TextureAttachment)(COLOR + 1);
                ids.clearColor = Color::white;
                ids.mipMapping = false;
                ids.name = "ids";

                FrameBufferAttachmentSpec emissive;
                emissive.type = (TextureAttachment)(COLOR + 2);
                emissive.clearColor = Color::black;
                emissive.name = "emissive";
                emissive.textureFormat = TextureFormat::RGB8;

                FrameBufferAttachmentSpec normals;
                normals.type = (TextureAttachment)(COLOR + 3);
                normals.clearColor = Color::black;
                normals.name = "normals";
                normals.textureFormat = TextureFormat::RGB8;

                env->renderPipeline->defaultFrameBufferSpecs = {
                    {COLOR, env->window->getBackgroundColor()},
                    {DEPTH, Color(0)},
                    ids,
                    emissive,
                    normals,
                };

                if (!env->editor) {
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

                Ref<FrameBuffer> buffer = Ref<FrameBuffer>::make();
                buffer->init(0, 0, { { COLOR, Color::transparent } });

                Ref<FrameBuffer> bloomBuffer = Ref<FrameBuffer>::make();
                bloomBuffer->init(0, 0, { { COLOR, Color::transparent } });

                auto pp = env->renderPipeline->getPass("post processing");


                //bloom
                auto bloom = pp->getPass("bloom", true);
                bloom->active = true;
                bloom->addCommand("clear", CLEAR, true)->frameBuffer = bloomBuffer;
                bloom->addCommand("resize", RESIZE, true)->frameBuffer = bloomBuffer;
                bloom->addCommand("clear", CLEAR, true)->frameBuffer = buffer;
                bloom->addCommand("resize", RESIZE, true)->frameBuffer = buffer;
                bloom->addCommand("blend on", BLEND_ON, true);
                bloom->addCommand("depth off", DEPTH_OFF, true);

                auto hbloom = bloom->addDrawCall("hbloom", true);
                hbloom->shader = env->assets->get<Shader>("shaders/blur.glsl").get();
                hbloom->shaderState = Ref<ShaderState>::make();
                hbloom->shaderState->set("steps", 30);
                hbloom->shaderState->set("spread", glm::vec2(0, 1));
                hbloom->shaderState->set("gaussian", true);
                hbloom->frameBuffer = buffer;
                hbloom->textures.push_back(nullptr);

                auto vbloom = bloom->addDrawCall("vbloom", true);
                vbloom->shader = env->assets->get<Shader>("shaders/blur.glsl").get();
                vbloom->shaderState = Ref<ShaderState>::make();
                vbloom->shaderState->set("steps", 30);
                vbloom->shaderState->set("spread", glm::vec2(1, 0));
                vbloom->shaderState->set("gaussian", true);
                vbloom->textures.push_back(buffer->getAttachment(COLOR).get());
                vbloom->frameBuffer = bloomBuffer;

                auto cbloom = bloom->addDrawCall("bloom combine", true);
                cbloom->shader = env->assets->get<Shader>("shaders/bloom.glsl").get();
                cbloom->textures.push_back(nullptr);//albedo
                cbloom->textures.push_back(nullptr);//emissive
                cbloom->textures.push_back(bloomBuffer->getAttachment(COLOR).get());//bloom
                cbloom->shaderState = Ref<ShaderState>::make();
                cbloom->shaderState->set("bloomIntesity", 1.0f);
                cbloom->frameBuffer = buffer;

                auto bloomSwap = bloom->addDrawCall("bloom swap", true);
                bloomSwap->shader = env->assets->get<Shader>("shaders/base.glsl").get();
                bloomSwap->frameBuffer = nullptr;
                bloomSwap->textures.push_back(buffer->getAttachment(COLOR).get());

                bloom->addCommand("depth on", DEPTH_ON, true);


                //blur
                auto blur = pp->getPass("blur", true);
                blur->active = false;
                blur->addCommand("clear", CLEAR, true)->frameBuffer = buffer;
                blur->addCommand("resize", RESIZE, true)->frameBuffer = buffer;
                blur->addCommand("blend on", BLEND_ON, true);
                blur->addCommand("depth off", DEPTH_OFF, true);

                auto hblur = blur->addDrawCall("hblur", true);
                hblur->shader = env->assets->get<Shader>("shaders/blur.glsl").get();
                hblur->shaderState = Ref<ShaderState>::make();
                hblur->shaderState->set("steps", 4);
                hblur->shaderState->set("spread", glm::vec2(0, 1));
                hblur->shaderState->set("gaussian", true);
                hblur->frameBuffer = buffer;
                hblur->textures.push_back(nullptr);

                auto vblur = blur->addDrawCall("vblur", true);
                vblur->shader = env->assets->get<Shader>("shaders/blur.glsl").get();
                vblur->shaderState = Ref<ShaderState>::make();
                vblur->shaderState->set("steps", 4);
                vblur->shaderState->set("spread", glm::vec2(1, 0));
                vblur->shaderState->set("gaussian", true);
                vblur->textures.push_back(buffer->getAttachment(COLOR).get());
                vblur->frameBuffer = nullptr;

                blur->addCommand("depth on", DEPTH_ON, true);


                //gamma correction
                auto gammaCorrection = pp->getPass("gamma correction");
                gammaCorrection->active = false;
                gammaCorrection->addCommand("clear", CLEAR, true)->frameBuffer = buffer;
                gammaCorrection->addCommand("resize", RESIZE, true)->frameBuffer = buffer;
                gammaCorrection->addCommand("blend on", BLEND_ON, true);
                gammaCorrection->addCommand("depth off", DEPTH_OFF, true);

                auto gamma = gammaCorrection->addDrawCall("gamma", true);
                gamma->shader = env->assets->get<Shader>("shaders/gammaCorrection.glsl").get();
                gamma->shaderState = Ref<ShaderState>::make();
                gamma->shaderState->set("gamma", 2.2f);
                gamma->shaderState->set("exposure", 1.0f);
                gamma->frameBuffer = buffer;
                gamma->textures.push_back(nullptr);

                auto gammaSwap = gammaCorrection->addDrawCall("gamma swap", true);
                gammaSwap->shader = env->assets->get<Shader>("shaders/base.glsl").get();
                gammaSwap->frameBuffer = nullptr;
                gammaSwap->textures.push_back(buffer->getAttachment(COLOR).get());
                
                gammaCorrection->addCommand("blend on", DEPTH_ON, true);


                //frame buffer link
                pp->addCallback("link frame buffer", [hblur, vblur, gamma, gammaSwap, hbloom, cbloom, bloomSwap]() {
                    Ref<FrameBuffer> fb = env->renderPipeline->getPass("geometry")->getOutputFrameBuffer();
                    if (fb) {
                        hblur->textures[0] = fb->getAttachment(COLOR).get();
                        gamma->textures[0] = fb->getAttachment(COLOR).get();
                        hbloom->textures[0] = fb->getAttachment((TextureAttachment)(COLOR + 2)).get();//emissive
                        cbloom->textures[0] = fb->getAttachment((TextureAttachment)(COLOR)).get();
                        cbloom->textures[1] = fb->getAttachment((TextureAttachment)(COLOR + 2)).get();//emissive
                    }
                    vblur->frameBuffer = fb;
                    gammaSwap->frameBuffer = fb;
                    bloomSwap->frameBuffer = fb;
                }, true);


            });

		}
	};

	TRI_REGISTER_SYSTEM(PostProcess);

}