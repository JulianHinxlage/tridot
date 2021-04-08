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

        template<typename T>
        static void drawType(T &t, const std::string &name = ""){
            drawType(ecs::Reflection::id<T>(), &t, name);
        }
        template<typename T>
        static void addType(bool replaceMember, std::function<void(T&, const std::string &)> func){
            addType(ecs::Reflection::id<T>(), replaceMember, [func](void *ptr, const std::string &name){func(*(T*)ptr, name);});
        }
        template<typename T, typename M>
        static void addMember(const std::string &name, std::function<void(M&, const std::string &)> func){
            addMember(ecs::Reflection::id<T>(), ecs::Reflection::id<M>(), name, [func](void *ptr, const std::string &name){func(*(M*)ptr, name);});
        }

        template<typename T>
        static void drawResourceSelection(Ref<T> &res, const std::string &name){
            std::string resName = engine.resources.getName<T>(res);
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
                for (auto &n : engine.resources.getNames<T>()){
                    if(ImGui::Selectable(n.c_str())){
                        res = engine.resources.get<T>(n);
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndCombo();
            }


            if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)){
                ImGui::SetDragDropPayload(ecs::Reflection::get<T>().name().c_str(), &res, sizeof(res));
                ImGui::Text("%s", resName.c_str());
                ImGui::EndDragDropSource();
            }
            if(ImGui::BeginDragDropTarget()){
                if(const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(ecs::Reflection::get<T>().name().c_str())){
                    res = *(Ref<T>*)payload->Data;
                }
                ImGui::EndDragDropTarget();
            }
        }
    };

}

#endif //TRIDOT_EDITORGUI_H
