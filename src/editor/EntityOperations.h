//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once
#include "entity/Scene.h"
#include "entity/Prefab.h"

namespace tri {

    class EntityOperations {
    public:
        EntityId addEntity(Scene *scene = nullptr);
        void removeEntity(EntityId id, Scene *scene = nullptr);
        void *addComponent(int typeId, EntityId id, Scene *scene = nullptr);
        void removeComponent(int typeId, EntityId id, Scene *scene = nullptr);

        EntityId duplicateEntity(EntityId id, Scene *scene = nullptr);
        void copyEntity(EntityId id, Scene *scene = nullptr);
        EntityId pastEntity(Scene *scene = nullptr);
        void copyComponent(int typeId, EntityId id, Scene *scene = nullptr);
        void pastComponent(EntityId id, Scene *scene = nullptr);

        bool wasEntityCopied();
        bool wasComponentCopied();

        void duplicateSelection(Scene *scene = nullptr);
        void removeSelection(Scene *scene = nullptr);

    private:
        ComponentBuffer componentBuffer;
        Prefab entityBuffer;
        bool isEntityBufferFilled = false;
    };

}

