//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "TypeGui.h"
#include "core/core.h"
#include "Editor.h"
#include "render/Color.h"
#include "engine/Transform.h"
#include "engine/AssetManager.h"
#include "render/Material.h"
#include "render/Mesh.h"
#include "EditorCamera.h"
#include "render/Light.h"
#include "render/ShaderState.h"
#include "engine/ComponentCache.h"
#include <glm/glm.hpp>
#include <imgui.h>

namespace tri {

    void TypeGui::drawType(int typeId, void *data, bool topLevelLabel) {
        auto *desc = env->reflection->getType(typeId);
        drawMember(typeId, data, topLevelLabel ? desc->name.c_str() : " ", nullptr, nullptr, false);
    }

    bool TypeGui::anyTypeChange(int typeId, void *v1, void *v2){
        auto *desc = env->reflection->getType(typeId);
        if(desc->hasEquals()){
            return !desc->equals(v1, v2);
        }
        for(auto &m : desc->member){
            if(anyTypeChange(m.type->typeId, (uint8_t*)v1 + m.offset, (uint8_t*)v2 + m.offset)){
                return true;
            }
        }
        return false;
    }

    void TypeGui::setTypeFunction(int typeId, const std::function<void(const char *, void *, void *, void *)> &func) {
        if(typeFunctions.size() <= typeId){
            typeFunctions.resize(typeId + 1);
        }
        typeFunctions[typeId] = func;
    }

    void TypeGui::unsetTypeFunction(int typeId) {
        if (typeFunctions.size() > typeId) {
            typeFunctions[typeId] = nullptr;
        }
    }

    void TypeGui::drawMember(int typeId, void *data, const char *label, void *min, void *max, bool drawHeader, bool useFunction) {
        if(useFunction && typeFunctions.size() > typeId){
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
                if (!(m.flags & Reflection::HIDDEN_IN_EDITOR)) {
                    void *ptr = (uint8_t*)data + m.offset;
                    if(m.type->constants.size() > 0){
                        drawConstants(m.type->typeId, ptr, m.name.c_str());
                    }else{
                        drawMember(m.type->typeId, ptr, m.name.c_str(), m.minValue, m.maxValue, true);
                    }
                }
            }
            if(drawHeader){
                ImGui::TreePop();
            }
        }
    }

    void TypeGui::drawConstants(int typeId, void *data, const char *label) {
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

    template<typename T>
    void addAssetTypeFunction(AssetManager::Options options = AssetManager::NONE){
        env->editor->gui.typeGui.setTypeFunction<Ref<T>>([options](const char *label, Ref<T> &v, Ref<T> *min, Ref<T> *max){
            bool hasFile = false;
            std::string name = "<none>";
            if(v){
                name = env->assets->getFile(v);
                if(name == ""){
                    name = "<unknown>";
                }else{
                    hasFile = true;
                }
            }

            int typeId = env->reflection->getTypeId<T>();
            if(ImGui::BeginCombo(label, name.c_str())){
                if(ImGui::Selectable("<none>", name == "<none>")){
                    v = nullptr;
                }
                for(auto &file : env->assets->getAssetList(typeId)){
                    if(ImGui::Selectable(file.c_str(), name == file)){
                        v = env->assets->get<T>(file, options);
                    }
                }
                if(ImGui::Selectable("...", false)){
                    env->editor->gui.fileGui.openBrowseWindow("Open", std::string("Open ") + env->reflection->getType<T>()->name, typeId, [&](const std::string &file){
                        v = env->assets->get<T>(file, options);
                    });
                }
                ImGui::EndCombo();
            }

            //drag/drop
            std::string file = env->editor->gui.dragDropTarget(typeId);
            if(file != ""){
                v = env->assets->get<T>(file, options);
            }
            if(hasFile){
                env->editor->gui.dragDropSource(typeId, name);
            }
        });
    }

    void addVectorTypeFunction(int typeId) {
        auto* desc = env->reflection->getType(typeId);
        if (desc->baseType) {
            env->editor->gui.typeGui.setTypeFunction(typeId, [typeId](const char* label, void* v, void* min, void* max) {
                auto* desc = env->reflection->getType(typeId);
                if (desc && desc->baseType) {
                    if (desc->flags & Reflection::VECTOR) {
                        if (ImGui::TreeNodeEx(label)) {

                            int size = desc->vectorSize(v);
                            for (int i = 0; i < size; i++) {

                                int indentStartValue = ImGui::GetCursorPosX();


                                ImGui::PushID(i);
                                if (ImGui::Button("-")) {
                                    desc->vectorErase(v, i);
                                    size--;
                                    if (i >= size) {
                                        ImGui::PopID();
                                        break;
                                    }
                                }

                                ImGui::SameLine();
                                if (ImGui::Button("+")) {
                                    desc->vectorInsert(v, i, nullptr);
                                }

                                ImGui::SameLine();
                                std::string indexStr = std::to_string(i);

                                int indentValue = ImGui::GetCursorPosX() - indentStartValue;
                                ImGui::Indent(indentValue);

                                void* data = desc->vectorGet(v, i);
                                if (env->reflection->getType(desc->baseType->typeId)) {
                                    env->editor->gui.typeGui.drawMember(desc->baseType->typeId, data, std::to_string(i).c_str(), nullptr, nullptr, false);
                                }

                                ImGui::Unindent(indentValue);

                                ImGui::PopID();
                            }

                            if (ImGui::Button("+")) {
                                desc->vectorInsert(v, size, nullptr);
                            }

                            ImGui::SameLine();
                            if (ImGui::Button("+5")) {
                                for (int i = 0; i < 5; i++) {
                                    desc->vectorInsert(v, size, nullptr);
                                }
                            }

                            ImGui::SameLine();
                            if (ImGui::Button("clear")) {
                                desc->vectorClear(v);
                            }

                            ImGui::TreePop();
                        }


                    }
                }
            });
        }
    }

    TRI_STARTUP_CALLBACK("TypeGui"){
        env->editor->gui.typeGui.setTypeFunction<float>([](const char *label, float &v, float *min, float *max){
            if(min != nullptr && max != nullptr){
                ImGui::SliderFloat(label, &v, *min, *max);
            }else{
                ImGui::DragFloat(label, &v, 0.01, min ? *min : 0.0f, max ? *max : 0.0f);
            }
        });
        env->editor->gui.typeGui.setTypeFunction<double>([](const char *label, double &v, double *min, double *max){
            float f = v;
            if(min != nullptr && max != nullptr){
                ImGui::SliderFloat(label, &f, *min, *max);
            }else{
                ImGui::DragFloat(label, &f, 0.01, min ? *min : 0.0f, max ? *max : 0.0f);
            }
            v = f;
        });
        env->editor->gui.typeGui.setTypeFunction<int>([](const char *label, int &v, int *min, int *max){
            if(min != nullptr && max != nullptr){
                ImGui::SliderInt(label, &v, *min, *max);
            }else{
                ImGui::DragInt(label, &v, 1.0f, min ? *min : 0.0f, max ? *max : 0.0f);
            }
        });
        env->editor->gui.typeGui.setTypeFunction<bool>([](const char *label, bool &v, bool *min, bool *max){
            ImGui::Checkbox(label, &v);
        });
        env->editor->gui.typeGui.setTypeFunction<EntityId>([](const char* label, EntityId& v, EntityId* min, EntityId* max) {
            int i = v;
            ImGui::InputInt(label, &i, 0);
            v = i;
        });

        env->editor->gui.typeGui.setTypeFunction<std::string>([](const char *label, std::string &v, std::string *min, std::string *max){
            env->editor->gui.textInput(label, v);
        });

        env->editor->gui.typeGui.setTypeFunction<glm::vec2>([](const char *label, glm::vec2 &v, glm::vec2 *min, glm::vec2 *max){
            ImGui::DragFloat2(label, (float*)&v, 0.01);
        });
        env->editor->gui.typeGui.setTypeFunction<glm::vec3>([](const char *label, glm::vec3 &v, glm::vec3 *min, glm::vec3 *max){
            ImGui::DragFloat3(label, (float*)&v, 0.01);
        });
        env->editor->gui.typeGui.setTypeFunction<glm::vec4>([](const char *label, glm::vec4 &v, glm::vec4 *min, glm::vec4 *max){
            ImGui::DragFloat4(label, (float*)&v, 0.01);
        });

        env->editor->gui.typeGui.setTypeFunction<Color>([](const char *label, Color &v, Color *min, Color *max){
            glm::vec4 c = v.vec();
            ImGui::ColorEdit4(label, (float*)&c);
            v = Color(c);
        });
        env->editor->gui.typeGui.setTypeFunction<Transform>([](const char *label, Transform &v, Transform *min, Transform *max){
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
        addAssetTypeFunction<Prefab>();
        addAssetTypeFunction<Scene>(AssetManager::DO_NOT_LOAD);

        for (auto &desc : env->reflection->getDescriptors()) {
            if (desc->flags & Reflection::VECTOR) {
                addVectorTypeFunction(desc->typeId);
            }
        }
        env->signals->typeRegister.addCallback("TypeGui", [](int typeId) {
            for (auto& desc : env->reflection->getDescriptors()) {
                if (desc->flags & Reflection::VECTOR) {
                    addVectorTypeFunction(desc->typeId);
                }
            }
        });
        env->signals->typeUnregister.addCallback("TypeGui", [](int typeId) {
            env->editor->gui.typeGui.unsetTypeFunction(typeId);
        });

        env->editor->gui.typeGui.setTypeFunction<Ref<FrameBuffer>>([](const char *label, Ref<FrameBuffer> &v, Ref<FrameBuffer> *min, Ref<FrameBuffer> *max) {
            if(v){
                if(ImGui::TreeNode("FrameBuffer")) {
                    ImGui::Text("size: %i, %i", (int)v->getSize().x, (int)v->getSize().y);

                    std::vector<TextureAttachment> attachments;
                    for(int i = 0; i < 16; i++){
                        attachments.push_back(TextureAttachment(COLOR + i));
                    }
                    attachments.push_back(DEPTH);
                    attachments.push_back(STENCIL);

                    for (TextureAttachment attachment : attachments) {
                        auto texture = v->getAttachment(attachment);
                        if (texture) {
                            float aspect = 1;
                            if (texture->getHeight() != 0) {
                                aspect = (float) texture->getWidth() / (float) texture->getHeight();
                            }
                            ImGui::Image((void *) (size_t) texture->getId(), ImVec2(200 * aspect, 200), ImVec2(0, 1), ImVec2(1, 0));

                            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                                ImGui::SetDragDropPayload(env->reflection->getType<Texture>()->name.c_str(), &texture, sizeof(texture));
                                ImGui::Text("Texture");
                                ImGui::EndDragDropSource();
                            }

                            ImGui::SameLine();
                            std::string name;
                            if (auto *spec = v->getAttachmentSpec(attachment)) {
                                name = spec->name;
                            }
                            if (name.empty()) {
                                if (attachment == DEPTH) {
                                    name = "depth";
                                }
                                else if (attachment == STENCIL) {
                                    name = "stencil";
                                }
                                else if (attachment == COLOR) {
                                    name = "color";
                                }
                                else {
                                    name = "color " + std::to_string((int)attachment - COLOR);
                                }
                            }
                            ImGui::Text("%s", name.c_str());
                        }
                    }
                    ImGui::TreePop();
                }
            }
        });

        env->editor->gui.typeGui.setTypeFunction<Camera>([](const char *label, Camera &v, Camera *min, Camera *max) {
            env->editor->gui.typeGui.drawMember(env->reflection->getTypeId<Camera>(), &v, label, min, max, false, false);
            if(v.output) {
                if(ImGui::TreeNode("View")) {
                    v.active = true;
                    auto texture = v.output->getAttachment(COLOR);
                    if (texture) {
                        float aspect = 1;
                        if (texture->getHeight() != 0) {
                            aspect = (float) texture->getWidth() / (float) texture->getHeight();
                        }
                        ImGui::Image((void *) (size_t) texture->getId(), ImVec2(200 * aspect, 200), ImVec2(0, 1),
                                     ImVec2(1, 0));

                        EntityId id = env->scene->getEntityIdByComponent(v);
                        if(id != -1){
                            if(env->scene->hasComponent<Transform>(id)){
                                Transform &transform = env->scene->getComponent<Transform>(id);
                                if(ImGui::IsItemHovered()) {
                                    env->editor->properties.noContextMenu = true;
                                    env->editor->properties.noWindowScroll = true;
                                    env->editor->viewport.editorCamera.update(v, transform);
                                }
                            }
                        }
                    }
                    ImGui::TreePop();
                }
            }
        });

        env->editor->gui.typeGui.setTypeFunction<ComponentCache>([](const char* label, ComponentCache& v, ComponentCache* min, ComponentCache* max) {
            for (auto& i : v.data) {
                ImGui::Text("%s", i.first.c_str());
                ImGui::SameLine();
                if (ImGui::Button("Remove")) {
                    v.uncache(i.first);
                    if (v.data.size() == 0) {
                        env->scene->removeComponent<ComponentCache>(env->scene->getEntityIdByComponent(v));
                    }
                    break;
                }
            }
        });

        env->editor->gui.typeGui.setTypeFunction<Ref<ShaderState>>([](const char* label, Ref<ShaderState>& v, Ref<ShaderState>* min, Ref<ShaderState>* max) {
            if (ImGui::TreeNodeEx(label)) {
                if (v) {
                    for (auto& value : v->getValues()) {
                        if (value) {
                            env->editor->gui.typeGui.drawMember(value->typeId, value->getData(), value->name.c_str(), nullptr, nullptr, false);
                        }
                    }
                }
                ImGui::TreePop();
            }
         });

    }

}
