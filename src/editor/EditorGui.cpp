//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "EditorGui.h"
#include "tridot/render/Camera.h"
#include "tridot/components/Tag.h"
#include "tridot/components/RenderComponent.h"
#include "tridot/components/ComponentCache.h"
#include "EditorCamera.h"
#include "Editor.h"
#include <imgui.h>

using namespace tridot;

class GuiData{
public:
    class Member{
    public:
        int parentTypeId;
        int typeId;
        std::string name;
        std::function<void(void*, const std::string &)> func;
    };

    class Type{
    public:
        int typeId;
        bool replaceMember;
        std::function<void(void*, const std::string &)> func;
    };

    std::vector<Member> member;
    std::vector<Type> types;
};
GuiData data;

namespace tridot {

    void EditorGui::drawType(int typeId, void *ptr, const std::string &name, int parentTypeId) {
        ImGui::PushID(ptr);
        bool noMember = false;
        for(auto &type : data.types){
            if(type.typeId == typeId){
                if(type.replaceMember){
                    noMember = true;
                    break;
                }
            }
        }

        if(!noMember){
            for(auto &member : env->reflection->getDescriptor(typeId)->member()){
                drawType(member.descriptor->id(), (char*)ptr + member.offset, member.name, typeId);
            }
        }

        bool handled = false;
        for(auto &member : data.member){
            if(member.typeId == typeId && parentTypeId == member.parentTypeId){
                if(member.name == name){
                    member.func(ptr, name);
                    handled = true;
                }
            }
        }
        if(!handled) {
            for (auto &type : data.types) {
                if (type.typeId == typeId) {
                    type.func(ptr, name);
                }
            }
        }
        ImGui::PopID();
    }

    void EditorGui::addType(int typeId, bool replaceMember, std::function<void(void *, const std::string &)> func) {
        data.types.push_back({typeId, replaceMember, func});
    }

    void EditorGui::addMember(int parentTypeId, int typeId, const std::string &name,
                              std::function<void(void *, const std::string &)> func) {
        data.member.push_back({parentTypeId, typeId, name, func});
    }

    void EditorGui::window(const std::string &name, const std::function<void()> &func) {
        if (ImGui::GetCurrentContext() != nullptr) {
            bool& open = Editor::getFlag(name);
            if (open) {
                if (ImGui::Begin(name.c_str(), &open, ImGuiWindowFlags_HorizontalScrollbar)) {
                    ImGui::SetWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
                    if(func){
                        func();
                    }
                }
                ImGui::End();
            }
        }
    }

}

TRI_INIT_CALLBACK("panels"){
    EditorGui::addType<float>(true, [](float &v, const std::string &name) {
        ImGui::SliderFloat(name.c_str(), &v, 0, 1);
    });
    EditorGui::addType<int>(true, [](int &v, const std::string &name) {
        ImGui::DragInt(name.c_str(), &v, 0.1);
    });
    EditorGui::addType<bool>(true, [](bool &v, const std::string &name) {
        ImGui::Checkbox(name.c_str(), &v);
    });
    EditorGui::addType<std::string>(true, [](std::string &v, const std::string &name) {
        char buffer[256];
        strcpy(buffer, v.c_str());
        ImGui::InputText(name.c_str(), buffer, sizeof(buffer));
        v = buffer;
    });
    EditorGui::addType<glm::vec2>(true, [](glm::vec2 &v, const std::string &name) {
        ImGui::DragFloat2(name.c_str(), (float*)&v, 0.01);
    });
    EditorGui::addType<glm::vec3>(true, [](glm::vec3 &v, const std::string &name) {
        ImGui::DragFloat3(name.c_str(), (float*)&v, 0.01);
    });
    EditorGui::addType<glm::vec4>(true, [](glm::vec4 &v, const std::string &name) {
        ImGui::DragFloat4(name.c_str(), (float*)&v, 0.01);
    });
    EditorGui::addType<Color>(true, [](Color &v, const std::string &name) {
        glm::vec4 c = v.vec();
        ImGui::ColorEdit4(name.c_str(), (float *) &c);
        v = c;
    });
    EditorGui::addType<Ref<Texture>>(true, [](Ref<Texture> &v, const std::string &name) {
        EditorGui::drawResourceSelection(v, name);
    });
    EditorGui::addType<Ref<Material>>(true, [](Ref<Material> &v, const std::string &name) {
        EditorGui::drawResourceSelection(v, name);
    });
    EditorGui::addType<Ref<Mesh>>(true, [](Ref<Mesh> &v, const std::string &name) {
        EditorGui::drawResourceSelection(v, name);
    });
    EditorGui::addType<Ref<Shader>>(true, [](Ref<Shader> &v, const std::string &name) {
        EditorGui::drawResourceSelection(v, name);
    });
#ifndef WIN32
    EditorGui::addType<uuid>(true, [](uuid &v, const std::string &name) {
        ImGui::LabelText(name.c_str(), "%s", v.str().c_str());
    });
#endif
    EditorGui::addType<Light>(true, [](Light &v, const std::string &name){
        std::vector<const char *> list = {"Ambient", "Directional", "Point Light"};
        ImGui::Combo("type", (int *) &v.type, list.data(), list.size());
        ImGui::ColorEdit3("color", (float *) &v.color);
        ImGui::DragFloat("intensity", &v.intensity, 0.01, 0.0, 1000);
    });
    EditorGui::addType<Collider::Type>(true, [](Collider::Type &v, const std::string &name){
        std::vector<const char *> list = {"Box", "Sphere"};
        ImGui::Combo(name.c_str(), (int *) &v, list.data(), list.size());
    });
    EditorGui::addType<Material::Mapping>(true, [](Material::Mapping &v, const std::string &name){
        std::vector<const char *> list = {"uv", "tri planar", "scaled tri planar"};
        ImGui::Combo(name.c_str(), (int *) &v, list.data(), list.size());
    });
    EditorGui::addType<Transform>(true, [](Transform &v, const std::string &name){
        ImGui::DragFloat3("position", (float*)&v.position, 0.01);
        ImGui::DragFloat3("scale", (float*)&v.scale, 0.01);

        glm::vec3 r = glm::degrees(v.rotation);
        if (ImGui::DragFloat3("rotation", (float*)&r, 1.0f)) {
            v.rotation = glm::radians(r);
        }
    });

    EditorGui::addType<Texture>(true, [](Texture &v, const std::string &name){
        ImGui::Text("id: %i", v.getId());
        ImGui::Text("width: %i", v.getWidth());
        ImGui::Text("height: %i", v.getHeight());
        ImGui::Text("channels: %i", v.getChannels());

        if(ImGui::Button("repeat")){
            v.setWrap(true, true);
        }
        ImGui::SameLine();
        if(ImGui::Button("clamp")){
            v.setWrap(false, false);
        }

        float aspect = 1;
        if (v.getHeight() != 0) {
            aspect = (float)v.getWidth() / (float)v.getHeight();
        }
        ImGui::Image((void *) (size_t) v.getId(), ImVec2(200 * aspect, 200));
    });
    EditorGui::addType<Mesh>(true, [](Mesh &v, const std::string &name){
        ImGui::Text("id: %i", v.vertexArray.getId());
    });
    EditorGui::addType<Shader>(true, [](Shader &v, const std::string &name){
        ImGui::Text("id: %i", v.getId());
    });

    EditorGui::addMember<RigidBody, float>("mass", [](float &v, const std::string &name){
        ImGui::DragFloat(name.c_str(), &v, 0.01, 0.0f, INFINITY);
    });
    EditorGui::addType<RigidBody>(false, [](RigidBody &rb, const std::string &name){
        if(ImGui::Button("update")){
            env->physics->remove(rb);
            rb.physicsReference = nullptr;
        }
    });

    EditorGui::addMember<PerspectiveCamera, float>("aspectRatio", [](float &v, const std::string &name){});
    EditorGui::addMember<PerspectiveCamera, float>("fieldOfView", [](float &v, const std::string &name){
        ImGui::DragFloat(name.c_str(), &v, 0.1, 0.0f, 170);
    });
    EditorGui::addMember<PerspectiveCamera, float>("far", [](float &v, const std::string &name){
        ImGui::DragFloat(name.c_str(), &v, 0.01, 0.0f, INFINITY);
    });
    EditorGui::addMember<PerspectiveCamera, float>("near", [](float &v, const std::string &name){
        ImGui::DragFloat(name.c_str(), &v, 0.01, 0.0f, INFINITY);
    });
    EditorGui::addMember<OrthographicCamera, float>("aspectRatio", [](float &v, const std::string &name){});
    EditorGui::addMember<OrthographicCamera, float>("rotation", [](float &v, const std::string &name){
        ImGui::DragFloat(name.c_str(), &v, 0.01);
    });

    EditorGui::addType<Ref<FrameBuffer>>(true, [](Ref<FrameBuffer> &v, const std::string &name) {
        if(v){
            if(ImGui::TreeNode("FrameBuffer")) {
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
                            ImGui::SetDragDropPayload(env->reflection->getDescriptor<Texture>()->name().c_str(), &texture, sizeof(texture));
                            ImGui::Text("Texture");
                            ImGui::EndDragDropSource();
                        }

                        ImGui::SameLine();
                        if(attachment == DEPTH){
                            ImGui::Text("depth");
                        }else if(attachment == STENCIL){
                            ImGui::Text("stencil");
                        }else if(attachment == COLOR){
                            ImGui::Text("color");
                        }else{
                            ImGui::Text("color %i", (int)attachment - COLOR);
                        }
                    }
                }
                ImGui::TreePop();
            }
        }
    });

    EditorGui::addType<PerspectiveCamera>(false, [](PerspectiveCamera &v, const std::string &name) {
       if(v.output) {
           if(ImGui::TreeNode("View")) {
               auto texture = v.output->getAttachment(COLOR);
               if (texture) {
                   float aspect = 1;
                   if (texture->getHeight() != 0) {
                       aspect = (float) texture->getWidth() / (float) texture->getHeight();
                   }
                   ImGui::Image((void *) (size_t) texture->getId(), ImVec2(200 * aspect, 200), ImVec2(0, 1),
                                ImVec2(1, 0));

                   EntityId id = env->scene->getIdByComponent(v);
                   if(id != -1){
                       if(env->scene->has<Transform>(id)){
                           Transform &transform = env->scene->get<Transform>(id);
                           static EditorCamera editorCamera;
                           if(ImGui::IsItemHovered()) {
                               Editor::propertiesWindowFlags |= ImGuiWindowFlags_NoScrollWithMouse;
                           }
                           editorCamera.update(v, transform, ImGui::IsItemHovered());
                       }
                   }
               }
               ImGui::TreePop();
           }
       }
    });

    EditorGui::addType<OrthographicCamera>(false, [](OrthographicCamera &v, const std::string &name) {
        if(v.output) {
            if(ImGui::TreeNode("View")) {
                auto texture = v.output->getAttachment(COLOR);
                if (texture) {
                    float aspect = 1;
                    if (texture->getHeight() != 0) {
                        aspect = (float) texture->getWidth() / (float) texture->getHeight();
                    }
                    ImGui::Image((void *) (size_t) texture->getId(), ImVec2(200 * aspect, 200), ImVec2(0, 1),
                                 ImVec2(1, 0));
                }
                ImGui::TreePop();
            }
        }
    });

    EditorGui::addType<ComponentCache>(true, [](ComponentCache &v, const std::string &name){
        for (auto comp : v.data) {
            if (ImGui::Button("remove")) {
                v.data.remove(comp.first);
            }
            ImGui::SameLine();
            ImGui::Text("%s", comp.first.as<std::string>("").c_str());
        }
    });
}
