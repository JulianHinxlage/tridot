//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "editor/Editor.h"
#include "window/UIManager.h"
#include "window/Window.h"
#include "render/objects/Material.h"
#include "engine/AssetManager.h"
#include "engine/Transform.h"
#include "animation/Animation.h"
#include "animation/AnimationComponent.h"
#include <imgui/imgui.h>

namespace tri {

	class AnimationWindow : public UIWindow {
	public:
		Ref<Animation> animation;
		EntityId recordEntityId = -1;
		float keyTime = 1.0f;
		KeyFrameBlendMode keyBlend = KeyFrameBlendMode::LINEAR;
		int currentFrame = 0;
		Prefab entityBuffer;
		glm::mat4 startTransform;

		void init() override {
			env->uiManager->addWindow<AnimationWindow>("Animations");
			if (env->editor) {
				env->editor->fileAssosiations[".anim"] = Reflection::getClassId<Animation>();
			}
		}

		void startup() override {
			env->eventManager->onRuntimeModeChange.addListener([&](int prev, int mode) {
				resetToStart();
			});
		}

		void shutdown() override {}

		void selection() {
			env->editor->classUI->enableReferenceEdit = false;
			env->editor->classUI->draw(animation, "animation");
			env->editor->classUI->enableReferenceEdit = true;
			if (ImGui::Button("Save")) {
				if (animation) {
					auto file = env->assetManager->getFile(animation);
					if (!file.empty()) {
						file = env->assetManager->searchFile(file);
						animation->save(file);
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Save All")) {
				for (auto& file : env->assetManager->getAssetList(Reflection::getClassId<Animation>())) {
					if (!file.empty()) {
						auto anim = env->assetManager->get<Animation>(file);
						if (anim) {
							file = env->assetManager->searchFile(file);
							anim->save(file);
						}
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Reload")) {
				if (animation) {
					auto file = env->assetManager->getFile(animation);
					if (!file.empty()) {
						env->assetManager->reload(file);
					}
				}
			}

			ImGui::Separator();
			if (animation) {
				env->editor->classUI->draw(*animation.get());
			}
		}

		Ref<Animation> getAnimationFromEntity(EntityId id) {
			if (auto* anim = env->world->getComponent<AnimationComponent>(id)) {
				return anim->animation;
			}
			return nullptr;
		}

		void applyFrame(int frame) {
			currentFrame = frame;

			if (animation && animation->keyFrames.size() > 0) {
				if (currentFrame < 0) {
					currentFrame += animation->keyFrames.size();
				}
				else if(currentFrame >= animation->keyFrames.size()) {
					currentFrame -= animation->keyFrames.size();
				}

				if (currentFrame >= 0 && currentFrame < animation->keyFrames.size()) {
					animation->apply(animation->keyFrames[currentFrame].time, recordEntityId);
					entityBuffer.copyEntity(recordEntityId);
				}
			}
		}

		bool hasPropertyInAnimation(int classId, int propertyIndex) {
			if (animation) {
				for (auto& frame : animation->keyFrames) {
					for (auto& prop : frame.properties) {
						if (prop.value.classId == classId) {
							if (prop.value.propertyIndex == propertyIndex) {
								return true;
							}
						}
					}
				}
			}
			return false;
		}

		void checkForNewProperties() {
			for (auto* desc : Reflection::getDescriptors()) {
				if (desc && (desc->flags & ClassDescriptor::COMPONENT)) {
					if (void* comp1 = env->world->getComponent(recordEntityId, desc->classId)) {
						if (void *comp2 = entityBuffer.getComponent(desc->classId)) {
							for (int propertyIndex = 0; propertyIndex < desc->properties.size(); propertyIndex++) {
								auto& propDesc = desc->properties[propertyIndex];
								if (!hasPropertyInAnimation(desc->classId, propertyIndex)) {
									if (propDesc.type->hasEquals() && !propDesc.type->equals((uint8_t*)comp1 + propDesc.offset, (uint8_t*)comp2 + propDesc.offset)) {
									
										if (animation) {
											if (!(currentFrame >= 0 && currentFrame < animation->keyFrames.size())) {
												KeyFrame frame;
												frame.blend = keyBlend;
												animation->keyFrames.push_back(frame);
												currentFrame = 0;
											}
											
											for (int i = 0; i < animation->keyFrames.size(); i++) {
												auto& frame = animation->keyFrames[i];
												KeyFrameProperty prop;
												prop.value.classId = desc->classId;
												prop.value.propertyIndex = propertyIndex;
												prop.value.value.set(propDesc.type->classId, (uint8_t*)comp2 + propDesc.offset);
												frame.properties.push_back(prop);
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

		void updateFrame() {
			if (animation) {
				checkForNewProperties();
				if (currentFrame >= 0 && currentFrame < animation->keyFrames.size()) {
					auto& frame = animation->keyFrames[currentFrame];
					for (auto& prop : frame.properties) {
						void* value = prop.value.getProperty(recordEntityId, env->world);
						prop.value.value.set(prop.value.value.classId, value);
					}
				}
				entityBuffer.copyEntity(recordEntityId);
			}
		}

		void addFrame() {
			if (animation) {
				checkForNewProperties();
				if (currentFrame >= 0 && currentFrame < animation->keyFrames.size()) {
					auto& frame = animation->keyFrames[currentFrame];
					KeyFrame newFrame;
					newFrame.time = frame.time + keyTime;
					newFrame.blend = keyBlend;

					for (auto& prop : frame.properties) {
						KeyFrameProperty newProp;
						newProp.value.classId = prop.value.classId;
						newProp.value.propertyIndex = prop.value.propertyIndex;
						
						void* value = newProp.value.getProperty(recordEntityId, env->world);
						if (auto* desc = newProp.value.getPropertyDescriptor()) {
							newProp.value.value.set(desc->type->classId, value);
						}
						newFrame.properties.push_back(newProp);
					}

					animation->keyFrames.insert(animation->keyFrames.begin() + currentFrame + 1, newFrame);
				}
				for (int i = currentFrame + 2; i < animation->keyFrames.size(); i++) {
					animation->keyFrames[i].time += keyTime;
				}
				currentFrame++;
				entityBuffer.copyEntity(recordEntityId);
			}
		}

		void removeFrame() {
			if (animation) {
				if (currentFrame >= 0 && currentFrame < animation->keyFrames.size()) {
					animation->keyFrames.erase(animation->keyFrames.begin() + currentFrame);
					for (int i = currentFrame; i < animation->keyFrames.size(); i++) {
						animation->keyFrames[i].time -= keyTime;
					}
				}
				currentFrame--;
				entityBuffer.copyEntity(recordEntityId);
			}
		}

		void relativeSpace() {
			if (auto* t = env->world->getComponent<Transform>(recordEntityId)) {
				t->decompose(glm::inverse(startTransform) * t->calculateLocalMatrix());
			}
		}

		void unrelativeSpace() {
			if (auto* t = env->world->getComponent<Transform>(recordEntityId)) {
				t->decompose(startTransform * t->calculateLocalMatrix());
			}
		}

		void resetToStart() {
			if (recordEntityId != -1) {
				relativeSpace();
				currentFrame = 0;
				applyFrame(currentFrame);
				unrelativeSpace();
			}
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
				if (ImGui::Begin("Animations", &active)) {
					if (env->editor) {
						selection();

						if (ImGui::Button("Record")) {
							recordEntityId = -1;
							if (env->editor->selectionContext->isSingleSelection()) {
								recordEntityId = env->editor->selectionContext->getSelected()[0];
								if (auto* t = env->world->getComponent<Transform>(recordEntityId)) {
									startTransform = t->calculateLocalMatrix();
								}
								relativeSpace();
								animation = getAnimationFromEntity(recordEntityId);
								if (animation && animation->keyFrames.size() > 0) {
									currentFrame = 0;
									applyFrame(currentFrame);
								}
								else {
									currentFrame = -1;
									entityBuffer.copyEntity(recordEntityId);
								}
								unrelativeSpace();
							}
						}
						if (ImGui::Button("Reset")) {
							resetToStart();
							recordEntityId = -1;
							animation = nullptr;
						}

						if (recordEntityId != -1) {
							ImGui::InputFloat("key time", &keyTime);
							env->editor->classUI->draw(keyBlend, "key blend");

							if (ImGui::Button("Next Frame")) {
								relativeSpace();
								applyFrame(currentFrame + 1);
								unrelativeSpace();
							}

							if (ImGui::Button("Prev Frame")) {
								relativeSpace();
								applyFrame(currentFrame - 1);
								unrelativeSpace();
							}

							if (ImGui::Button("Update Frame")) {
								relativeSpace();
								updateFrame();
								unrelativeSpace();
							}

							if (ImGui::Button("Add Frame")) {
								relativeSpace();
								addFrame();
								unrelativeSpace();
							}

							if(ImGui::Button("Remove Frame")){
								relativeSpace();
								removeFrame();
								unrelativeSpace();
							}

						}
					}
				}
				ImGui::End();
			}
		}
	};
	TRI_SYSTEM(AnimationWindow);

}