//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "engine/AssetManager.h"
#include "render/Material.h"
#include <imgui/imgui.h>

namespace tri {

    class MaterialsWindow : public EditorWindow {
    public:
        Ref<Material> material;
        bool lastFrameAnyActiveItem;
        ComponentBuffer changeBuffer;
        bool inChange;

        void startup() {
            name = "Materials";
            material = nullptr;
            lastFrameAnyActiveItem = false;
            inChange = false;
        }

        void update() override{
            int typeId = env->reflection->getTypeId<Material>();
            auto files = env->assets->getAssetList(typeId);
            bool hasFile = false;

            std::string name = "<none>";
            if(material){
                name = env->assets->getFile(material);
                if(name == ""){
                    name = "<unknown>";
                }else{
                    hasFile = true;
                }
            }

            //selection combo
            if(ImGui::BeginCombo(" ", name.c_str())){
                if(ImGui::Selectable("<none>", name == "<none>")){
                    material = nullptr;
                }
                for(auto &file : files){
                    if(ImGui::Selectable(file.c_str(), file == name)){
                        material = env->assets->get<Material>(file);
                    }
                }
                if(ImGui::Selectable("...")){
                    env->editor->gui.file.openBrowseWindow("Open", "Open Material", typeId, [&](const std::string &file){
                        material = env->assets->get<Material>(file);
                    });
                }
                ImGui::EndCombo();
            }

            //drag/drop
            std::string file = env->editor->gui.dragDropTarget(typeId);
            if(!file.empty()){
                material = env->assets->get<Material>(file);
            }
            if(hasFile){
                env->editor->gui.dragDropSource(typeId, name);
            }

            //save button
            if(ImGui::Button("Save")){
                if((material.get() != nullptr) && hasFile){
                    material->save(env->assets->searchFile(name));
                }
            }
            ImGui::SameLine();
            if(ImGui::Button("Save All")){
                for(auto &file : files){
                    env->assets->get<Material>(file)->save(env->assets->searchFile(file));
                }
            }

            ImGui::Separator();
            ImGui::BeginChild("Materials Child");

            //material editing
            if(material){
                //detect changes
                ComponentBuffer preEditValue;
                preEditValue.set(typeId, material.get());

                //draw gui
                env->editor->gui.type.drawType(typeId, material.get(), false);

                //detect changes
                if (env->editor->gui.type.anyTypeChange(typeId, material.get(), preEditValue.get())) {
                    if(lastFrameAnyActiveItem || ImGui::IsAnyItemActive()) {
                        if(!inChange){
                            changeBuffer.set(typeId, preEditValue.get());
                            inChange = true;
                        }
                    }
                }else{
                    if(!lastFrameAnyActiveItem && !ImGui::IsAnyItemActive()) {
                        if(inChange){
                            env->editor->undo.valueChanged(typeId, material.get(), changeBuffer.get());
                            inChange = false;
                        }
                    }
                }
            }

            if(ImGui::IsAnyItemActive()){
                lastFrameAnyActiveItem = true;
            }else{
                lastFrameAnyActiveItem = false;
            }

            ImGui::EndChild();
        }

    };

    TRI_STARTUP_CALLBACK("") {
        env->editor->addWindow(new MaterialsWindow);
    }

}
