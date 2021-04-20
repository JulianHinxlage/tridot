//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_UNDO_H
#define TRIDOT_UNDO_H

#include "tridot/ecs/config.h"
#include "tridot/ecs/Reflection.h"
#include <functional>

namespace tridot {

	class Undo {
	public:
		void addCustomAction(const std::function<void()> &undo, const std::function<void()> &redo = nullptr);

		void beginAction();
        void destroyEntity(ecs::EntityId id);
        void duplicateEntity(ecs::EntityId id, ecs::EntityId newId);
        void changeComponent(ecs::EntityId id, ecs::Reflection::Type *type, void *value);
        void endAction();

		void undoAction();
		void redoAction();
		void clearActions();

		class Action{
		public:
            std::vector<std::function<void()>> undos;
            std::vector<std::function<void()>> redos;
            class Component{
            public:
                std::shared_ptr<uint8_t[]> startData;
                std::shared_ptr<uint8_t[]> endData;
                ecs::EntityId id;
                int typeId;
            };
            std::vector<Component> components;
        };
        std::vector<Action> actions;
        int nextAction;
        Action actionBuffer;
        bool inAction;
        bool enabled;
	};

}

#endif //TRIDOT_UNDO_H
