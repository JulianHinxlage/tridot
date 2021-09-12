//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Editor.h"
#include "engine/Camera.h"
#include "engine/Transform.h"
#include "engine/MeshComponent.h"
#include "entity/Scene.h"
#include "render/Window.h"
#include <imgui/imgui.h>

float randf() {
	return (float)std::rand() / RAND_MAX;
}

glm::vec3 randf3() {
	return { randf(), randf(), randf(), };
}

namespace tri {

	class ViewportWindow : public EditorWindow {
	public:
		EntityId editorCameraId;

		void startup() {
			name = "Viewport";
			isWindow = false;
			editorCameraId = env->scene->addEntity();
			Transform& t = env->scene->addComponent<Transform>(editorCameraId);
			t.position = { 0.5, 0.5, 2 };
			Camera& cam = env->scene->addComponent<Camera>(editorCameraId);
			cam.output = cam.output.make();
			cam.output->setAttachment({ COLOR, env->window->getBackgroundColor() });
			cam.output->setAttachment({ DEPTH });

			for (int i = 0; i < 1000; i++) {
				env->scene->addEntity(Transform(randf3(), (randf3() * 0.1f) + glm::vec3(0.01, 0.01, 0.01), glm::vec3(0, 0, randf() * 6)), MeshComponent(nullptr, nullptr, Color(glm::vec4(randf3(), 1))));
			}
		}

		void update() override {
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0);
			if (ImGui::Begin(name.c_str(), &isOpen)) {

				Ref<FrameBuffer> output = nullptr;
				ImVec2 viewportSize = ImGui::GetContentRegionAvail();
				env->scene->view<Camera>().each([&](EntityId id, Camera& camera) {
					if (editor->runtimeMode) {
						if (camera.isPrimary) {
							output = camera.output;
						}
					}
					else {
						if (id == editorCameraId) {
							output = camera.output;
							if (camera.output->getSize() != glm::vec2(viewportSize.x, viewportSize.y)) {
								camera.output->resize(viewportSize.x, viewportSize.y);
								camera.aspectRatio = viewportSize.x / viewportSize.y;
							}
						}
					}
				});

				if (output != nullptr) {
					ImGui::Image((ImTextureID)output->getAttachment(TextureAttachment::COLOR)->getId(), viewportSize);
				}
			}
			ImGui::End();
			ImGui::PopStyleVar(2);
		}
	};
	TRI_STARTUP_CALLBACK("") {
		editor->addWindow(new ViewportWindow);
	}

}
