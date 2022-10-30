//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "EditorCamera.h"
#include "window/UIManager.h"
#include "window/Window.h"
#include "window/Viewport.h"
#include "window/Input.h"
#include "render/objects/FrameBuffer.h"
#include "Gizmos.h"
#include "engine/RuntimeMode.h"
#include <imgui/imgui.h>

namespace tri {

	class ViewportWindow : public UIWindow {
	public:
		EditorCamera editorCamera;
		EntityId editorCameraEntity;
		int editorCameraPersistandHandle;

		void init() override {
			env->systemManager->addSystem<Viewport>();
			env->systemManager->getSystem<UIManager>()->addWindow<ViewportWindow>("Viewport");
			env->viewport->displayInWindow = false;
			editorCameraPersistandHandle = -1;
		}

		void shutdown() override {
			env->viewport->displayInWindow = true;
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
				if (ImGui::Begin("Viewport", &active)) {
					if (!env->viewport->displayInWindow) {
						env->viewport->size = { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y };
						env->viewport->position = { ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y};
					}
					if (env->viewport->frameBuffer) {
						//draw rendered image
						ImGui::Image((ImTextureID)(size_t)env->viewport->frameBuffer->getAttachment(TextureAttachment::COLOR)->getId(), ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
						
						//camera
						Camera* camera = nullptr;
						Transform* cameraTransform = nullptr;
						bool hasPrimary = false;
						env->world->each<const Camera, const Transform>([&](EntityId id, Camera& c, Transform& t) {
							if (c.isPrimary && !hasPrimary) {
								camera = &c;
								cameraTransform = &t;
								editorCameraEntity = id;
								hasPrimary = true;
							}
						});

						editorCameraPersistandHandle = env->editor->setPersistentEntity(editorCameraEntity, editorCameraPersistandHandle);

						bool usingGizmos = false;
						if (camera) {
							//camera
							if (ImGui::IsItemHovered()) {
								if (env->editor->viewportCameraInPlay || env->runtimeMode->getMode() != RuntimeMode::PLAY) {
									editorCamera.update(*camera, *cameraTransform);
								}
							}
							//gizmos
							glm::vec2 viewportPos = { ImGui::GetWindowPos().x, ImGui::GetWindowPos().y };
							usingGizmos = env->systemManager->getSystem<Gizmos>()->updateGizmo(*camera, viewportPos, env->viewport->size);
						}

						if (!usingGizmos) {
							//mouse picking
							if (env->editor->viewportCameraInPlay || env->runtimeMode->getMode() != RuntimeMode::PLAY) {
								if (env->input->pressed(Input::MOUSE_BUTTON_LEFT)) {
									if (ImGui::IsItemHovered()) {
										glm::vec2 pos = { ImGui::GetMousePos().x - ImGui::GetItemRectMin().x, ImGui::GetMousePos().y - ImGui::GetItemRectMin().y };
										pos.y = env->viewport->size.y - pos.y;
										if (pos.x >= 0 && pos.y >= 0) {
											if (pos.x < env->viewport->size.x && pos.y < env->viewport->size.y) {
												if (env->viewport->idMap) {
													Color idColor = env->viewport->idMap->getPixel(pos.x, pos.y);
													if (idColor.value != -1) {
														EntityId id = idColor.value & ~(0xff << 24);
														env->editor->selectionContext->select(id, !env->input->downControl());
													}
													else {
														env->editor->selectionContext->unselectAll();
													}
												}
											}
										}
									}
								}
							}
						}


					}

				}
				ImGui::End();
				ImGui::PopStyleVar();
			}
		}
	};
	TRI_SYSTEM(ViewportWindow);

}