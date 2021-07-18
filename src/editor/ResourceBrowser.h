//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_RESOURCEBROWSER_H
#define TRIDOT_RESOURCEBROWSER_H

#include "tridot/engine/Engine.h"
#include "editor/EditorGui.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <imgui.h>

namespace tridot {

    class ResourceBrowser {
    public:
        void init();
        void update();
        template<typename T>
        void addFileAssociation(std::vector<std::string> extensions){
            for(auto &extension : extensions){
                Association association;
                association.dragDropSource = [](const std::string &name){
                    EditorGui::resourceDragDropSource<T>(name);
                };
                association.load = [](const std::string &name){
                    engine.resources.get<T>(name);
                };
                association.unload = [](const std::string &name){
                    engine.resources.remove(name);
                };
                fileAssociations[extension] = association;
            }
        }
    private:
        void updateDirectory(const std::string &directory, const std::string &baseDirectory);
        bool isSubdirectory(const std::string &directory);

        class Association{
        public:
            std::function<void(const std::string &)> dragDropSource;
            std::function<void(const std::string &)> load;
            std::function<void(const std::string &)> unload;
        };

        std::unordered_map<std::string, Association> fileAssociations;
    };

}

#endif //TRIDOT_RESOURCEBROWSER_H
