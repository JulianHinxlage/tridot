//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/util/Ref.h"
#include "entity/Prefab.h"

namespace tri {

    class UndoSystem : public System {
    public:
        void init() override;
        void startup() override;
        void shutdown() override;

        void undo();
        void redo();
        void clear();

        void beginAction();
        void endAction();

        void entityRemoved(EntityId id);
        void entityAdded(EntityId id);
        void componentChanged(int classId, EntityId id, void *preEditValue);
        void componentRemoved(int classId, EntityId id);
        void componentAdded(int classId, EntityId id);
        void valueChanged(int classId, void *instance, void *preEditValue);

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
            int classId = -1;
            DynamicObjectBuffer comp;
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

        std::vector<Ref<Action>> undoActionsPlayBuffer;
        std::vector<Ref<Action>> redoActionsPlayBuffer;

        void undoAction(Action &action);
    };

}

