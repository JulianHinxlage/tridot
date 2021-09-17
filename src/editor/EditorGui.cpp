//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "EditorGui.h"
#include "core/core.h"
#include "Editor.h"
#include "render/Color.h"
#include "engine/Transform.h"
#include "engine/AssetManager.h"
#include "render/Material.h"
#include <glm/glm.hpp>
#include <imgui.h>

namespace tri {

    void EditorGui::drawType(int typeId, void *data) {
        auto *desc = env->reflection->getType(typeId);
        drawMember(typeId, data, desc->name.c_str(), nullptr, nullptr, false);
    }

    void EditorGui::dragDropSource(int typeId, const std::string &file) {
        if(ImGui::BeginDragDropSource()){
            ImGui::SetDragDropPayload(env->reflection->getType(typeId)->name.c_str(), file.data(), file.size());
            ImGui::Text("%s", file.c_str());
            ImGui::EndDragDropSource();
        }
    }

    std::string EditorGui::dragDropTarget(int typeId) {
        if(ImGui::BeginDragDropTarget()){
            if(ImGui::AcceptDragDropPayload(env->reflection->getType(typeId)->name.c_str())){
                auto *payload = ImGui::GetDragDropPayload();
                ImGui::EndDragDropTarget();
                return std::string((char*)payload->Data, payload->DataSize);
            }
            ImGui::EndDragDropTarget();
        }
        return "";
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
        if(desc->member.size() == 0){
            return;
        }
        if(!drawHeader || ImGui::TreeNodeEx(desc->name.c_str())) {
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

    void EditorGui::openBrowseFile(const std::string &name, const std::function<void(const std::string &)> &callback) {
        browseFileCallback = callback;
        browseFileOpenFlag = true;
        browseFileName = name;
    }

    void EditorGui::updateDirectory(const std::string &directory, const std::string &searchDirectory){
        for(auto &entry : std::filesystem::directory_iterator(directory)){
            if(entry.is_directory()){
                if(ImGui::TreeNode(entry.path().filename().c_str())){
                    updateDirectory(entry.path(), searchDirectory);
                    ImGui::TreePop();
                }
            }
        }for(auto &entry : std::filesystem::directory_iterator(directory)){
            if(entry.is_regular_file()){
                if(ImGui::Selectable(entry.path().filename().c_str(), browseFileFile == entry.path(), ImGuiSelectableFlags_DontClosePopups)){
                    browseFileFile = entry.path();
                }
            }
        }
    }

    void EditorGui::update() {
        if(browseFileOpenFlag){
            ImGui::OpenPopup("Browse File");
            browseFileFile = "";
            browseFileOpenFlag = false;
        }
        if(ImGui::BeginPopupModal("Browse File")){
            for(auto &dir : env->assets->getSearchDirectories()){
                std::string name = std::filesystem::path(dir).parent_path().filename();
                if(ImGui::TreeNode(name.c_str())){
                    updateDirectory(dir, dir);
                    ImGui::TreePop();
                }
            }
            if(ImGui::Button("Close")){
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if(ImGui::Button(browseFileName.c_str())){
                if(browseFileCallback){
                    browseFileCallback(browseFileFile);
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    template<typename T>
    void addAssetTypeFunction(){
        editor->gui.setTypeFunction<Ref<T>>([](const char *label, Ref<T> &v, Ref<T> *min, Ref<T> *max){
            std::string name = "<none>";
            if(v.get() != nullptr){
                name = env->assets->getFile(v);
            }
            if(ImGui::BeginCombo(label, name.c_str())){
                if(ImGui::Selectable("<none", name == "<none>")){
                    v = nullptr;
                }
                for(auto &file : env->assets->getAssetList(env->reflection->getTypeId<T>())){
                    if(ImGui::Selectable(file.c_str(), name == file)){
                        v = env->assets->get<T>(file);
                    }
                }
                ImGui::EndCombo();
            }
            std::string file = editor->gui.dragDropTarget(env->reflection->getTypeId<T>());
            if(file != ""){
                v = env->assets->get<T>(file);
            }
        });
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

        addAssetTypeFunction<Mesh>();
        addAssetTypeFunction<Material>();
        addAssetTypeFunction<Shader>();
        addAssetTypeFunction<Texture>();
    }

}
