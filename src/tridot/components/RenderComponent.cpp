//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "RenderComponent.h"
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
        auto render = [](const glm::mat4 &projection, glm::vec3 cameraPosition, const Ref<FrameBuffer> &frameBuffer){
            engine.renderer.begin(projection, cameraPosition, frameBuffer);
            engine.pbRenderer.begin(projection, cameraPosition, frameBuffer);

            engine.view<Light>().each([](Light &light){
               engine.pbRenderer.submit(light);
            });

            engine.view<Transform, RenderComponent>().each([&](ecs::EntityId id, Transform &transform, RenderComponent &rc){
                engine.pbRenderer.submit(transform.getMatrix(), rc.color, rc.mesh.get(), rc.material.get());
            });
            engine.renderer.end();
            engine.pbRenderer.end();
        };

        engine.view<PerspectiveCamera>().each([&](PerspectiveCamera &camera){
            if(camera.target.get() == nullptr){
                camera.aspectRatio = engine.window.getAspectRatio();
            }
            render(camera.getProjection(), camera.position, camera.target);
        });
        engine.view<OrthographicCamera>().each([&](OrthographicCamera &camera){
            if(camera.target.get() == nullptr) {
                camera.aspectRatio = engine.window.getAspectRatio();
            }
            render(camera.getProjection(), glm::vec3(camera.position, 0.0f), camera.target);
        });
    }

}