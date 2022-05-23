//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once
#include "EditorElement.h"
#include "entity/ComponentBuffer.h"

namespace tri {

    class PropertiesWindow : public EditorElement {
    public:
        bool noContextMenu;
        bool noWindowScroll;

        void startup();
        void update() override;
        void updateHeader();
        void updateEntity(EntityId id);
        void updateMultipleEntities();
        void updateComponent(int typeId, EntityId id);
        void updateComponentMenu(int typeId, EntityId editId);
        void updateMenu();

    private:
        std::vector<std::pair<ComponentBuffer, EntityId>> componentChangeBuffers;
        ComponentBuffer componentChangeBuffer;
        int lastFrameChangeTypeId;
        bool lastFrameAnyActiveItem;
        bool lastNoContextMenu;
        bool lastNoWindowScroll;

        void propagateComponentChange(int rootTypeId, int typeId, void *preEdit, void *postEdit, int offset = 0);

    };

}

