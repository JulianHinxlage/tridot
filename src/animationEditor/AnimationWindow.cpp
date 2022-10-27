//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "editor/Editor.h"
#include "window/UIManager.h"
#include "window/Window.h"
#include "render/objects/Material.h"
#include "engine/AssetManager.h"
#include "animation/Animation.h"
#include "animation/AnimationComponent.h"
#include <imgui/imgui.h>

namespace tri {

	class AnimationWindow : public UIWindow {
	public:
		Ref<Animation> animation;
		EntityId recordEntityId = -1;
		float keyTiming = 1.0f;
		int currentFrame = 0;

		void init() override {
			env->uiManager->addWindow<AnimationWindow>("Animations");
			if (env->editor) {
				env->editor->fileAssosiations[".anim"] = Reflection::getClassId<Animation>();
			}
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

			if (animation && animation->timeline.size() > 0) {
				auto &seq = animation->timeline[0];

				if (currentFrame < 0) {
					currentFrame += seq.frames.size();
				}
				else if(currentFrame >= seq.frames.size()) {
					currentFrame -= seq.frames.size();
				}

				if (currentFrame >= 0 && currentFrame < seq.frames.size()) {
					auto &f = seq.frames[currentFrame];
					animation->apply(f.time, recordEntityId);
				}
			}
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
				if (ImGui::Begin("Animations", &active)) {
					selection();

					if (ImGui::Button("Record")) {
						recordEntityId = -1;
						if (env->editor->selectionContext->isSingleSelection()) {
							recordEntityId = env->editor->selectionContext->getSelected()[0];
							animation = getAnimationFromEntity(recordEntityId);
							if (animation && animation->timeline.size() > 0) {
								currentFrame = animation->timeline[0].frames.size() - 1;
								applyFrame(currentFrame);
							}
							else {
								applyFrame(0);
								currentFrame = -1;
							}
						}
					}

					if (recordEntityId != -1) {
						ImGui::InputFloat("key timing", &keyTiming);

						if (ImGui::Button("Next Frame")) {
							applyFrame(currentFrame + 1);
						}

						if (ImGui::Button("Prev Frame")) {
							applyFrame(currentFrame - 1);
						}

						if (ImGui::Button("Add Frame")) {
							if (animation) {
								for (auto& s : animation->timeline) {
									if (s.frames.size() > 0){
										auto &f = s.frames[s.frames.size() - 1];

										PropertyFrame frame;
										frame.blend = f.blend;
										frame.time = f.time + keyTiming;
										frame.relativeValue = f.relativeValue;
										frame.value.classId = f.value.classId;
										frame.value.propertyIndex = f.value.propertyIndex;
										void *prop = nullptr;
										prop = frame.value.getProperty(recordEntityId, env->world);
										if (auto* desc = frame.value.getPropertyDescriptor()) {
											frame.value.value.set(desc->type->classId, prop);
										}
										s.frames.push_back(frame);
									}
								}
								currentFrame++;
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