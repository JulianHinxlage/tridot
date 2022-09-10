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

					ImGui::Separator();

					ImGui::Checkbox("enableTransparency", &env->renderSettings->enableTransparency);
					ImGui::Checkbox("enablePointLights", &env->renderSettings->enablePointLights);
					ImGui::Checkbox("enableSpotLights", &env->renderSettings->enableSpotLights);
					ImGui::Checkbox("enableBloom", &env->renderSettings->enableBloom);
					ImGui::SliderFloat("bloomThreshold", &env->renderSettings->bloomThreshold, 0, 2);
					ImGui::SliderFloat("bloomIntesity", &env->renderSettings->bloomIntesity, 0, 2);
					ImGui::SliderInt("bloomSpread", &env->renderSettings->bloomSpread, 0, 50);
					
					ImGui::Separator();

					env->editor->classUI->draw(env->systemManager->getSystem<Renderer>()->getGBuffer(), "GBuffer");
					env->editor->classUI->draw(env->systemManager->getSystem<Renderer>()->getBloomBuffer(), "Bloom Buffer");
				}
				ImGui::End();
			}
		}
	};

	TRI_SYSTEM(RenderStatisticsWindow);

}