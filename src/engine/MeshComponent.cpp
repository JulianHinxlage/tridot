//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "MeshComponent.h"
#include "entity/Scene.h"
#include "render/Renderer.h"
#include "render/Window.h"
#include "Transform.h"
#include "Camera.h"
#include "Skybox.h"
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
                env->renderer->stats.cameraCount++;

                //frame buffer
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
                    Transform t;
                    t.decompose(transform.getMatrix());
                    glm::vec3 position = t.position;
                    t.position = {0, 0, 0};
                    t.scale = {1, 1, 1};
                    glm::vec3 direction = t.calculateLocalMatrix() * glm::vec4(0, 0, -1, 0);
                    env->renderer->submit(position, direction, light);
                });

                env->renderer->setCamera(camera.projection, cameraTransform.position, camera.output);

                env->scene->view<Skybox>().each([](Skybox& skybox) {
                    if (skybox.useIBL && skybox.irradianceMap.get() != nullptr) {
                        env->renderer->setEnvironMap(skybox.texture, skybox.irradianceMap, skybox.intensity);
                    }
                });

                {
                    TRI_PROFILE("submit");
                    //meshes
                    env->scene->view<MeshComponent, Transform>().each([](EntityId id, MeshComponent& mesh, Transform& transform) {
                        env->renderer->submit(transform.getMatrix(), transform.position, mesh.mesh.get(), mesh.material.get(), mesh.color, id);
                    });
                }

            }
            if(env->editor || !camera.isPrimary){
                camera.active = false;
            }
        });

    }

}
