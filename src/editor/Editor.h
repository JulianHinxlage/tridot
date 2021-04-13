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
        static ecs::EntityId selectedEntity;
        static ecs::EntityId cameraId;
        static glm::vec2 viewportSize;
        static std::map<std::string, bool> flags;
        static std::string currentSceneFile;
        static uint64_t propertiesWindowFlags;

        static bool &getFlag(const std::string &name);
        static void loadFlags();
        static void saveFlags();
    };

}

#endif //TRIDOT_EDITOR_H
