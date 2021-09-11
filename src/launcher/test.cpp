//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "core/core.h"
#include "render/Renderer.h"
#include "entity/Scene.h"
#include "engine/Transform.h"

using namespace tri;

float randf() {
	return (float)std::rand() / RAND_MAX;
}

glm::vec3 randf3() {
	return { randf(), randf(), randf(), };
}

class Test : public System {
public:
	void startup() override {
		Ref<Mesh> teapot;
		teapot = teapot.make();
		teapot->load("../res/models/teapot.obj");

		for (int i = 0; i < 1000; i++) {
			if (i > 500) {
				env->scene->addEntity(Transform(randf3(), randf3() * 0.001f, glm::vec3(0, 0, randf() * 6)), Color(glm::vec4(randf3(), 1)), teapot);
			}
			else {
				env->scene->addEntity(Transform(randf3(), randf3() * 0.1f, glm::vec3(0, 0, randf() * 6)), Color(glm::vec4(randf3(), 1)), Ref<Mesh>());
			}
		}
	}
	void update() override {
		Transform cam;
		cam.position = { 0.5, 0.5, 2 };
		cam.scale *= 0.5;
		cam.scale.z = 10;
		env->renderer->beginScene(glm::inverse(cam.getLocalMatrix()), cam.position);
		env->scene->view<Transform, Color, Ref<Mesh>>().each([this](Transform& t, Color& c, Ref<Mesh>& mesh) {
			env->renderer->submit(t.getLocalMatrix(), t.position, mesh.get(), nullptr, c);
		});
		env->renderer->drawScene();
		env->renderer->resetScene();
	}
};
TRI_REGISTER_SYSTEM(Test);
