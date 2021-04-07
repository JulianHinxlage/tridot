//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/engine/Engine.h"
#include "Editor.h"
#include <imgui.h>

using namespace tridot;
using namespace ecs;

void componentGui(void *comp, Reflection::Type *type);

template<typename T>
void resourceSelectGui(Ref<T> &res, const Reflection::Member &member);

TRI_UPDATE("panels"){
    if(ImGui::GetCurrentContext() != nullptr) {
        bool &open = editor.getFlag("Materials");
        if (open) {
            if (ImGui::Begin("Materials", &open)) {
                static Ref<Material> material = nullptr;

                Reflection::Member m;
                m.name = "material";
                m.typeId = Reflection::id<Material>();
                m.offset = 0;
                resourceSelectGui(material, m);

                if(material.get() != nullptr){
                    componentGui((void*)material.get(), &ecs::Reflection::get<Material>());
                }

            }
            ImGui::End();
        }
    }
}

REFLECT_TYPE(Material)
REFLECT_MEMBER10(Material,
        color,
        mapping,
        roughness,
        metallic,
        normalMapFactor,
        texture,
        normalMap,
        roughnessMap,
        metallicMap,
        textureOffset
)
REFLECT_MEMBER8(Material,
        textureScale,
        normalMapOffset,
        normalMapScale,
        roughnessMapOffset,
        roughnessMapScale,
        metallicMapOffset,
        metallicMapScale,
        shader
)
