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
#include "engine/MeshComponent.h"
#include "render/renderer/OutlineRenderer.h"
#include "Gizmos.h"
#include "engine/RuntimeMode.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace tri {

	class ViewportWindow : public UIWindow {
	public:
		EditorCamera editorCamera;
		EntityId editorCameraEntity;
		int editorCameraPersistandHandle;
		std::vector<int> persistentHandles;

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
						env->viewport->size = glm::vec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
						env->viewport->position = glm::vec2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);
						env->viewport->position += glm::vec2(ImGui::GetCursorPos().x, ImGui::GetCursorPos().y);
						env->viewport->position -= env->window->getPosition();
					}
					if (env->viewport->frameBuffer) {
						//draw rendered image
						auto curserPos = ImGui::GetCursorPos();
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

						{
							//make camera persistent
							editorCameraPersistandHandle = env->editor->setPersistentEntity(editorCameraEntity, editorCameraPersistandHandle);
							EntityId id = editorCameraEntity;
							int i = 0;
							while (true) {
								if (auto* t = env->world->getComponent<Transform>(id)) {
									if (t && t->parent != -1) {
										id = t->parent;
										if (persistentHandles.size() <= i) {
											persistentHandles.resize(i + 1, -1);
										}
										persistentHandles[i] = env->editor->setPersistentEntity(id, persistentHandles[i]);
										i++;
										continue;
									}
								}
								break;
							}
						}



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

													env->renderPipeline->addCallbackStep([pos]() {
														Color idColor = env->viewport->idMap->getPixel(pos.x, pos.y);
														if (idColor.value != -1) {
															EntityId id = idColor.value & ~(0xff << 24);
															if (env->editor->selectionContext->isSelected(id) && env->input->downControl()) {
																env->editor->selectionContext->unselect(id);
															}
															else {
																env->editor->selectionContext->select(id, !env->input->downControl());
															}
														}
														else {
															env->editor->selectionContext->unselectAll();
														}
													}, RenderPipeline::DISPLAY);

												}
											}
										}
									}
								}
							}
						}

						//outlines
						OutlineRenderer *renderer = env->systemManager->getSystem<OutlineRenderer>();
						if (renderer) {
							for (auto &id : env->editor->selectionContext->getSelected()) {
								auto* transform = env->world->getComponent<Transform>(id);
								auto* mesh = env->world->getComponent<MeshComponent>(id);
								if (transform && mesh) {
									if (mesh->mesh) {
										renderer->submit(transform->getMatrix(), mesh->mesh.get());
									}
								}
							}

							if (camera) {
								renderer->submitBatches(camera->viewProjection);
							}
							auto& frameBuffer = renderer->getFrameBuffer();
							if (frameBuffer) {
								ImGui::SetCursorPos(curserPos);
								ImGui::Image((ImTextureID)(size_t)frameBuffer->getAttachment(TextureAttachment::COLOR)->getId(), ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
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