//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "SimpleRenderer.h"
#include "entity/World.h"
#include "engine/AssetManager.h"
#include "engine/MeshComponent.h"
#include "engine/Transform.h"
#include "engine/Camera.h"
#include "window/RenderContext.h"
#include "window/Viewport.h"
#include <GL/glew.h>
#include <tracy/TracyOpenGL.hpp>

namespace tri {

	TRI_SYSTEM(SimpleRenderer);

    void SimpleRenderer::init() {
        auto* job = env->jobManager->addJob("Render");
        job->addSystem<SimpleRenderer>();
        job->orderSystems({ "SimpleRenderer", "ViewportWindow" });
    }

	void SimpleRenderer::startup() {

        //glew needs to be initialized in every dll, so for now we do it here
        //we cant use the RenderContext class, because the functions are implemented in another dll
        glewInit();

        Image image;
        image.init(1, 1, 4, 8);
        image.set(0, 0, color::white);
        defaultTexture = Ref<Texture>::make();
        defaultTexture->load(image);
        defaultMaterial = Ref<Material>::make();
        defaultMesh = Ref<Mesh>::make();
        float quadVertices[] = {
            -0.5, +0.0, -0.5, 0.0, 1.0, 0.0, 0.0, 0.0,
            +0.5, +0.0, -0.5, 0.0, 1.0, 0.0, 1.0, 0.0,
            +0.5, +0.0, +0.5, 0.0, 1.0, 0.0, 1.0, 1.0,
            -0.5, +0.0, +0.5, 0.0, 1.0, 0.0, 0.0, 1.0,
        };
        int quadIndices[] = {
            0, 2, 1,
            0, 3, 2,
        };
        defaultMesh->create(quadVertices, sizeof(quadVertices) / sizeof(quadVertices[0]), quadIndices, sizeof(quadIndices) / sizeof(quadIndices[0]), { {FLOAT, 3}, {FLOAT, 3} ,{FLOAT, 2} });

        defaultShader = env->assetManager->get<Shader>("shaders/simple.glsl");
        projection = glm::mat4(1);
	}

    void SimpleRenderer::shutdown() {
        defaultTexture = nullptr;
        defaultMaterial = nullptr;
        defaultMesh = nullptr;
        defaultMesh = nullptr;
        defaultShader = nullptr;
        frameBuffer = nullptr;
    }

	void SimpleRenderer::tick() {
        if (!env->viewport->displayInWindow) {
            if (!env->viewport->frameBuffer) {
                env->viewport->frameBuffer = Ref<FrameBuffer>::make();
                env->viewport->frameBuffer->setAttachment({ COLOR, Color(0, 0, 0, 0) });
                env->viewport->frameBuffer->setAttachment({ DEPTH, color::white });
                env->viewport->frameBuffer->setAttachment({ (TextureAttachment)(COLOR + 1), color::white });//ID buffer
            } 
            if(env->viewport->size != glm::ivec2(env->viewport->frameBuffer->getSize())) {
                env->viewport->frameBuffer->resize(env->viewport->size.x, env->viewport->size.y);
            }
            frameBuffer = env->viewport->frameBuffer;
        }
        else {
            frameBuffer = nullptr;
        }

        RenderContext::setDepth(true);

        if (frameBuffer) {
            TracyGpuZone("clear");
            frameBuffer->clear();
        }

        env->world->each<Camera>([&](Camera& c) {
            if (env->viewport->size.y != 0) {
                c.aspectRatio = (float)env->viewport->size.x / (float)env->viewport->size.y;
            }
            projection = c.viewProjection;
        });
		env->world->each<const Transform, const MeshComponent>([&](EntityId id, const Transform &t, const MeshComponent &m) {
			submit(t.getMatrix(), m.mesh.get(), m.material.get(), m.color, id);
		});
        FrameBuffer::unbind();
	}

	void SimpleRenderer::submit(const glm::mat4& transform, Mesh* mesh, Material* material, Color color, EntityId id) {
        if (!mesh) {
            mesh = defaultMesh.get();
        }
        if (!material) {
            material = defaultMaterial.get();
        }
        Shader* shader = material->shader.get();
        if (!shader) {
            shader = defaultShader.get();
        }
        Texture *texture = material->texture.get();
        if (!texture) {
            texture = defaultTexture.get();
        }

        if (shader->getId() == 0) {
            return;
        }

        shader->bind();
        shader->set("uTransform", transform);
        shader->set("uProjection", projection);

        texture->bind(0);
        int texId = 0;
        shader->set("uTextures", &texId, 1);
        shader->set("uColor", color.vec() * material->color.vec());

        Color idColor(id | (0xff << 24));
        shader->set("uId", idColor.vec());

        if (frameBuffer) {
            frameBuffer->bind();
        }
        else {
            FrameBuffer::unbind();
        }

        {
            TracyGpuZone("submit");
            mesh->vertexArray.submit();
        }
	}

}