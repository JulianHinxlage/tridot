//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "RenderComponent.h"
#include "PostProcessingEffect.h"
#include "SkyBox.h"
#include "tridot/engine/Engine.h"
#include "tridot/render/Camera.h"

namespace tridot {

    RenderComponent::RenderComponent(const Color &color)
            : color(color) {}

    RenderComponent &RenderComponent::setMesh(const Ref<Mesh> &mesh){
        this->mesh = mesh;
        return *this;
    }

    RenderComponent &RenderComponent::setMaterial(const Ref<Material> &material) {
        this->material = material;
        return *this;
    }

    RenderComponent &RenderComponent::setTexture(const Ref<Texture> &texture) {
        if(!material){
            material = Ref<Material>::make();
        }
        material->texture = texture;
        return *this;
    }

    RenderComponent &RenderComponent::setShader(const Ref<Shader> &shader) {
        if(!material){
            material = Ref<Material>::make();
        }
        material->shader = shader;
        return *this;
    }

    TRI_UPDATE("rendering"){
        TRI_PROFILE("render");
        auto render = [](const glm::mat4 &projection, glm::vec3 cameraPosition, const Ref<FrameBuffer> &frameBuffer){
            {
                TRI_PROFILE("render/begin");
                engine.pbRenderer.begin(projection, cameraPosition, frameBuffer);
            }

            engine.view<SkyBox>().each([](SkyBox &skybox){
               if(skybox.useEnvironmentMap && skybox.texture.get() != nullptr){
                   engine.pbRenderer.submitEnvironmentMap(skybox.texture, skybox.irradianceTexture, skybox.intensity);
               }
            });

            engine.view<Light, Transform>().each([](Light &light, Transform &transform){
                if(light.type == LightType::POINT_LIGHT){
                    engine.pbRenderer.submit(light, transform.position);
                }else{
                    glm::vec3 direction = glm::vec4(1, 0, 0, 0) * transform.getMatrix();
                    engine.pbRenderer.submit(light, direction);
                }
            });

            {
                TRI_PROFILE("render/submit");
                engine.view<Transform, RenderComponent>().each([&](EntityId id, Transform &transform, RenderComponent &rc){
                    engine.pbRenderer.submit(transform.getMatrix(), rc.color, rc.mesh.get(), rc.material.get(), id);
                });
            }
            {
                TRI_PROFILE("render/end");
                engine.pbRenderer.end();
            }
        };

        engine.view<PerspectiveCamera, Transform>().each([&](PerspectiveCamera &camera, Transform &transform){
            camera.forward = transform.getMatrix() * glm::vec4(0, 0, 1, 0);
            camera.up = transform.getMatrix() * glm::vec4(0, 1, 0, 0);
            camera.right = glm::cross(camera.forward, camera.up);
            if(camera.target.get() == nullptr){
                camera.aspectRatio = engine.window.getAspectRatio();
            }
            render(camera.getProjection() * glm::inverse(transform.getMatrix()), transform.position, camera.target);
            camera.output = camera.target;
        });
        engine.view<OrthographicCamera, Transform>().each([&](OrthographicCamera &camera, Transform &transform){
            camera.right = transform.getMatrix() * glm::vec4(1, 0, 0, 0);
            camera.up = transform.getMatrix() * glm::vec4(0, 1, 0, 0);
            if(camera.target.get() == nullptr) {
                camera.aspectRatio = engine.window.getAspectRatio();
            }
            render(camera.getProjection() * glm::inverse(transform.getMatrix()), transform.position, camera.target);
            camera.output = camera.target;
        });
    }

}