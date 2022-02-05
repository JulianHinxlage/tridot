//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"
#include "RenderPass.h"

namespace tri {

    class RenderPipeline : public System {
    public:
        RenderPipeline();

        void startup() override;

        Ref<RenderPass> addRenderPass(const std::string &name);
        Ref<RenderPass> getRenderPass(const std::string& name);
        Ref<RenderPass> getOrAddRenderPass(const std::string &name);
        void removeRenderPass(const std::string& name);
        void activateRenderPass(const std::string& name, bool active = true);
        const std::vector<Ref<RenderPass>>& getRenderPasses() { return currentRenderPasses; }

        void setInput(Ref<FrameBuffer> frameBuffer);
        void setOutput(Ref<FrameBuffer> frameBuffer);
        void setSize(uint32_t width, uint32_t height);
        Ref<Mesh> getQuad();

        void submitRenderPasses();
        void execute();

    private:
        Ref<FrameBuffer> mainFrameBuffer;
        Ref<FrameBuffer> inputFrameBuffer;
        Ref<FrameBuffer> outputFrameBuffer;

        Ref<Mesh> quad;
        std::vector<Ref<RenderPass>> renderPasses;
        std::vector<Ref<RenderPass>> currentRenderPasses;

        uint32_t width;
        uint32_t height;

        void setupPipeline();
        void replaceFrameBuffer(Ref<FrameBuffer> target, Ref<FrameBuffer> replacement);
        void executeDrawCall(RenderPassStep& step);
        void executeRenderCommand(RenderPassStep& step);
    };

}

