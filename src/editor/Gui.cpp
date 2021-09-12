//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Gui.h"
#include "core/core.h"
#include "Editor.h"
#include "render/Color.h"
#include "engine/Transform.h"
#include <glm/glm.hpp>
#include <imgui.h>

namespace tri {

    void Gui::drawTypeGui(int typeId, void *data) {
        auto *desc = env->reflection->getType(typeId);
        drawMember(typeId, data, desc->name.c_str(), false);
    }

    void Gui::setTypeGui(int typeId, const std::function<void(const char *, void *)> &func) {
        if(typeGuiFuncs.size() <= typeId){
            typeGuiFuncs.resize(typeId + 1);
        }
        typeGuiFuncs[typeId] = func;
    }

    void Gui::drawMember(int typeId, void *data, const char *label, bool drawHeader) {
        if(typeGuiFuncs.size() > typeId){
            if(typeGuiFuncs[typeId]){
                typeGuiFuncs[typeId](label, data);
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
                    drawMember(m.type->typeId, ptr, m.name.c_str(), true);
                }
            }
            if(drawHeader){
                ImGui::TreePop();
            }
        }
    }

    void Gui::drawConstants(int typeId, void *data, const char *label) {
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

    TRI_STARTUP_CALLBACK("Gui"){
        editor->gui.setTypeGui<float>([](const char *label, float &v){
            ImGui::DragFloat(label, &v, 0.01);
        });
        editor->gui.setTypeGui<double>([](const char *label, double &v){
            float f = v;
            ImGui::DragFloat(label, &f, 0.01);
            v = f;
        });
        editor->gui.setTypeGui<int>([](const char *label, int &v){
            ImGui::DragInt(label, &v);
        });
        editor->gui.setTypeGui<bool>([](const char *label, bool &v){
            ImGui::Checkbox(label, &v);
        });

        editor->gui.setTypeGui<glm::vec2>([](const char *label, glm::vec2 &v){
            ImGui::DragFloat2(label, (float*)&v, 0.01);
        });
        editor->gui.setTypeGui<glm::vec3>([](const char *label, glm::vec3 &v){
            ImGui::DragFloat3(label, (float*)&v, 0.01);
        });
        editor->gui.setTypeGui<glm::vec4>([](const char *label, glm::vec4 &v){
            ImGui::DragFloat4(label, (float*)&v, 0.01);
        });

        editor->gui.setTypeGui<Color>([](const char *label, Color &v){
            glm::vec4 c = v.vec();
            ImGui::ColorEdit4(label, (float*)&c);
            v = Color(c);
        });
        editor->gui.setTypeGui<Transform>([](const char *label, Transform &v){
            ImGui::DragFloat3("position", (float*)&v.position, 0.01);
            ImGui::DragFloat3("scale", (float*)&v.scale, 0.01);
            glm::vec3 rot = glm::degrees(v.rotation);
            ImGui::DragFloat3("rotation", (float*)&rot, 0.1);
            v.rotation = glm::radians(rot);
        });
    }

}
