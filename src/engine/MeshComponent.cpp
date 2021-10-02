//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "MeshComponent.h"
#include "entity/Scene.h"
#include "render/Renderer.h"
#include "render/Window.h"
#include "Transform.h"
#include "Camera.h"
#include "core/core.h"

namespace tri {

    TRI_REGISTER_COMPONENT(MeshComponent);
    TRI_REGISTER_MEMBER(MeshComponent, mesh);
    TRI_REGISTER_MEMBER(MeshComponent, material);
    TRI_REGISTER_MEMBER(MeshComponent, color);

    TRI_REGISTER_TYPE(Color);

    TRI_UPDATE_CALLBACK("MeshComponent") {


        env->scene->view<Camera, Transform>().each([](Camera& camera, Transform &cameraTransform) {
            if (camera.active) {
                env->renderer->cameraCount++;

                //frame buffer
                if (camera.output) {
                    camera.output->clear();
                }
                if(!env->editor){
                    if (camera.isPrimary) {
                        camera.aspectRatio = env->window->getAspectRatio();
                        if(camera.output) {
                            if (camera.output->getSize() != env->window->getSize()) {
                                camera.output->resize(env->window->getSize().x, env->window->getSize().y);
                            }
                        }
                    }
                }

                //lights
                env->scene->view<Light, Transform>().each([](Light &light, Transform &transform){
                    if(light.type == DIRECTIONAL_LIGHT){
                        Transform t;
                        t.decompose(transform.getMatrix());
                        t.position = {0, 0, 0};
                        t.scale = {1, 1, 1};
                        env->renderer->submit(t.calculateLocalMatrix() * glm::vec4(0, 0, -1, 0), light);
                    }else{
                        Transform t;
                        t.decompose(transform.getMatrix());
                        env->renderer->submit(t.position, light);
                    }
                });

                env->renderer->beginScene(camera.projection, cameraTransform.position);
                {
                    TRI_PROFILE("render/submit");
                    //meshes
                    env->scene->view<MeshComponent, Transform>().each([](EntityId id, MeshComponent& mesh, Transform& transform) {
                        env->renderer->submit(transform.getMatrix(), transform.position, mesh.mesh.get(), mesh.material.get(), mesh.color, id);
                    });
                }
                env->renderer->drawScene(camera.output, camera.pipeline);
                env->renderer->resetScene();
            }
            if(env->editor || !camera.isPrimary){
                camera.active = false;
            }
        });
    }

}
