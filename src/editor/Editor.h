//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_EDITOR_H
#define TRIDOT_EDITOR_H

#include "tridot/ecs/Registry.h"
#include <glm/glm.hpp>
#include <map>

namespace tridot {

    class Editor {
    public:
        ecs::EntityId selectedEntity = -1;
        ecs::EntityId cameraId = -1;
        glm::vec2 viewportSize = {0, 0};

        std::map<std::string, bool> flags;
        bool &getFlag(const std::string &name);
        void loadFlags();
        void saveFlags();
    };

}

extern tridot::Editor editor;

#endif //TRIDOT_EDITOR_H
