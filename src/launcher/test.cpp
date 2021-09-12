//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "core/core.h"
#include "render/Renderer.h"
#include "entity/Scene.h"
#include "engine/Transform.h"
#include "engine/MeshComponent.h"
#include "engine/Camera.h"

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

		env->scene->addEntity(Transform({ 0.5, 0.5, 2 }), Camera(Camera::PERSPECTIVE, true));

		for (int i = 0; i < 1000; i++) {
			if (i > 500) {
				env->scene->addEntity(Transform(randf3(), randf3() * 0.001f, glm::vec3(0, 0, randf() * 6)), MeshComponent(teapot, nullptr, Color(glm::vec4(randf3(), 1))));
			}
			else {
				env->scene->addEntity(Transform(randf3(), randf3() * 0.1f, glm::vec3(0, 0, randf() * 6)), MeshComponent(nullptr, nullptr, Color(glm::vec4(randf3(), 1))));
			}
		}
	}

};
TRI_REGISTER_SYSTEM(Test);
