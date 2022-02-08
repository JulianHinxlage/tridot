//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "RenderPipeline.h"
#include "engine/AssetManager.h"
#include "RenderContext.h"
#include "ShaderState.h"
#include "RenderThread.h"
#include "Renderer.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "tracy/TracyOpenGL.hpp"

namespace tri {
    
    TRI_REGISTER_SYSTEM_INSTANCE(RenderPipeline, env->pipeline);

    void RenderPipeline::setupPipeline() {
        outputFrameBuffer = Ref <FrameBuffer>::make();
        mainFrameBuffer = Ref <FrameBuffer>::make();
        outputFrameBuffer->setAttachment({ COLOR });
        mainFrameBuffer->setAttachment({ COLOR });

        getOrAddRenderPass("gui end");
        getOrAddRenderPass("window");
        getOrAddRenderPass("gui begin");
        getOrAddRenderPass("clear");
        getOrAddRenderPass("editor");
        getOrAddRenderPass("skybox");
        getOrAddRenderPass("shadow");
        getOrAddRenderPass("geometry");
        getOrAddRenderPass("viewport");
        getOrAddRenderPass("outlines");
        auto postProcess = getOrAddRenderPass("post process");




        postProcess->active = false;
        auto& resize = postProcess->addCommand(RESIZE, true);
        resize.frameBuffer = mainFrameBuffer;
        resize.name = "resize";
        
        {
            auto &step = postProcess->addDrawCall("hblur", true);
            step.frameBuffer = mainFrameBuffer;
            step.textures.push_back(outputFrameBuffer->getAttachment(COLOR));
            step.shader = env->assets->get<Shader>("shaders/blur.glsl");
            step.shaderState = Ref<ShaderState>::make();
            step.shaderState->set("steps", 10);
            step.shaderState->set("spread", glm::vec2(1, 0.0f));
        }
        {
            auto& step = postProcess->addDrawCall("vblur", true);
            step.frameBuffer = outputFrameBuffer;
            step.textures.push_back(mainFrameBuffer->getAttachment(COLOR));
            step.shader = env->assets->get<Shader>("shaders/blur.glsl");
            step.shaderState = Ref<ShaderState>::make();
            step.shaderState->set("steps", 10);
            step.shaderState->set("spread", glm::vec2(0.0f, 1));
        }

    }

    RenderPipeline::RenderPipeline() {
        width = 0;
        height = 0;
    }

    void RenderPipeline::startup() {
        env->signals->update.callbackOrder({"Camera", "Skybox", "MeshComponent", "Renderer", "RenderPipeline"});

        //create quad mesh for full screen passes
        quad = quad.make();
        float quadVertices[] = {
            -1.0, -1.0, +0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
            +1.0, -1.0, +0.0, 0.0, 0.0, 1.0, 1.0, 0.0,
            +1.0, +1.0, +0.0, 0.0, 0.0, 1.0, 1.0, 1.0,
            -1.0, +1.0, +0.0, 0.0, 0.0, 1.0, 0.0, 1.0,
        };
        int quadIndices[] = {
            0, 1, 2,
            0, 2, 3,
        };
        quad->create(quadVertices, sizeof(quadVertices) / sizeof(quadVertices[0]), 
            quadIndices, sizeof(quadIndices) / sizeof(quadIndices[0]), 
            { {FLOAT, 3}, {FLOAT, 3} ,{FLOAT, 2} });
           
        setupPipeline();
    }

    Ref<RenderPass> RenderPipeline::addRenderPass(const std::string& name) {
        Ref<RenderPass> pass = Ref<RenderPass>::make();
        pass->name = name;
        renderPasses.push_back(pass);
        return pass;
    }

    Ref<RenderPass> RenderPipeline::getRenderPass(const std::string& name) {
        for (auto& pass : renderPasses) {
            if (pass && pass->name == name) {
                return pass;
            }
        }
        return nullptr;
    }

    Ref<RenderPass> RenderPipeline::getOrAddRenderPass(const std::string& name) {
        for (auto& pass : renderPasses) {
            if (pass && pass->name == name) {
                return pass;
            }
        }
        return addRenderPass(name);
    }

    void RenderPipeline::removeRenderPass(const std::string& name) {
        for (int i = 0; i < renderPasses.size(); i++) {
            auto& pass = renderPasses[i];
            if (pass && pass->name == name) {
                renderPasses.erase(renderPasses.begin() + i);
                i--;
            }
        }
    }

    void RenderPipeline::activateRenderPass(const std::string& name, bool active){
        for (auto& pass : renderPasses) {
            if (pass && pass->name == name) {
                pass->active = active;
            }
        }
    }

    void RenderPipeline::setInput(Ref<FrameBuffer> frameBuffer) {
        if (inputFrameBuffer != frameBuffer) {
            replaceFrameBuffer(inputFrameBuffer, frameBuffer);
            inputFrameBuffer = frameBuffer;
        }
    }

    void RenderPipeline::setOutput(Ref<FrameBuffer> frameBuffer) {
        if (outputFrameBuffer != frameBuffer) {
            replaceFrameBuffer(outputFrameBuffer, frameBuffer);
            outputFrameBuffer = frameBuffer;
        }
    }

    void RenderPipeline::setSize(uint32_t width, uint32_t height) {
        this->width = width;
        this->height = height;
    }

    Ref<Mesh> RenderPipeline::getQuad() {
        return quad;
    }

    void RenderPipeline::replaceFrameBuffer(Ref<FrameBuffer> target, Ref<FrameBuffer> replacement) {
        for (auto& pass : renderPasses) {
            if (pass) {
                for (auto& step : pass->steps) {
                    if (step.frameBuffer == target) {
                        step.frameBuffer = replacement;
                    }
                    if (target) {
                        for (auto& tex : step.textures) {
                            if (tex) {
                                for (int attachment = DEPTH; attachment < COLOR + 16; attachment++) {
                                    if (tex == target->getAttachment((TextureAttachment)attachment)) {
                                        if (replacement) {
                                            tex = replacement->getAttachment((TextureAttachment)attachment);
                                        }
                                        else {
                                            tex = nullptr;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void RenderPipeline::submitRenderPasses() {
        TRI_PROFILE("submitRenderPasses");

        env->renderThread->lock();

        //move active flag to next frame steps
        for (auto& currentPass : currentRenderPasses) {
            auto pass = getOrAddRenderPass(currentPass->name);
            pass->active = currentPass->active;
            for (int i = 0; i < currentPass->steps.size(); i++) {
                auto& currentStep = currentPass->steps[i];
                if (currentStep.name == "") {
                    if (pass->steps.size() > i) {
                        if (pass->steps[i].name == "") {
                            pass->steps[i].active = currentStep.active;
                        }
                    }
                }
                else {
                    for (auto& step : pass->steps) {
                        if (currentStep.name == step.name) {
                            step.active = currentStep.active;
                        }
                    }
                }
            }
        }

        //copy passes to current passes and clear passes
        int stepCount = 0;
        int drawCallCount = 0;
        currentRenderPasses.clear();
        for (auto& pass : renderPasses) {
            currentRenderPasses.push_back(Ref<RenderPass>::make(*pass));
            for (int i = pass->steps.size() - 1; i >= 0; i--) {
                auto& step = pass->steps[i];
                stepCount++;
                if (step.type == RenderPassStep::DRAW_CALL) {
                    drawCallCount++;
                }
                if (!step.fixed) {
                    pass->steps.erase(pass->steps.begin() + i);
                }
            }
        }
        env->renderer->stats.drawCallCount = drawCallCount;
        env->renderThread->unlock();
    }

    void RenderPipeline::execute() {
        TRI_PROFILE("RenderPipeline");
        TracyGpuZone("RenderPipeline");
        for (auto& pass : currentRenderPasses) {
            if (pass && pass->active) {

                TRI_PROFILE_NAME(pass->name.c_str(), pass->name.size());

                for (auto& step : pass->steps) {
                    if (step.active && (step.fixed || step.newThisFrame)) {
                        TRI_PROFILE_NAME(step.name.c_str(), step.name.size());

                        if (step.type == RenderPassStep::DRAW_CALL) {
                            TracyGpuZone("DrawCall");
                            executeDrawCall(step);
                        }
                        else if (step.type == RenderPassStep::DRAW_COMMAND) {
                            TracyGpuZone("RenderCommand");
                            executeRenderCommand(step);
                        }
                        else if (step.type == RenderPassStep::DRAW_CALLBACK) {
                            if (step.callback) {
                                TracyGpuZone("Callback");
                                step.callback();
                            }
                        }
                    }
                }
            }
        }
        RenderContext::flush();
    }

    void RenderPipeline::executeDrawCall(RenderPassStep& call) {
        if (call.frameBuffer && call.frameBuffer->getId() != 0) {
            call.frameBuffer->bind();
        }
        else {
            FrameBuffer::unbind();
        }
        if (call.shader && call.shader->getId() != 0) {
            call.shader->bind();
            if (call.shaderState) {
                call.shaderState->apply(call.shader.get());
            }

            if (call.textures.size() > 0) {
                int slots[30];
                for (int i = 0; i < 30; i++) {
                    slots[i] = i;
                    if (call.textures.size() > i) {
                        if (call.textures[i]) {
                            call.textures[i]->bind(i);
                        }
                    }
                }
                call.shader->set("uTextures", slots, 30);
            }

            VertexArray* va = nullptr;
            if (call.mesh) {
                va = &call.mesh->vertexArray;
            }
            if (call.vertexArray) {
                va = call.vertexArray;
            }
            if (va == nullptr) {
                va = &quad->vertexArray;
            }
            if (va->getId() != 0) {
                TracyGpuZone("Submit");
                va->submit(-1, call.insatnceCount);
            }

            for (auto& tex : call.textures) {
                if (tex) {
                    tex->unbind();
                }
            }
        }
    }

    void RenderPipeline::executeRenderCommand(RenderPassStep& call) {
        switch (call.command) {
        case RenderCommand::NOOP:
            break;
        case RenderCommand::CLEAR:
            if (call.frameBuffer) {
                call.frameBuffer->clear();
            }
            break;
        case RenderCommand::RESIZE:
            if (call.frameBuffer) {
                call.frameBuffer->resize(width, height);
            }
            break;
        case RenderCommand::DEPTH_ON:
            RenderContext::setDepth(true);
            break;
        case RenderCommand::DEPTH_OFF:
            RenderContext::setDepth(false);
            break;
        case RenderCommand::BLEND_ON:
            RenderContext::setBlend(true);
            break;
        case RenderCommand::BLEND_OFF:
            RenderContext::setBlend(false);
            break;
        case RenderCommand::CULL_ON:
            RenderContext::setCull(true);
            break;
        case RenderCommand::CULL_OFF:
            RenderContext::setCull(false);
            break;
        default:
            break;
        }
    }


}
