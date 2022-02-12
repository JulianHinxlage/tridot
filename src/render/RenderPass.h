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

    class RenderPassDrawCall;
    class RenderPassDrawCommand;
    class RenderPassDrawCallback;

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

    class RenderPass {
    public:
        enum Type {
            NODE,
            DRAW_CALL,
            DRAW_COMMAND,
            DRAW_CALLBACK,
        };

        Type type = NODE;
        std::string name;
        bool active;
        bool newThisFrame;
        bool fixed;
        std::vector<Ref<RenderPass>> subPasses;

        RenderPass();
        RenderPass(const RenderPass& renderPass);

        Ref<RenderPassDrawCall> addDrawCall(const std::string& name, bool fixed = false);
        Ref<RenderPassDrawCommand> addCommand(const std::string& name, RenderCommand command, bool fixed = false);
        Ref<RenderPassDrawCallback> addCallback(const std::string &name, const std::function<void()> &callback, bool fixed = false);
        Ref<RenderPass> getPass(const std::string& name, bool fixed = false, bool resetPosition = false);
        void removetPass(const std::string& name);
        virtual void execute();
    };

    class RenderPassDrawCall : public RenderPass {
    public:
        Mesh *mesh;
        VertexArray* vertexArray;
        Shader *shader;
        Ref<ShaderState> shaderState;
        FrameBuffer *frameBuffer;
        int instanceCount = -1;
        std::vector<Texture*> textures;
        std::vector<Buffer*> buffers;

        RenderPassDrawCall();
        RenderPassDrawCall(const RenderPassDrawCall &call);
    
        virtual void execute() override;
    };

    class RenderPassDrawCommand : public RenderPass {
    public:
        RenderCommand command = NOOP;
        FrameBuffer* frameBuffer;
    
        virtual void execute() override;
    };

    class RenderPassDrawCallback : public RenderPass {
    public:
        std::function<void()> callback;
    
        virtual void execute() override;
    };

}