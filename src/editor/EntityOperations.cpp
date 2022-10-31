//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "EntityOperations.h"
#include "Editor.h"
#include "engine/Transform.h"
#include "engine/EntityUtil.h"

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

	void EntityOperations::duplicateSelection() {
		if (env->editor->selectionContext->isMultiSelection()) {
			std::map<EntityId, EntityId> idMap;

			env->editor->undo->beginAction();
			auto selected = env->editor->selectionContext->getSelected();
			env->editor->selectionContext->unselectAll();
			for (EntityId id : selected) {
				EntityId newId = env->editor->entityOperations->duplicateEntity(id);
				env->editor->selectionContext->select(newId, false);
				idMap[id] = newId;
			}
			env->editor->undo->endAction();

			EntityUtil::replaceIds(idMap, env->world);
		}
		else if (env->editor->selectionContext->isSingleSelection()) {
			EntityId id = env->editor->selectionContext->getSelected()[0];
			env->editor->selectionContext->select(env->editor->entityOperations->duplicateEntity(id));
		}
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

	void EntityOperations::parentEntity(EntityId id, EntityId parent, World* world) {
		if (!world) {
			world = env->world;
		}
		if (id == parent) {
			return;
		}
		Transform* t = env->world->getComponent<Transform>(id);
		env->editor->undo->componentChanged(Reflection::getClassId<Transform>(), id, t);
		if (t) {
			t->parent = parent;
			if (parent == -1) {
				t->decompose(t->getMatrix());
			}
			else {
				Transform* pt = env->world->getComponent<Transform>(parent);
				if (pt) {
					t->decompose(glm::inverse(pt->getMatrix()) * t->getMatrix());
				}
			}
		}
	}

	bool EntityOperations::hasCopiedEntity() {
		return hasEntityInBuffer;
	}

	int EntityOperations::getCopiedComponentClassId() {
		return componentBuffer.classId;
	}

}
