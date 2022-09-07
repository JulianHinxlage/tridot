//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "window/UIManager.h"
#include "window/Window.h"
#include "render/renderer/RenderSettings.h"
#include "render/renderer/Renderer.h"
#include "editor/Editor.h"
#include <imgui/imgui.h>

namespace tri {

	class RenderStatisticsWindow : public UIWindow {
	public:
		void init() override {
			env->systemManager->getSystem<UIManager>()->addWindow<RenderStatisticsWindow>("Render Statistics", "Debug");
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
				if (ImGui::Begin("Render Statistics", &active)) {
					ImGui::Text("draw calls: %i", env->renderSettings->statistics.drawCallCount);
					ImGui::Text("instances:  %i", env->renderSettings->statistics.instanceCount);
					ImGui::Text("triangles:  %i", env->renderSettings->statistics.triangleCount);
					ImGui::Text("shaders:    %i", env->renderSettings->statistics.shaderCount);
					ImGui::Text("meshs:      %i", env->renderSettings->statistics.meshCount);
					ImGui::Text("lights:     %i", env->renderSettings->statistics.lightCount);
					ImGui::Text("cameras:    %i", env->renderSettings->statistics.cameraCount);
					ImGui::Text("materials:  %i", env->renderSettings->statistics.materialCount);

					env->editor->classUI->draw(env->systemManager->getSystem<Renderer>()->getGBuffer(), "GBuffer");
				}
				ImGui::End();
			}
		}
	};

	TRI_SYSTEM(RenderStatisticsWindow);

}