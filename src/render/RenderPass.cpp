//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//


#include "RenderPass.h"
#include "core/core.h"
#include "RenderThread.h"
#include "RenderContext.h"
#include "ShaderState.h"
#include "RenderPipeline.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "tracy/TracyOpenGL.hpp"

namespace tri {

    TRI_REGISTER_TYPE(RenderPass::Type);
    TRI_REGISTER_CONSTANT(RenderPass::Type, NODE);
    TRI_REGISTER_CONSTANT(RenderPass::Type, DRAW_CALL);
    TRI_REGISTER_CONSTANT(RenderPass::Type, DRAW_COMMAND);
    TRI_REGISTER_CONSTANT(RenderPass::Type, DRAW_CALLBACK);

    TRI_REGISTER_TYPE(RenderCommand);
    TRI_REGISTER_CONSTANT(RenderCommand, NOOP);
    TRI_REGISTER_CONSTANT(RenderCommand, CLEAR);
    TRI_REGISTER_CONSTANT(RenderCommand, RESIZE);
    TRI_REGISTER_CONSTANT(RenderCommand, DEPTH_ON);
    TRI_REGISTER_CONSTANT(RenderCommand, DEPTH_OFF);
    TRI_REGISTER_CONSTANT(RenderCommand, BLEND_ON);
    TRI_REGISTER_CONSTANT(RenderCommand, BLEND_OFF);
    TRI_REGISTER_CONSTANT(RenderCommand, CULL_ON);
    TRI_REGISTER_CONSTANT(RenderCommand, CULL_OFF);

    TRI_REGISTER_TYPE(RenderPass);
    TRI_REGISTER_MEMBER(RenderPass, type);
    TRI_REGISTER_MEMBER(RenderPass, subPasses);

    TRI_REGISTER_TYPE(RenderPassDrawCall);
    TRI_REGISTER_MEMBER(RenderPassDrawCall, mesh);
    TRI_REGISTER_MEMBER(RenderPassDrawCall, vertexArray);
    TRI_REGISTER_MEMBER(RenderPassDrawCall, shader);
    TRI_REGISTER_MEMBER(RenderPassDrawCall, shaderState);
    TRI_REGISTER_MEMBER(RenderPassDrawCall, frameBuffer);
    TRI_REGISTER_MEMBER(RenderPassDrawCall, instanceCount);
    TRI_REGISTER_MEMBER(RenderPassDrawCall, textures);
    TRI_REGISTER_MEMBER(RenderPassDrawCall, buffers);

    TRI_REGISTER_TYPE(RenderPassDrawCommand);
    TRI_REGISTER_MEMBER(RenderPassDrawCommand, command);
    TRI_REGISTER_MEMBER(RenderPassDrawCommand, frameBuffer);

    TRI_REGISTER_TYPE(RenderPassDrawCallback);

    RenderPass::RenderPass() {
        type = NODE;
        name = "";
        active = true;
        newThisFrame = true;
        fixed = false;
    }

    RenderPass::RenderPass(const RenderPass& renderPass) {
        type = renderPass.type;
        name = renderPass.name;
        active = renderPass.active;
        newThisFrame = renderPass.newThisFrame;
        fixed = renderPass.fixed;

        subPasses.resize(renderPass.subPasses.size());
        for (int i = 0; i < subPasses.size(); i++) {
            auto pass = renderPass.subPasses[i];
            if (pass->type == NODE) {
                subPasses[i] = Ref<RenderPass>::make(*pass);
            }
            else if (pass->type == DRAW_CALL) {
                subPasses[i] = (Ref<RenderPass>)Ref<RenderPassDrawCall>::make(*(RenderPassDrawCall*)pass.get());
            }
            else if (pass->type == DRAW_COMMAND) {
                subPasses[i] = (Ref<RenderPass>)Ref<RenderPassDrawCommand>::make(*(RenderPassDrawCommand*)pass.get());
            }
            else if (pass->type == DRAW_CALLBACK) {
                subPasses[i] = (Ref<RenderPass>)Ref<RenderPassDrawCallback>::make(*(RenderPassDrawCallback*)pass.get());
            }
        }
    }

    Ref<RenderPassDrawCall> RenderPass::addDrawCall(const std::string& name, bool fixed) {
        Ref<RenderPassDrawCall> pass = Ref<RenderPassDrawCall>::make();
        pass->name = name;
        pass->type = DRAW_CALL;
        pass->newThisFrame = true;
        pass->active = true;
        pass->fixed = fixed;
        subPasses.push_back((Ref<RenderPass>)pass);
        return pass;
    }

    Ref<RenderPassDrawCommand> RenderPass::addCommand(const std::string& name, RenderCommand command, bool fixed) {
        Ref<RenderPassDrawCommand> pass = Ref<RenderPassDrawCommand>::make();
        pass->name = name;
        pass->type = DRAW_COMMAND;
        pass->newThisFrame = true;
        pass->active = true;
        pass->fixed = fixed;
        pass->command = command;
        subPasses.push_back((Ref<RenderPass>)pass);
        return pass;
    }

    Ref<RenderPassDrawCallback> RenderPass::addCallback(const std::string& name, const std::function<void()>& callback, bool fixed) {
        Ref<RenderPassDrawCallback> pass = Ref<RenderPassDrawCallback>::make();
        pass->name = name;
        pass->type = DRAW_CALLBACK;
        pass->newThisFrame = true;
        pass->active = true;
        pass->fixed = fixed;
        pass->callback = callback;
        subPasses.push_back((Ref<RenderPass>)pass);
        return pass;
    }

    Ref<RenderPass> RenderPass::getPass(const std::string& name, bool fixed, bool resetPosition) {
        for (int i = 0; i < subPasses.size(); i++) {
            auto& pass = subPasses[i];
            if (pass && pass->name == name) {
                if (resetPosition) {
                    Ref<RenderPass> p = pass;
                    subPasses.erase(subPasses.begin() + i);
                    subPasses.push_back(p);
                    return p;
                }
                else {
                    return pass;
                }
            }
        }
        Ref<RenderPass> pass = Ref<RenderPass>::make();
        pass->name = name;
        pass->type = NODE;
        pass->newThisFrame = true;
        pass->active = true;
        pass->fixed = fixed;
        subPasses.push_back((Ref<RenderPass>)pass);
        return pass;
    }

    void RenderPass::removetPass(const std::string& name) {
        for (int i = 0; i < subPasses.size(); i++) {
            auto& pass = subPasses[i];
            if (pass && pass->name == name) {
                subPasses.erase(subPasses.begin() + i);
                return;
            }
        }
    }

    void RenderPass::execute() {
        if (active) {
            for (auto& pass : subPasses) {
                if (pass) {
                    TRI_PROFILE_NAME(pass->name.c_str(), pass->name.size());
                    pass->execute();
                }
            }
        }
    }

    Ref<FrameBuffer> RenderPass::getOutputFrameBuffer() {
        if (subPasses.size() > 0) {
            for (int i = subPasses.size() - 1; i >= 0; i--) {
                auto& pass = subPasses[i];
                auto fb = pass->getOutputFrameBuffer();
                if (fb) {
                    return fb;
                }
            }
        }
        return nullptr;
    }


    RenderPassDrawCall::RenderPassDrawCall() {
        mesh = nullptr;
        vertexArray = nullptr;
        shader = nullptr;
        shaderState = nullptr;
        frameBuffer = nullptr;
        instanceCount = -1;
    }

    RenderPassDrawCall::RenderPassDrawCall(const RenderPassDrawCall& call)
        : RenderPass(call) {
        mesh = call.mesh;
        vertexArray = call.vertexArray;
        shader = call.shader;
        if (call.shaderState) {
            shaderState = Ref<ShaderState>::make(*call.shaderState);
        }
        else {
            shaderState = nullptr;
        }
        frameBuffer = call.frameBuffer;
        instanceCount = call.instanceCount;
        textures = call.textures;
        buffers = call.buffers;
    }

    void RenderPassDrawCall::execute() {
        if (active) {
            TracyGpuZone("drawCall");

            if (frameBuffer && frameBuffer->getId() != 0) {
                frameBuffer->bind();
            }
            else {
                FrameBuffer::unbind();
            }
            if (shader && shader->getId() != 0) {
                shader->bind();
                if (shaderState) {
                    shaderState->apply(shader);
                }

                if (textures.size() > 0) {
                    int slots[30];
                    for (int i = 0; i < 30; i++) {
                        slots[i] = i;
                        if (textures.size() > i) {
                            if (textures[i]) {
                                textures[i]->bind(i);
                            }
                        }
                    }
                    shader->set("uTextures", slots, 30);
                }

                VertexArray* va = nullptr;
                if (mesh) {
                    va = &mesh->vertexArray;
                }
                if (vertexArray) {
                    va = vertexArray;
                }
                if (va == nullptr) {
                    va = &env->renderPipeline->getQuad()->vertexArray;
                }
                if (va->getId() != 0) {
                    TracyGpuZone("Submit");
                    va->submit(-1, instanceCount);
                }

                for (auto& tex : textures) {
                    if (tex) {
                        tex->unbind();
                    }
                }
            }
            RenderPass::execute();
        }
    }

    Ref<FrameBuffer> RenderPassDrawCall::getOutputFrameBuffer() {
        if (subPasses.size() > 0) {
            for (int i = subPasses.size() - 1; i >= 0; i--) {
                auto& pass = subPasses[i];
                auto fb = pass->getOutputFrameBuffer();
                if (fb) {
                    return fb;
                }
            }
        }
        return frameBuffer;
    }

    void RenderPassDrawCommand::execute() {
        if (active) {
            TracyGpuZone("drawCommand");

            switch (command) {
            case RenderCommand::NOOP:
                break;
            case RenderCommand::CLEAR:
                if (frameBuffer) {
                    frameBuffer->clear();
                }
                break;
            case RenderCommand::RESIZE:
                if (frameBuffer) {
                    frameBuffer->resize(env->renderPipeline->getWidth(), env->renderPipeline->getHeight());
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

            RenderPass::execute();
        }
    }

    void RenderPassDrawCallback::execute() {
        if (active) {
            TracyGpuZone("drawCallback");

            if (callback) {
                callback();
            }
            RenderPass::execute();
        }
    }

}