//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once
#include "core/util/Ref.h"
#include "entity/ComponentBuffer.h"
#include "entity/Prefab.h"

namespace tri {

    /*
     undo tracking:
     gizmos
     component edit
     entity operations (x)
     material edits
     selections?
     */
    class UndoSystem {
    public:
        void undo();
        void redo();
        void clear();

        void beginAction();
        void endAction();

        void entityRemoved(EntityId id);
        void entityAdded(EntityId id);
        void componentChanged(int typeId, EntityId id, void *preEditValue);
        void componentRemoved(int typeId, EntityId id);
        void componentAdded(int typeId, EntityId id);
        void valueChanged(int typeId, void *instance, void *preEditValue);

    private:
        class Action{
        public:
            enum Operation{
                NOOP,
                COMP_CHANGED,
                COMP_REMOVED,
                COMP_ADDED,
                ENTITY_REMOVED,
                ENTITY_ADDED,
                VALUE_CHANGED,
            };
            Operation op = NOOP;
            EntityId id = -1;
            int typeId = -1;
            ComponentBuffer comp;
            Prefab entity;
            void *instance = nullptr;
            //if multiple actions count as one action
            bool isActionSequence = false;
        };
        std::vector<Ref<Action>> undoActions;
        std::vector<Ref<Action>> redoActions;
        int inActionCounter = 0;
        bool inActionWasAdded = false;
        bool addToRedo = false;
        bool clearRedoOnAction = true;

        void undoAction(Action &action);
    };

}

