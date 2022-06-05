//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "EntityOperations.h"
#include "Editor.h"

namespace tri {

	TRI_SYSTEM(EntityOperations);

	void EntityOperations::init() {
		env->editor->entityOperations = this;
		hasEntityInBuffer = false;
	}

	EntityId EntityOperations::duplicateEntity(EntityId id, World* world) {
		Prefab prefab;
		prefab.copyEntity(id, world);
		EntityId newId = prefab.createEntity(world);
		if (!world || world == env->world) {
			env->editor->undo->entityAdded(newId);
		}
		return newId;
	}

	void EntityOperations::copyEntity(EntityId id, World* world) {
		entityBuffer.copyEntity(id, world);
		hasEntityInBuffer = true;
	}

	EntityId EntityOperations::pastEntity(World* world) {
		EntityId id = entityBuffer.createEntity(world);
		if (!world || world == env->world) {
			env->editor->undo->entityAdded(id);
		}
		return id;
	}

	void EntityOperations::removeEntity(EntityId id, World* world) {
		if (!world) {
			world = env->world;
		}
		if (!world || world == env->world) {
			env->editor->undo->entityRemoved(id);
		}
		world->removeEntity(id);
	}

	void EntityOperations::copyComponent(EntityId id, int classId, World* world) {
		if (!world) {
			world = env->world;
		}
		componentBuffer.set(classId, world->getComponent(id, classId));
	}

	void EntityOperations::pastComponent(EntityId id, World* world) {
		if (!world) {
			world = env->world;
		}
		if (componentBuffer.classId != -1) {
			if (world->hasComponent(id, componentBuffer.classId)) {
				void* comp = world->getComponent(id, componentBuffer.classId);
				if (!world || world == env->world) {
					env->editor->undo->componentChanged(componentBuffer.classId, id, comp);
				}
				componentBuffer.get(comp);
			}
			else {
				void* comp = world->addComponent(id, componentBuffer.classId, componentBuffer.get());
				if (!world || world == env->world) {
					env->editor->undo->componentAdded(componentBuffer.classId, id);
				}
			}
		}
	}

	void EntityOperations::removeComponent(EntityId id, int classId, World* world) {
		if (!world) {
			world = env->world;
		}
		if (!world || world == env->world) {
			env->editor->undo->componentRemoved(classId, id);
		}
		world->removeComponent(id, classId);
	}

	bool EntityOperations::hasCopiedEntity() {
		return hasEntityInBuffer;
	}

	int EntityOperations::getCopiedComponentClassId() {
		return componentBuffer.classId;
	}

}
