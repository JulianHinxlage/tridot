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

	TRI_REGISTER_TYPE(MeshComponent);
	TRI_REGISTER_MEMBER(MeshComponent, mesh);
	TRI_REGISTER_MEMBER(MeshComponent, material);
	TRI_REGISTER_MEMBER(MeshComponent, color);

	TRI_UPDATE_CALLBACK("MeshComponent") {
		env->scene->view<Camera, Transform>().each([](Camera& camera, Transform &cameraTransform) {
			if (camera.active) {
				if (camera.output) {
					if (camera.isPrimary) {
						if (camera.output->getSize() != env->window->getSize()) {
							camera.output->resize(env->window->getSize().x, env->window->getSize().y);
							camera.aspectRatio = env->window->getAspectRatio();
						}
					}
					camera.output->clear();
				}
				else {
					if (camera.isPrimary) {
						camera.aspectRatio = env->window->getAspectRatio();
					}
				}
				env->renderer->beginScene(camera.projection, cameraTransform.position);
				env->scene->view<MeshComponent, Transform>().each([](EntityId id, MeshComponent& mesh, Transform& transform) {
					env->renderer->submit(transform.calculateMatrix(), transform.position, mesh.mesh.get(), mesh.material.get(), mesh.color, id);
				});
				env->renderer->drawScene(camera.output, camera.pipeline);
				env->renderer->resetScene();
			}
		});
	}

}
