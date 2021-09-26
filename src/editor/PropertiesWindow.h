//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once
#include "EditorWindow.h"
#include "entity/ComponentBuffer.h"

namespace tri {

    class PropertiesWindow : public EditorWindow {
    public:
        bool noContextMenu;
        bool noWindowScroll;

        void startup();
        void update() override;
        void updateHeader(EntityId id);
        void updateEntity(EntityId id);
        void updateComponent(int typeId, EntityId id);
        void updateComponentMenu(int typeId, EntityId id);
        void updateMenu(EntityId id);

    private:
        ComponentBuffer componentChangeBuffer;
        int lastFrameChangeTypeId;
        bool lastFrameAnyActiveItem;
        bool lastNoContextMenu;
        bool lastNoWindowScroll;

    };

}

