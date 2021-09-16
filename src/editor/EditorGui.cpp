//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "EditorGui.h"
#include "core/core.h"
#include "Editor.h"
#include "render/Color.h"
#include "engine/Transform.h"
#include <glm/glm.hpp>
#include <imgui.h>

namespace tri {

    void EditorGui::drawType(int typeId, void *data) {
        auto *desc = env->reflection->getType(typeId);
        drawMember(typeId, data, desc->name.c_str(), nullptr, nullptr, false);
    }

    void EditorGui::setTypeFunction(int typeId, const std::function<void(const char *, void *, void *, void *)> &func) {
        if(typeFunctions.size() <= typeId){
            typeFunctions.resize(typeId + 1);
        }
        typeFunctions[typeId] = func;
    }

    void EditorGui::drawMember(int typeId, void *data, const char *label, void *min, void *max, bool drawHeader) {
        if(typeFunctions.size() > typeId){
            if(typeFunctions[typeId]){
                typeFunctions[typeId](label, data, min, max);
                return;
            }
        }
        auto *desc = env->reflection->getType(typeId);
        ImGuiTreeNodeFlags flags = 0;
        if(desc->member.size() == 0){
            flags |= ImGuiTreeNodeFlags_Bullet;
        }
        if(!drawHeader || ImGui::TreeNodeEx(desc->name.c_str(), flags)) {
            for(auto &m : desc->member){
                void *ptr = (uint8_t*)data + m.offset;
                if(m.type->constants.size() > 0){
                    drawConstants(m.type->typeId, ptr, m.name.c_str());
                }else{
                    drawMember(m.type->typeId, ptr, m.name.c_str(), m.minValue, m.maxValue, true);
                }
            }
            if(drawHeader){
                ImGui::TreePop();
            }
        }
    }

    void EditorGui::drawConstants(int typeId, void *data, const char *label) {
        auto *desc = env->reflection->getType(typeId);
        int &value = *(int*)data;
        const char *preview = "";
        for(auto &c : desc->constants) {
            if(c.value == value){
                preview = c.name.c_str();
            }
        }
        if(ImGui::BeginCombo(label, preview)){
            for(auto &c : desc->constants){
                if(ImGui::Selectable(c.name.c_str(), value == c.value)){
                    value = c.value;
                }
            }
            ImGui::EndCombo();
        }
    }

    TRI_STARTUP_CALLBACK("EditorGui"){
        editor->gui.setTypeFunction<float>([](const char *label, float &v, float *min, float *max){
            if(min != nullptr && max != nullptr){
                ImGui::SliderFloat(label, &v, *min, *max);
            }else{
                ImGui::DragFloat(label, &v, 0.01, min ? *min : 0.0f, max ? *max : 0.0f);
            }
        });
        editor->gui.setTypeFunction<double>([](const char *label, double &v, double *min, double *max){
            float f = v;
            if(min != nullptr && max != nullptr){
                ImGui::SliderFloat(label, &f, *min, *max);
            }else{
                ImGui::DragFloat(label, &f, 0.01, min ? *min : 0.0f, max ? *max : 0.0f);
            }
            v = f;
        });
        editor->gui.setTypeFunction<int>([](const char *label, int &v, int *min, int *max){
            if(min != nullptr && max != nullptr){
                ImGui::SliderInt(label, &v, *min, *max);
            }else{
                ImGui::DragInt(label, &v, 1.0f, min ? *min : 0.0f, max ? *max : 0.0f);
            }
        });
        editor->gui.setTypeFunction<bool>([](const char *label, bool &v, bool *min, bool *max){
            ImGui::Checkbox(label, &v);
        });

        editor->gui.setTypeFunction<glm::vec2>([](const char *label, glm::vec2 &v, glm::vec2 *min, glm::vec2 *max){
            ImGui::DragFloat2(label, (float*)&v, 0.01);
        });
        editor->gui.setTypeFunction<glm::vec3>([](const char *label, glm::vec3 &v, glm::vec3 *min, glm::vec3 *max){
            ImGui::DragFloat3(label, (float*)&v, 0.01);
        });
        editor->gui.setTypeFunction<glm::vec4>([](const char *label, glm::vec4 &v, glm::vec4 *min, glm::vec4 *max){
            ImGui::DragFloat4(label, (float*)&v, 0.01);
        });

        editor->gui.setTypeFunction<Color>([](const char *label, Color &v, Color *min, Color *max){
            glm::vec4 c = v.vec();
            ImGui::ColorEdit4(label, (float*)&c);
            v = Color(c);
        });
        editor->gui.setTypeFunction<Transform>([](const char *label, Transform &v, Transform *min, Transform *max){
            ImGui::DragFloat3("position", (float*)&v.position, 0.01);
            ImGui::DragFloat3("scale", (float*)&v.scale, 0.01);
            glm::vec3 rot = glm::degrees(v.rotation);
            ImGui::DragFloat3("rotation", (float*)&rot, 0.1);
            v.rotation = glm::radians(rot);
        });
    }

}
