//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/util/Ref.h"
#include "Mesh.h"
#include "Shader.h"
#include "FrameBuffer.h"
#include "Texture.h"
#include "Buffer.h"

namespace tri {

    enum RenderCommand {
        NOOP,
        CLEAR,
        RESIZE,
        DEPTH_ON,
        DEPTH_OFF,
        BLEND_ON,
        BLEND_OFF,
        CULL_ON,
        CULL_OFF,
    };

    class RenderPassStep {
    public:
        enum Type {
            DRAW_CALL,
            DRAW_COMMAND,
            DRAW_CALLBACK,
        };

        std::string name;
        bool active = true;
        bool fixed = true;
        bool newThisFrame = true;

        Type type = DRAW_CALL;
        Ref<Mesh> mesh;
        Ref<Shader> shader;
        Ref<ShaderState> shaderState;
        Ref<FrameBuffer> frameBuffer;
        std::vector<Ref<Texture>> textures;
        std::vector<Ref<Buffer>> buffers;

        std::function<void()> callback;
        RenderCommand command = NOOP;
    };

    class RenderPass {
    public:
        std::string name;
        bool active = true;
        std::vector<RenderPassStep> steps;

        RenderPassStep& addDrawCall(const std::string& name = "", bool fixed = false);
        RenderPassStep& addCommand(RenderCommand command, bool fixed = false);
        RenderPassStep& addCallback(const std::function<void()>& callback, bool fixed = false);
    };

}