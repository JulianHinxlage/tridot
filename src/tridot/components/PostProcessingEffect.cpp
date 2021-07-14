//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "PostProcessingEffect.h"
#include "tridot/engine/Engine.h"
#include "tridot/render/Camera.h"

namespace tridot {

    PostProcessingEffect::PostProcessingEffect() {
        shader = nullptr;
        frameBuffer = nullptr;
    }

    TRI_UPDATE("post processing"){
        auto postProcess = [&](PostProcessingEffect &effect, Ref<FrameBuffer> &output){
            if(output.get() != nullptr && effect.shader.get() != nullptr && effect.shader->getId() != 0){
                TRI_PROFILE("post processing");
                if(effect.frameBuffer.get() == nullptr){
                    effect.frameBuffer = Ref<FrameBuffer>::make();
                    effect.frameBuffer->init(output->getSize().x, output->getSize().y,{{COLOR}});
                }
                if(effect.frameBuffer->getSize() != output->getSize()){
                    effect.frameBuffer->resize(output->getSize().x, output->getSize().y);
                }
                effect.frameBuffer->clear();
                engine.renderer.begin(glm::mat4(1), {0, 0, 0}, effect.frameBuffer);
                engine.renderer.submit({{0, 0, 0}, {2, 2, 2}},
                                       output->getAttachment(COLOR).get(), nullptr, effect.shader.get());
                engine.renderer.end();
                output = effect.frameBuffer;
            }
        };
        engine.view<PostProcessingEffect>().each([&](EntityId id, PostProcessingEffect &effect){
            if(auto *camera = engine.getComponentByHierarchy<PerspectiveCamera>(id)){
                postProcess(effect, camera->output);
            }else if(auto *camera = engine.getComponentByHierarchy<OrthographicCamera>(id)){
                postProcess(effect, camera->output);
            }else{
                bool processed = false;
                engine.view<PerspectiveCamera>().each([&](PerspectiveCamera &camera){
                    if(!processed) {
                        postProcess(effect, camera.output);
                        processed = true;
                    }
                });
                engine.view<OrthographicCamera>().each([&](OrthographicCamera &camera){
                    if(!processed){
                        postProcess(effect, camera.output);
                        processed = true;
                    }
                });
            }
        });
    }
}
