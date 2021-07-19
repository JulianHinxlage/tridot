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

    TRI_UPDATE_CALLBACK("post processing"){
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
                env->renderer->begin(glm::mat4(1), {0, 0, 0}, effect.frameBuffer);
                env->renderer->submit({{0, 0, 0}, {2, 2, 2}},
                                       output->getAttachment(COLOR).get(), nullptr, effect.shader.get());
                env->renderer->end();
                output = effect.frameBuffer;
            }
        };
        env->scene->view<PostProcessingEffect>().each([&](EntityId id, PostProcessingEffect &effect){
            if(auto *camera = env->scene->getComponentByHierarchy<PerspectiveCamera>(id)){
                postProcess(effect, camera->output);
            }else if(auto *camera = env->scene->getComponentByHierarchy<OrthographicCamera>(id)){
                postProcess(effect, camera->output);
            }else{
                bool processed = false;
                env->scene->view<PerspectiveCamera>().each([&](PerspectiveCamera &camera){
                    if(!processed) {
                        postProcess(effect, camera.output);
                        processed = true;
                    }
                });
                env->scene->view<OrthographicCamera>().each([&](OrthographicCamera &camera){
                    if(!processed){
                        postProcess(effect, camera.output);
                        processed = true;
                    }
                });
            }
        });
    }
}
