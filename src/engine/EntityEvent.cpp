//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "EntityEvent.h"
#include "core/Environment.h"
#include "entity/World.h"

namespace tri {

	TRI_CLASS(EntityEvent::Listener);
	TRI_PROPERTIES3(EntityEvent::Listener, entityId, classId, func);

	TRI_COMPONENT(EntityEvent);
	TRI_PROPERTIES1(EntityEvent, listeners);
	TRI_FUNCTION(EntityEvent, invoke);

	void EntityEvent::invoke() {
		for (auto& listener : listeners) {
			void* comp = env->world->getComponent(listener.entityId, listener.classId);
			if (comp) {
				if (listener.func) {
					listener.func->invoke(comp);
				}
			}
		}
	}

	void EntityEvent::addListener(EntityId id, int classId, FunctionDescriptor* func) {
		Listener listener;
		listener.entityId = id;
		listener.classId = classId;
		listener.func = func;
		listeners.push_back(listener);
	}

}

