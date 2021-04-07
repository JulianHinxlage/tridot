//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/ecs/Reflection.h"
#include "tridot/engine/Physics.h"
#include "tridot/components/RenderComponent.h"
#include "tridot/components/Transform.h"
#include "tridot/render/Light.h"
#include "tridot/render/Camera.h"
#include "tridot/engine/Engine.h"
#include "Editor.h"
#include <imgui.h>

using namespace tridot;
using namespace ecs;

void componentGui(void *comp, ecs::Reflection::Type *type);

TRI_UPDATE("panels"){
    if(ImGui::GetCurrentContext() != nullptr) {
        bool &open = editor.getFlag("Properties");
        if(open) {
            if (ImGui::Begin("Properties", &open)) {
                EntityId id = editor.selectedEntity;
                if (id != -1) {

                    auto &types = ecs::Reflection::getTypes();
                    if (ImGui::Button("add Component")) {
                        ImGui::OpenPopup("add");
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("remove Entity")) {
                        engine.destroy(id);
                        editor.selectedEntity = -1;
                    }
                    if (ImGui::BeginPopup("add")) {
                        for (auto &type : types) {
                            auto *pool = engine.getPool(type->id());
                            if (pool && !pool->has(id)) {
                                if (ImGui::Button(type->name().c_str())) {
                                    pool->add(id, nullptr);
                                    ImGui::CloseCurrentPopup();
                                }
                            }
                        }
                        ImGui::EndPopup();
                    }

                    for (auto &type : types) {
                        auto *pool = engine.getPool(type->id());
                        if (pool && pool->has(id)) {
                            void *comp = pool->getById(id);
                            bool open = true;
                            if (ImGui::CollapsingHeader(type->name().c_str(), &open)) {
                                componentGui(comp, type);
                            }
                            if (!open) {
                                pool->remove(id);
                            }
                        }
                    }
                }
            }
            ImGui::End();
        }
    }
}

template<typename T>
void resourceSelectGui(Ref<T> &res, const Reflection::Member &member){
    std::string resName = "none";
    for (auto &name : engine.resources.getList<T>()){
        if(engine.resources.get<T>(name) == res){
            resName = name;
            break;
        }
    }
    if(ImGui::Button(resName.c_str())){
        ImGui::OpenPopup("res");
    }

    ImGui::SameLine();
    ImGui::Text(member.name.c_str());
    if(ImGui::BeginPopup("res")){
        if(ImGui::Button("none")){
            res = nullptr;
            ImGui::CloseCurrentPopup();
        }
        for (auto &name : engine.resources.getList<T>()){
            if(ImGui::Button(name.c_str())){
                res = engine.resources.get<T>(name);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}

void componentGui(void *comp, ecs::Reflection::Type *type){
    if(type->id() == ecs::Reflection::id<Light>()) {
        Light &light = *(Light*)comp;
        std::vector<const char *> list = {"Ambient", "Directional", "Point Light"};
        ImGui::Combo("type", (int *) &light.type, list.data(), list.size());
        ImGui::DragFloat3("position", (float *) &light.position, 0.01);
        ImGui::ColorEdit3("color", (float *) &light.color);
        ImGui::DragFloat("intensity", &light.intensity, 0.01, 0.0, 1000);
    }else {
        for (auto &member : type->member()) {
            void *ptr = (char *) comp + member.offset;
            ImGui::PushID((int) (size_t) ptr);
            if (member.typeId == ecs::Reflection::id<glm::vec3>()) {
                ImGui::DragFloat3(member.name.c_str(), (float *) ptr, 0.1f);
            } else if (member.typeId == ecs::Reflection::id<glm::vec2>()) {
                ImGui::DragFloat2(member.name.c_str(), (float *) ptr, 0.1f);
            } else if (member.typeId == ecs::Reflection::id<float>()) {
                ImGui::DragFloat(member.name.c_str(), (float *) ptr, 0.01f);
            } else if (member.typeId == ecs::Reflection::id<int>()) {
                ImGui::DragInt(member.name.c_str(), (int *) ptr);
            } else if (member.typeId == ecs::Reflection::id<bool>()) {
                ImGui::Checkbox(member.name.c_str(), (bool *) ptr);
            } else if (member.typeId == ecs::Reflection::id<Color>()) {
                Color &color = *(Color *) ptr;
                glm::vec4 c = color.vec();
                ImGui::ColorEdit3(member.name.c_str(), (float *) &c);
                color = Color(c);
            } else if (member.typeId == ecs::Reflection::id<Collider::Type>()) {
                Collider::Type &type = *(Collider::Type *) ptr;
                std::vector<const char *> list = {"Box", "Sphere"};
                ImGui::Combo(member.name.c_str(), (int *) &type, list.data(), list.size());
            } else if (member.typeId == ecs::Reflection::id<Material::Mapping>()) {
                Material::Mapping &mapping = *(Material::Mapping *) ptr;
                std::vector<const char *> list = {"uv", "tri planar", "scaled tri planar"};
                ImGui::Combo(member.name.c_str(), (int *) &mapping, list.data(), list.size());
            }  else if (member.typeId == ecs::Reflection::id<Ref<Material>>()) {
                Ref<Material> &material = *(Ref<Material>*)ptr;
                resourceSelectGui(material, member);
            } else if (member.typeId == ecs::Reflection::id<Ref<Mesh>>()) {
                Ref<Mesh> &mesh = *(Ref<Mesh>*)ptr;
                resourceSelectGui(mesh, member);
            } else {
                componentGui(ptr, ecs::Reflection::getTypes()[member.typeId]);
            }
            ImGui::PopID();
        }
        if(type->id() == ecs::Reflection::id<RigidBody>()){
            if(ImGui::Button("update RigidBody")){
                RigidBody &rb = *(RigidBody*)comp;
                engine.physics.remove(rb);
                rb.physicsReference = nullptr;
            }
        }
    }
}

REFLECT_TYPE(float)
REFLECT_TYPE(bool)
REFLECT_TYPE(int)

REFLECT_TYPE_NAME(glm::vec2, vec2)
REFLECT_MEMBER2(glm::vec2, x, y)

REFLECT_TYPE_NAME(glm::vec3, vec3)
REFLECT_MEMBER3(glm::vec3, x, y, z)

REFLECT_TYPE_NAME(glm::vec4, vec4)
REFLECT_MEMBER4(glm::vec4, x, y, z, w)

REFLECT_TYPE(Transform)
REFLECT_MEMBER3(Transform, position, scale, rotation)

REFLECT_TYPE(Collider)
REFLECT_MEMBER2(Collider, scale, type)

REFLECT_TYPE(RigidBody)
REFLECT_MEMBER8(RigidBody, velocity, angular, mass, friction, restitution, linearDamping, angularDamping, enablePhysics)

REFLECT_TYPE(RenderComponent)
REFLECT_MEMBER3(RenderComponent, mesh, material, color)

REFLECT_TYPE(Light)
REFLECT_MEMBER4(Light, position, color, intensity, type)

REFLECT_TYPE(PerspectiveCamera)
REFLECT_MEMBER9(PerspectiveCamera, position, forward, up, right, fieldOfView, aspectRatio, near, far, target)

REFLECT_TYPE(OrthographicCamera)
REFLECT_MEMBER6(OrthographicCamera, position, scale, up, right, rotation, aspectRatio)
