//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "RenderComponent.h"
#include "tridot/engine/Engine.h"
#include "tridot/render/Camera.h"

namespace tridot {

    RenderComponent &RenderComponent::setMesh(const Ref<Mesh> &mesh){
        this->mesh = mesh;
        return *this;
    }

    RenderComponent &RenderComponent::setTexture(const Ref<Texture> &texture){
        this->texture = texture;
        return *this;
    }

    RenderComponent &RenderComponent::setShader(const Ref<Shader> &shader){
        this->shader = shader;
        return *this;
    }

    TRI_UPDATE("rendering"){
        auto render = [](const glm::mat4 &projection, glm::vec3 cameraPosition, const Ref<FrameBuffer> &frameBuffer){
            engine.renderer.begin(projection, frameBuffer);
            engine.view<Transform, RenderComponent>().each([&](ecs::EntityId id, Transform &transform, RenderComponent &rc){
                engine.renderer.submit(
                        {transform.position, transform.scale, transform.rotation, rc.color, {0, 0},
                         rc.textureScale}, rc.texture.get(), rc.mesh.get(),rc.shader.get());
            });
            engine.renderer.end();
        };

        engine.view<PerspectiveCamera>().each([&](PerspectiveCamera &camera){
            camera.aspectRatio = engine.window.getAspectRatio();
            render(camera.getProjection(), camera.position, camera.target);
        });
        engine.view<OrthographicCamera>().each([&](OrthographicCamera &camera){
            camera.aspectRatio = engine.window.getAspectRatio();
            render(camera.getProjection(), glm::vec3(camera.position, 0.0f), camera.target);
        });
    }

}