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

        Ref<RenderPass> getPass(const std::string& name, bool fixed = true);
        void removePass(const std::string& name);

        void setSize(uint32_t width, uint32_t height);
        uint32_t getHeight() { return height; }
        uint32_t getWidth() { return width; }
        Ref<Mesh> getQuad() { return quad; }

        void submitRenderPasses();
        void execute();
        Ref<RenderPass> getRootPass() { return executeRootPass; }

    private:
        Ref<RenderPass> rootPass;
        Ref<RenderPass> executeRootPass;

        Ref<Mesh> quad;
        uint32_t width;
        uint32_t height;
    };

}

