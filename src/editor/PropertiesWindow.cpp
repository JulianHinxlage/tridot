//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Editor.h"
#include "entity/Scene.h"
#include <imgui/imgui.h>

namespace tri {

    class PropertiesWindow : public EditorWindow {
    public:
        void startup() {
            name = "Properties";
        }

        void update() override {
            if(editor->selectionContext.getSelected().size() == 1){
                for(EntityId id : editor->selectionContext.getSelected()){
                    updateEntity(id);
                    break;
                }
            }else if(editor->selectionContext.getSelected().size() > 1){
                //todo: multi selection properties
            }
        }

        void updateEntity(EntityId id){
            for(auto &desc : env->reflection->getDescriptors()){
                if(env->scene->hasComponent(desc->typeId, id)){
                    if(ImGui::CollapsingHeader(desc->name.c_str())){
                        editor->gui.drawTypeGui(desc->typeId, env->scene->getComponent(desc->typeId, id));
                    }
                }
            }
        }
    };

    TRI_STARTUP_CALLBACK("") {
        editor->addWindow(new PropertiesWindow);
    }

}
