//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "window/UIManager.h"
#include "window/Window.h"
#include "window/Viewport.h"
#include "window/Input.h"
#include "render/objects/FrameBuffer.h"
#include "engine/RuntimeMode.h"
#include "Gizmos.h"
#include <imgui/imgui.h>

namespace tri {

	class ViewportSettingsWindow : public UIWindow {
	public:
		void init() override {
			env->systemManager->getSystem<UIManager>()->addWindow<ViewportSettingsWindow>("Viewport Settings");
		}

		void shutdown() override {
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
				if (ImGui::Begin("Viewport Settings", &active)) {

					auto mode = env->runtimeMode->getMode();
					bool play = (mode == RuntimeMode::PLAY || mode == RuntimeMode::PAUSED);
					if (ImGui::Checkbox("Play", &play)) {
						env->runtimeMode->setMode(play ? RuntimeMode::PLAY : RuntimeMode::EDIT);
					}
					bool paused = (mode == RuntimeMode::PAUSED);
					if (ImGui::Checkbox("Pause", &paused)) {
						env->runtimeMode->setMode(paused ? RuntimeMode::PAUSED : RuntimeMode::PLAY);
					}

					env->systemManager->getSystem<Gizmos>()->updateSettings();
				}
				ImGui::End();
			}
		}
	};
	TRI_SYSTEM(ViewportSettingsWindow);

}