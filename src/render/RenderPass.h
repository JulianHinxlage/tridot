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
#include "BatchBuffer.h"

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
        virtual Ref<FrameBuffer> getOutputFrameBuffer();
        virtual void prepare();
    };

    class RenderPassDrawCall : public RenderPass {
    public:
        Mesh *mesh;
        VertexArray* vertexArray;
        Shader *shader;
        Ref<ShaderState> shaderState;
        Ref<FrameBuffer> frameBuffer;
        int instanceCount;
        std::vector<Texture*> textures;
        std::vector<BatchBuffer*> buffers;

        class Input {
        public:
            TextureAttachment attachment;
            std::string attachmentName;
            RenderPass *source;

            Input(TextureAttachment attachment = COLOR, RenderPass* source = nullptr)
                : source(source), attachment(attachment), attachmentName("") {}

            Input(std::string attachmentName, RenderPass* source = nullptr)
                : source(source), attachment(COLOR), attachmentName(attachmentName) {}
        };
        std::vector<Input> inputs;
        RenderPass* output;
        RenderPass* shaderStateInput;

        RenderPassDrawCall();
        RenderPassDrawCall(const RenderPassDrawCall &call);
    
        virtual void execute() override;
        virtual Ref<FrameBuffer> getOutputFrameBuffer() override;
        virtual void prepare() override;
    };

    class RenderPassDrawCommand : public RenderPass {
    public:
        RenderCommand command = NOOP;
        Ref<FrameBuffer> frameBuffer;
    
        virtual void execute() override;
    };

    class RenderPassDrawCallback : public RenderPass {
    public:
        std::function<void()> callback;
        bool prepareCall = false;
    
        virtual void execute() override;
        virtual void prepare() override;
    };

}