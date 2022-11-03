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

		std::string toString(int value, const std::string &seperator = " ") {
			std::string result;
			if (value == 0) {
				result = "0";
			}

			int last = value * 1000;
			while (last >= 1000) {
				int v = value % 1000;
				if (result.empty()) {
					result = std::to_string(v);
				}
				else {
					result = std::to_string(v) + seperator + result;
				}
				if (value >= 1000) {
					if (v < 100) {
						result.insert(result.begin(), '0');
					}
					if (v < 10) {
						result.insert(result.begin(), '0');
					}
					if (v < 1) {
						result.insert(result.begin(), '0');
					}
				}

				last = value;
				value /= 1000;
			}
			return result;
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
				if (ImGui::Begin("Render Statistics", &active)) {
					ImGui::Text("draw calls: %s", toString(env->renderSettings->statistics.drawCallCount).c_str());
					ImGui::Text("instances:  %s", toString(env->renderSettings->statistics.instanceCount).c_str());
					ImGui::Text("triangles:  %s", toString(env->renderSettings->statistics.triangleCount).c_str());
					ImGui::Text("shaders:    %i", env->renderSettings->statistics.shaderCount);
					ImGui::Text("meshs:      %i", env->renderSettings->statistics.meshCount);
					ImGui::Text("lights:     %i", env->renderSettings->statistics.lightCount);
					ImGui::Text("cameras:    %i", env->renderSettings->statistics.cameraCount);
					ImGui::Text("materials:  %i", env->renderSettings->statistics.materialCount);

					ImGui::Separator();

					ImGui::Checkbox("VSync", env->console->getCVar("vsync")->getPtr<bool>());
					ImGui::Checkbox("enableTransparency", &env->renderSettings->enableTransparency);
					ImGui::Checkbox("enableFrustumCulling", &env->renderSettings->enableFrustumCulling);
					ImGui::Checkbox("enablePointLights", &env->renderSettings->enablePointLights);
					ImGui::Checkbox("enableSpotLights", &env->renderSettings->enableSpotLights);

					ImGui::Checkbox("enableBloom", &env->renderSettings->enableBloom);
					ImGui::SliderFloat("bloomThreshold", &env->renderSettings->bloomThreshold, 0, 2);
					ImGui::SliderFloat("bloomIntesity", &env->renderSettings->bloomIntesity, 0, 2);
					ImGui::SliderInt("bloomSpread", &env->renderSettings->bloomSpread, 0, 50);

					ImGui::Checkbox("enableSSAO", &env->renderSettings->enableSSAO);
					ImGui::SliderInt("ssaoKernalSize", &env->renderSettings->ssaoKernalSize, 0, 256);
					ImGui::SliderFloat("ssaoSampleRadius", &env->renderSettings->ssaoSampleRadius, 0, 10);
					ImGui::SliderFloat("ssaoBias", &env->renderSettings->ssaoBias, 0, 1);
					ImGui::SliderFloat("ssaoOcclusionStrength", &env->renderSettings->ssaoOcclusionStrength, 0, 10);

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