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
    
    TRI_REGISTER_SYSTEM_INSTANCE(RenderPipeline, env->renderPipeline);

    RenderPipeline::RenderPipeline() {
        rootPass = nullptr;
    }
    
    void RenderPipeline::startup() {
        rootPass = Ref<RenderPass>::make();
        rootPass->name = "pipeline";
        rootPass->type = RenderPass::NODE;
        rootPass->newThisFrame = true;
        rootPass->active = true;
        rootPass->fixed = true;

        executeRootPass = Ref<RenderPass>::make();
        executeRootPass->name = "pipeline";
        executeRootPass->type = RenderPass::NODE;
        executeRootPass->newThisFrame = true;
        executeRootPass->active = true;
        executeRootPass->fixed = true;

        env->signals->update.callbackOrder({ "Camera", "Skybox", "MeshComponent", "Renderer", "RenderPipeline" });

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


        getPass("gui end");
        getPass("window");
        getPass("gui begin");
        getPass("clear");
        getPass("editor");
        getPass("skybox");
        getPass("shadow");
        getPass("geometry");
        getPass("viewport");
        getPass("outlines");
    }

    Ref<RenderPass> RenderPipeline::getPass(const std::string& name, bool fixed) {
        return rootPass->getPass(name, fixed);
    }

    void RenderPipeline::removePass(const std::string& name) {
        rootPass->removetPass(name);
    }

    void RenderPipeline::submitRenderPasses() {
        TRI_PROFILE("submitRenderPasses");
        env->renderThread->lock();

        //move active flag to new passes
        std::function<void(Ref<RenderPass>, Ref<RenderPass>)> setActive = [&](Ref<RenderPass> pass, Ref<RenderPass> executePass) {
            for (auto& p1 : executePass->subPasses) {
                for (auto& p2 : pass->subPasses) {
                    if (p1->name == p2->name) {
                        p2->active = p1->active;
                        setActive(p2, p1);
                    }
                }
            }
        };
        setActive(rootPass, executeRootPass);


        //copy passes to current passes and clear passes
        int drawCallCount = 0;
        int instanceCount = 0;
        executeRootPass->subPasses.clear();
        std::function<void(Ref<RenderPass>)> clear = [&](Ref<RenderPass> pass) {
            for (int i = pass->subPasses.size() - 1; i >= 0; i--) {
                auto& subPass = pass->subPasses[i];
                if (subPass->type == RenderPass::DRAW_CALL) {
                    drawCallCount++;
                    int count = ((RenderPassDrawCall*)subPass.get())->instanceCount;
                    if (count != -1) {
                        instanceCount += count;
                    }
                }
                if (!subPass->fixed) {
                    pass->subPasses.erase(pass->subPasses.begin() + i);
                }
                clear(subPass);
            }
        };
        for (auto& pass : rootPass->subPasses) {
            executeRootPass->subPasses.push_back(Ref<RenderPass>::make(*pass));
            clear(pass);
        }

        env->renderer->stats.drawCallCount = drawCallCount;
        env->renderer->stats.instanceCount = instanceCount;
        env->renderThread->unlock();
    }

    void RenderPipeline::execute() {
        TRI_PROFILE("RenderPipeline");
        TracyGpuZone("RenderPipeline");

        if (executeRootPass) {
            executeRootPass->execute();
        }
    }

    void RenderPipeline::setSize(uint32_t width, uint32_t height) { 
        this->width = width;
        this->height = height;
    }

}
