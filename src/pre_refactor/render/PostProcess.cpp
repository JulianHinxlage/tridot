//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "core/core.h"
#include "render/RenderPipeline.h"
#include "render/ShaderState.h"
#include "engine/AssetManager.h"
#include "render/Window.h"
#include "RenderSettings.h"
#include "engine/Random.h"

namespace tri {

	class PostProcess : public System {
	public:
        bool deferredShadingLast;
        std::vector<FrameBufferAttachmentSpec> gBufferSpec;
        Ref<FrameBuffer> swapBuffer;
        Ref<FrameBuffer> ssaoBuffer;
        Ref<FrameBuffer> bloomBuffer;
        Ref<FrameBuffer> bloomBuffer2;
        Ref<FrameBuffer> bloomBuffer3;
        Ref<Texture> noise;

        void startup() {
            env->signals->postStartup.addCallback("PostProcess", [this]() {
                setupBuffers();
                setupClearBuffers();
                setupDrawToScreen();
                setupSSAO();
                setupBloom();
                setupPostProcess();
                deferredShadingLast = !env->renderSettings->deferredShadingEnabled;
            });
        }

        void update() override {
            if (deferredShadingLast != env->renderSettings->deferredShadingEnabled) {
                auto deferred = env->renderPipeline->getPass("deferred");
                auto deferred2 = env->renderPipeline->getRootPass()->getPass("deferred");
                if (env->renderSettings->deferredShadingEnabled) {
                    env->renderPipeline->defaultFrameBufferSpecs = gBufferSpec;
                    deferred->active = true;
                    deferred2->active = true;
                }
                else {
                    deferred->active = false;
                    deferred2->active = false;
                }
                deferredShadingLast = env->renderSettings->deferredShadingEnabled;
            }
        }

        void setupBuffers() {
            FrameBufferAttachmentSpec albedo;
            albedo.type = (TextureAttachment)(COLOR);
            albedo.clearColor = env->window->getBackgroundColor();
            albedo.mipMapping = false;
            albedo.name = "albedo";
            albedo.textureFormat = TextureFormat::RGBA8;

            FrameBufferAttachmentSpec ids;
            ids.type = (TextureAttachment)(COLOR + 1);
            ids.clearColor = Color::white;
            ids.mipMapping = false;
            ids.name = "ids";

            FrameBufferAttachmentSpec normals;
            normals.type = (TextureAttachment)(COLOR + 2);
            normals.clearColor = Color::black;
            normals.name = "normals";
            normals.textureFormat = TextureFormat::RGB8;

            FrameBufferAttachmentSpec position;
            position.type = (TextureAttachment)(COLOR + 3);
            position.clearColor = Color::black;
            position.name = "position";
            position.textureFormat = TextureFormat::RGB32F;
            position.sRepeat = false;
            position.tRepeat = false;
            position.useBorderColor = true;
            position.borderColor = Color::black;

            FrameBufferAttachmentSpec RME;
            RME.type = (TextureAttachment)(COLOR + 4);
            RME.clearColor = Color::black;
            RME.name = "RME";
            RME.textureFormat = TextureFormat::RGB8;

            FrameBufferAttachmentSpec depth;
            depth.type = (TextureAttachment)(DEPTH);
            depth.clearColor = Color::white;
            depth.name = "depth";
            depth.textureFormat = TextureFormat::DEPTH24STENCIL8;

            gBufferSpec = {
                albedo,
                ids,
                normals,
                position,
                RME,
                depth,
            };
            env->renderPipeline->defaultFrameBufferSpecs = gBufferSpec;

            FrameBufferAttachmentSpec emissive;
            emissive.type = (TextureAttachment)(COLOR + 1);
            emissive.clearColor = Color::black;
            emissive.name = "emissive";
            emissive.textureFormat = TextureFormat::RGB8;

            swapBuffer = Ref<FrameBuffer>::make();
            swapBuffer->init(0, 0, { albedo, emissive });

            FrameBufferAttachmentSpec bloomColor;
            bloomColor.type = (TextureAttachment)(COLOR);
            bloomColor.clearColor = Color::transparent;
            bloomColor.name = "bloom";
            bloomColor.textureFormat = TextureFormat::RGB8;
            //bloomColor.resizeFactor = { 0.5, 0.5 };
            bloomColor.magNearest = true;
            bloomColor.minNearest = true;

            bloomBuffer = Ref<FrameBuffer>::make();
            bloomBuffer->init(0, 0, { bloomColor });

            bloomBuffer2 = Ref<FrameBuffer>::make();
            bloomBuffer2->init(0, 0, { bloomColor });

            bloomBuffer3 = Ref<FrameBuffer>::make();
            bloomBuffer3->init(0, 0, { albedo });

            FrameBufferAttachmentSpec ao;
            ao.type = (TextureAttachment)(COLOR);
            ao.clearColor = Color::white;
            ao.mipMapping = false;
            ao.name = "ao";
            ao.textureFormat = TextureFormat::RED8;
            //ao.resizeFactor = { 0.5, 0.5 };
            ao.magNearest = true;
            ao.minNearest = true;

            ssaoBuffer = Ref<FrameBuffer>::make();
            ssaoBuffer->init(0, 0, { ao });
        }

        void setupDrawToScreen() {
            if (!env->editor) {
                auto draw = env->renderPipeline->getPass("draw to screen");
                auto call = draw->addDrawCall("draw", true);
                call->shader = env->assets->get<Shader>("shaders/base.glsl").get();
                call->inputs.emplace_back(COLOR, env->renderPipeline->getPass("geometry").get());
            }
        }

        void setupClearBuffers() {
            auto clear = env->renderPipeline->getPass("clear");

            clear->addCommand("clear", CLEAR, true)->frameBuffer = swapBuffer;
            clear->addCommand("resize", RESIZE, true)->frameBuffer = swapBuffer;

            clear->addCommand("clear", CLEAR, true)->frameBuffer = ssaoBuffer;
            clear->addCommand("resize", RESIZE, true)->frameBuffer = ssaoBuffer;

            clear->addCommand("clear", CLEAR, true)->frameBuffer = bloomBuffer;
            clear->addCommand("resize", RESIZE, true)->frameBuffer = bloomBuffer;
            clear->addCommand("clear", CLEAR, true)->frameBuffer = bloomBuffer2;
            clear->addCommand("resize", RESIZE, true)->frameBuffer = bloomBuffer2;
            clear->addCommand("clear", CLEAR, true)->frameBuffer = bloomBuffer3;
            clear->addCommand("resize", RESIZE, true)->frameBuffer = bloomBuffer3;
        }
        
        float lerp(float x, float y, float t) {
            return x * (1.0f - t) + y * t;
        }

        void setupSSAO() {
            auto geometry = env->renderPipeline->getPass("geometry");
            auto deferred = env->renderPipeline->getPass("deferred");

            int noiseResolution = 4;
            std::vector<Color> noiseData;
            noiseData.resize(noiseResolution * noiseResolution);
            for (int i = 0; i < noiseData.size(); i++) {
                noiseData[i] = Color(glm::vec3(env->random->getVec2(), 0.0));
            }

            Image noiseImage;
            noiseImage.init(noiseResolution, noiseResolution, 3, 8, noiseData.data(), noiseData.size());
            noise = Ref<Texture>::make();
            noise->load(noiseImage);

            auto ssaoParent = deferred->getPass("ssao", true);

            ssaoParent->addCommand("blend off", BLEND_OFF, true);

            auto ssao = ssaoParent->addDrawCall("ssao", true);
            ssao->shader = env->assets->get<Shader>("shaders/ssao.glsl").get();
            ssao->frameBuffer = ssaoBuffer;
            ssao->textures.resize(3);
            ssao->textures[2] = noise.get();
            ssao->shaderStateInput = geometry.get();

            int kernalSize = 64;
            std::vector<glm::vec3> samples(256);
            for (int i = 0; i < samples.size(); i++) {
                glm::vec3 sample = env->random->getVec3();
                sample.x = sample.x * 2.0 - 1.0;
                sample.y = sample.y * 2.0 - 1.0;
                sample = glm::normalize(sample);
                sample *= env->random->getFloat();

                float scale = (float)i / (float)kernalSize;
                scale = lerp(0.1f, 1.0f, scale * scale);
                sample *= scale;
                samples[i] = sample;
            }
            ssao->shaderState = Ref<ShaderState>::make();
            ssao->shaderState->set("samples", samples.data(), samples.size());
            ssao->shaderState->set("kernalSize", kernalSize);

            ssao->shaderState->set("sampleRadius", 1.0f);
            ssao->shaderState->set("occlusionStrength", 1.0f);
            ssao->shaderState->set("bias", 0.025f);

            ssao->inputs.emplace_back((TextureAttachment)(COLOR + 3), geometry.get()); //position
            ssao->inputs.emplace_back((TextureAttachment)(COLOR + 2), geometry.get()); //normals


            //blur
            ssaoParent->addCommand("blend on", BLEND_ON, true);
            ssaoParent->addCommand("depth off", DEPTH_OFF, true);

            auto hblur = ssaoParent->addDrawCall("hblur", true);
            hblur->shader = env->assets->get<Shader>("shaders/blur.glsl").get();
            hblur->shaderState = Ref<ShaderState>::make();
            hblur->shaderState->set("steps", noiseResolution);
            hblur->shaderState->set("spread", glm::vec2(0, 1));
            hblur->shaderState->set("gaussian", true);
            hblur->frameBuffer = swapBuffer;
            hblur->inputs.emplace_back(COLOR, ssao.get());

            auto vblur = ssaoParent->addDrawCall("vblur", true);
            vblur->shader = env->assets->get<Shader>("shaders/blur.glsl").get();
            vblur->shaderState = Ref<ShaderState>::make();
            vblur->shaderState->set("steps", noiseResolution);
            vblur->shaderState->set("spread", glm::vec2(1, 0));
            vblur->shaderState->set("gaussian", true);
            vblur->textures.push_back(swapBuffer->getAttachment(COLOR).get());
            vblur->frameBuffer = ssaoBuffer;

            ssaoParent->addCommand("depth on", DEPTH_ON, true);
        }

        void setupBloom() {
            Ref<RenderPass> pp = env->renderPipeline->getPass("post processing");
            Ref<RenderPass> geometry = env->renderPipeline->getPass("geometry");
            Ref<RenderPass> lighting = env->renderPipeline->getPass("deferred")->getPass("lighting");

            auto bloom = pp->getPass("bloom", true);
            bloom->active = true;
            bloom->addCommand("blend on", BLEND_ON, true);
            bloom->addCommand("depth off", DEPTH_OFF, true);

            auto hbloom = bloom->addDrawCall("hbloom", true);
            hbloom->shader = env->assets->get<Shader>("shaders/blur.glsl").get();
            hbloom->shaderState = Ref<ShaderState>::make();
            hbloom->shaderState->set("steps", 20);
            hbloom->shaderState->set("spread", glm::vec2(0, 1));
            hbloom->shaderState->set("gaussian", true);
            hbloom->frameBuffer = bloomBuffer;
            hbloom->inputs.emplace_back("emissive", lighting.get());


            auto vbloom = bloom->addDrawCall("vbloom", true);
            vbloom->shader = env->assets->get<Shader>("shaders/blur.glsl").get();
            vbloom->shaderState = Ref<ShaderState>::make();
            vbloom->shaderState->set("steps", 20);
            vbloom->shaderState->set("spread", glm::vec2(1, 0));
            vbloom->shaderState->set("gaussian", true);
            vbloom->textures.push_back(bloomBuffer->getAttachment(COLOR).get());
            vbloom->frameBuffer = bloomBuffer2;


            auto cbloom = bloom->addDrawCall("bloom combine", true);
            cbloom->shader = env->assets->get<Shader>("shaders/bloom.glsl").get();
            cbloom->textures.push_back(nullptr); //albedo
            cbloom->textures.push_back(bloomBuffer2->getAttachment(COLOR).get()); //bloom
            cbloom->inputs.emplace_back(COLOR, geometry.get());
            cbloom->shaderState = Ref<ShaderState>::make();
            cbloom->shaderState->set("bloomIntesity", 1.5f);
            cbloom->frameBuffer = bloomBuffer3;


            auto bloomSwap = bloom->addDrawCall("bloom swap", true);
            bloomSwap->shader = env->assets->get<Shader>("shaders/base.glsl").get();
            bloomSwap->output = geometry.get();
            bloomSwap->textures.push_back(bloomBuffer3->getAttachment(COLOR).get());

            bloom->addCommand("depth on", DEPTH_ON, true);

            bloom->addCallback("link passes", [hbloom, lighting, geometry]() {
                if (env->renderSettings->deferredShadingEnabled) {
                    hbloom->inputs[0] = RenderPassDrawCall::Input("emissive", lighting.get());
                }
                else {
                    hbloom->inputs[0] = RenderPassDrawCall::Input((TextureAttachment)(COLOR + 2), geometry.get());
                }
            }, true)->prepareCall = true;
        }

        void setupPostProcess() {
            Ref<RenderPass> pp = env->renderPipeline->getPass("post processing");
            Ref<RenderPass> geometry = env->renderPipeline->getPass("geometry");
            Ref<RenderPass> lighting = env->renderPipeline->getPass("deferred")->getPass("lighting");

            //blur
            auto blur = pp->getPass("blur", true);
            blur->active = false;
            blur->addCommand("clear", CLEAR, true)->frameBuffer = swapBuffer;
            blur->addCommand("resize", RESIZE, true)->frameBuffer = swapBuffer;
            blur->addCommand("blend on", BLEND_ON, true);
            blur->addCommand("depth off", DEPTH_OFF, true);

            auto hblur = blur->addDrawCall("hblur", true);
            hblur->shader = env->assets->get<Shader>("shaders/blur.glsl").get();
            hblur->shaderState = Ref<ShaderState>::make();
            hblur->shaderState->set("steps", 4);
            hblur->shaderState->set("spread", glm::vec2(0, 1));
            hblur->shaderState->set("gaussian", true);
            hblur->frameBuffer = swapBuffer;
            hblur->inputs.emplace_back(COLOR, geometry.get());

            auto vblur = blur->addDrawCall("vblur", true);
            vblur->shader = env->assets->get<Shader>("shaders/blur.glsl").get();
            vblur->shaderState = Ref<ShaderState>::make();
            vblur->shaderState->set("steps", 4);
            vblur->shaderState->set("spread", glm::vec2(1, 0));
            vblur->shaderState->set("gaussian", true);
            vblur->textures.push_back(swapBuffer->getAttachment(COLOR).get());
            vblur->output = geometry.get();

            blur->addCommand("depth on", DEPTH_ON, true);


            //gamma correction
            auto gammaCorrection = pp->getPass("gamma correction");
            gammaCorrection->active = false;
            gammaCorrection->addCommand("clear", CLEAR, true)->frameBuffer = swapBuffer;
            gammaCorrection->addCommand("resize", RESIZE, true)->frameBuffer = swapBuffer;
            gammaCorrection->addCommand("blend on", BLEND_ON, true);
            gammaCorrection->addCommand("depth off", DEPTH_OFF, true);

            auto gamma = gammaCorrection->addDrawCall("gamma", true);
            gamma->shader = env->assets->get<Shader>("shaders/gammaCorrection.glsl").get();
            gamma->shaderState = Ref<ShaderState>::make();
            gamma->shaderState->set("gamma", 2.2f);
            gamma->shaderState->set("exposure", 1.0f);
            gamma->frameBuffer = swapBuffer;
            gamma->inputs.emplace_back(COLOR, geometry.get());


            auto gammaSwap = gammaCorrection->addDrawCall("gamma swap", true);
            gammaSwap->shader = env->assets->get<Shader>("shaders/base.glsl").get();
            gammaSwap->textures.push_back(swapBuffer->getAttachment(COLOR).get());
            gammaSwap->output = geometry.get();

            gammaCorrection->addCommand("blend on", DEPTH_ON, true);
        }
	};

	TRI_REGISTER_SYSTEM(PostProcess);

}