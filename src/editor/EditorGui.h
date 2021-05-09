//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_EDITORGUI_H
#define TRIDOT_EDITORGUI_H

#include "tridot/engine/Engine.h"
#include <functional>
#include <string>
#include <imgui.h>

namespace tridot {

    class EditorGui {
    public:
        static void drawType(int reflectId, void *ptr, const std::string &name = "", int parentReflectId = -1);
        static void addType(int reflectId, bool replaceMember, std::function<void(void*, const std::string &name)> func);
        static void addMember(int reflectId, int memberReflectId, const std::string &name, std::function<void(void*, const std::string &name)> func);
        static void window(const std::string &name, const std::function<void()> &func);

        template<typename T>
        static void drawType(T &t, const std::string &name = ""){
            drawType(Reflection::id<T>(), &t, name);
        }
        template<typename T>
        static void addType(bool replaceMember, std::function<void(T&, const std::string &)> func){
            addType(Reflection::id<T>(), replaceMember, [func](void *ptr, const std::string &name){func(*(T*)ptr, name);});
        }
        template<typename T, typename M>
        static void addMember(const std::string &name, std::function<void(M&, const std::string &)> func){
            addMember(Reflection::id<T>(), Reflection::id<M>(), name, [func](void *ptr, const std::string &name){func(*(M*)ptr, name);});
        }

        template<typename T>
        static void resourceDragDropSource(const std::string &name, const Ref<T> &resource = nullptr){
            if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)){
                std::vector<uint8_t> buffer(sizeof(resource) + name.size());
                memcpy(buffer.data(), &resource, sizeof(resource));
                memcpy(buffer.data() + sizeof(resource), name.c_str(), name.size());
                ImGui::SetDragDropPayload(Reflection::get<T>()->name().c_str(), buffer.data(), buffer.size());
                ImGui::Text("%s", name.c_str());
                ImGui::EndDragDropSource();
            }
        }

        template<typename T>
        static void resourceDragDropTarget(Ref<T> &resource){
            if(ImGui::BeginDragDropTarget()){
                if(const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(Reflection::get<T>()->name().c_str())){
                    Ref<T> res;
                    uint8_t *buffer = (uint8_t*)payload->Data;
                    memcpy(&res, buffer, sizeof(res));
                    std::string name((char*)buffer + sizeof(res), payload->DataSize - sizeof(res));
                    if(res != nullptr){
                        resource = res;
                    }else{
                        resource = engine.resources.get<T>(name);
                    }
                }
                ImGui::EndDragDropTarget();
            }
        }

        template<typename T>
        static void drawResourceSelection(Ref<T> &res, const std::string &name){
            std::string resName = engine.resources.getName(res);
            if(resName.empty()){
                if(res){
                    resName = "<unknown>";
                }else{
                    resName = "<none>";
                }
            }

            if(ImGui::BeginCombo(name.c_str(), resName.c_str())){
                if(ImGui::Selectable("<none>")){
                    res = nullptr;
                    ImGui::CloseCurrentPopup();
                }
                for (auto &n : engine.resources.getNameList<T>(true)){
                    if(ImGui::Selectable(n.c_str())){
                        res = engine.resources.get<T>(n);
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndCombo();
            }

            resourceDragDropSource(resName, res);
            resourceDragDropTarget(res);
        }
    };

}

#endif //TRIDOT_EDITORGUI_H
