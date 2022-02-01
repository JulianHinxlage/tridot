//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//


#include "RenderPass.h"
#include "core/core.h"

namespace tri {

    TRI_REGISTER_TYPE(RenderPassStep::Type);
    TRI_REGISTER_CONSTANT(RenderPassStep::Type, DRAW_CALL);
    TRI_REGISTER_CONSTANT(RenderPassStep::Type, DRAW_COMMAND);
    TRI_REGISTER_CONSTANT(RenderPassStep::Type, DRAW_CALLBACK);

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

    TRI_REGISTER_TYPE(RenderPassStep);
    TRI_REGISTER_MEMBER(RenderPassStep, type);
    TRI_REGISTER_MEMBER(RenderPassStep, mesh);
    TRI_REGISTER_MEMBER(RenderPassStep, shader);
    TRI_REGISTER_MEMBER(RenderPassStep, command);
    TRI_REGISTER_MEMBER(RenderPassStep, shaderState);
    TRI_REGISTER_MEMBER(RenderPassStep, frameBuffer);
    TRI_REGISTER_MEMBER(RenderPassStep, textures);
    TRI_REGISTER_MEMBER(RenderPassStep, buffers);

    RenderPassStep& RenderPass::addDrawCall(const std::string& name, bool fixed) {
        steps.emplace_back();
        steps.back().type = RenderPassStep::DRAW_CALL;
        steps.back().name = name;
        steps.back().fixed = fixed;
        return steps.back();
    }

    RenderPassStep& RenderPass::addCommand(RenderCommand command, bool fixed) {
        steps.emplace_back();
        steps.back().type = RenderPassStep::DRAW_COMMAND;
        steps.back().command = command;
        steps.back().fixed = fixed;
        return steps.back();
    }

    RenderPassStep& RenderPass::addCallback(const std::function<void()>& callback, bool fixed) {
        steps.emplace_back();
        steps.back().type = RenderPassStep::DRAW_CALLBACK;
        steps.back().callback = callback;
        steps.back().fixed = fixed;
        return steps.back();
    }

}