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

        getPass("clear");
        getPass("skybox");
        getPass("shadow");
        getPass("geometry");
        getPass("deferred");
        getPass("transparency");
        getPass("viewport");
        getPass("outlines");
        getPass("post processing");
        getPass("draw to screen");
    }

    Ref<RenderPass> RenderPipeline::getPass(const std::string& name, bool fixed) {
        return rootPass->getPass(name, fixed);
    }

    void RenderPipeline::removePass(const std::string& name) {
        rootPass->removetPass(name);
    }

    void RenderPipeline::prepareRenderPasses() {
        TRI_PROFILE("prepareRenderPasses");
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


        rootPass->prepare();

        //copy passes to current passes and clear passes
        int drawCallCount = 0;
        int instanceCount = 0;
        executeRootPass = Ref<RenderPass>::make(*rootPass);
        std::function<void(Ref<RenderPass>, bool)> clear = [&](Ref<RenderPass> pass, bool active) {
            for (int i = pass->subPasses.size() - 1; i >= 0; i--) {
                auto& subPass = pass->subPasses[i];
                if (subPass->type == RenderPass::DRAW_CALL) {
                    if (subPass->active && active) {
                        drawCallCount++;
                        int count = ((RenderPassDrawCall*)subPass.get())->instanceCount;
                        if (count != -1) {
                            instanceCount += count;
                        }
                    }
                }
                clear(subPass, active && subPass->active);
                if (!subPass->fixed) {
                    if (subPass->type != RenderPass::NODE) {
                        pass->subPasses.erase(pass->subPasses.begin() + i);
                    }
                }
            }
        };
        clear(rootPass, true);

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
