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
#include "engine/Serializer.h"
#include <imgui/imgui.h>

float randf() {
	return (float)std::rand() / RAND_MAX;
}

glm::vec3 randf3() {
	return { randf(), randf(), randf(), };
}

class EditorOnly{};
TRI_REGISTER_TYPE(EditorOnly);

namespace tri {

	class ViewportWindow : public EditorWindow {
	public:
		EntityId editorCameraId;

		void startup() {
			name = "Viewport";
			isWindow = false;
            editorCameraId = -1;

            env->signals->sceneLoad.addCallback([&](Scene *scene){
                editorCameraId = -1;
            });
		}

        void setupCamera(){
            //search for camera in scene
            editorCameraId = -1;
            env->scene->view<Camera, EditorOnly>().each([&](EntityId id, Camera &cam, EditorOnly &){
                if(editorCameraId == -1){
                    editorCameraId = id;
                }
            });

            if(editorCameraId != -1) {
                //setup frame buffer
                Camera &cam = env->scene->getComponent<Camera>(editorCameraId);
                cam.output = cam.output.make();
                cam.output->setAttachment({COLOR, env->window->getBackgroundColor()});
                cam.output->setAttachment({DEPTH});
            }else{
                //create camera
                editorCameraId = env->scene->addEntity(EditorOnly());
                Transform& t = env->scene->addComponent<Transform>(editorCameraId);
                t.position = { 0.5, 0.5, 2 };
                Camera& cam = env->scene->addComponent<Camera>(editorCameraId);
                cam.isPrimary = false;
                cam.active = true;

                //setup frame buffer
                cam.output = cam.output.make();
                cam.output->setAttachment({ COLOR, env->window->getBackgroundColor() });
                cam.output->setAttachment({ DEPTH });
            }
        }

		void update() override {
		    if(editorCameraId == -1){
		        setupCamera();
		    }

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
							if(output) {
                                if (output->getSize() != glm::vec2(viewportSize.x, viewportSize.y)) {
                                    output->resize(viewportSize.x, viewportSize.y);
                                    camera.aspectRatio = viewportSize.x / viewportSize.y;
                                }
                            }
						}
					}
				});

				if (output) {
					ImGui::Image((ImTextureID)(size_t)output->getAttachment(TextureAttachment::COLOR)->getId(), viewportSize, ImVec2(0, 1), ImVec2(1, 0));
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
