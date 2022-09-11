//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "EntityEvent.h"
#include "core/Environment.h"
#include "entity/World.h"

namespace tri {

	TRI_CLASS(EntityEvent::Listener);
	TRI_PROPERTIES2(EntityEvent::Listener, entityId, function);

	TRI_CLASS(EntityEvent);
	TRI_PROPERTIES1(EntityEvent, listeners);
	TRI_FUNCTION(EntityEvent, invoke);

	void EntityEvent::invoke() {
		for (auto& listener : listeners) {
			void* comp = env->world->getComponent(listener.entityId, listener.function.classId);
			if (comp) {
				auto *desc = listener.function.getFunctionDescriptor();
				if (desc) {
					desc->invoke(comp);
				}
			}
		}
	}

	void EntityEvent::addListener(EntityId id, int classId, int functionIndex) {
		Listener listener;
		listener.entityId = id;
		listener.function.classId = classId;
		listener.function.functionIndex = functionIndex;
		listeners.push_back(listener);
	}

}

