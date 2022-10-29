//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Prefab.h"
#include "Serializer.h"
#include "Transform.h"

namespace tri {

	TRI_ASSET(Prefab);

	EntityId Prefab::createEntity(World* world, EntityId hint) {
		if (!world) {
			world = env->world;
		}
		EntityId id = world->addEntity(hint);
		copyIntoEntity(id, world, true);
		return id;
	}

	void Prefab::copyEntity(EntityId id, World* world, bool includeChilds) {
		if (!world) {
			world = env->world;
		}
		clear();
		for (auto* desc : Reflection::getDescriptors()) {
			if (desc && desc->flags & ClassDescriptor::COMPONENT) {
				void *comp = world->getComponent(id, desc->classId);
				if (comp) {
					addComponent(desc->classId, comp);
				}
			}
		}
		if (includeChilds && world == env->world) {
			for (auto &child : Transform::getChilds(id)) {
				addChild()->copyEntity(child, world, includeChilds);
			}
		}
	}

	void Prefab::copyIntoEntity(EntityId id, World* world, bool includeChilds) {
		if (!world) {
			world = env->world;
		}
		for (int i = 0; i < components.size(); i++) {
			int classId = components[i].classId;
			if (world->hasComponent(id, classId)) {
				components[i].get(world->getComponent(id, classId));
			}
			else {
				world->addComponent(id, classId, components[i].data);
			}
		}
		if (includeChilds) {
			for (auto& child : childs) {
				if (child) {
					EntityId childId = world->addEntity();
					child->copyIntoEntity(childId, world, includeChilds);
					if (Transform* t = world->getComponentPending<Transform>(childId)) {
						t->parent = id;
					}
				}
			}
		}
	}

	void* Prefab::addComponent(int classId, const void* ptr) {
		for (int i = 0; i < components.size(); i++) {
			if (components[i].classId == classId) {
				Reflection::getDescriptor(classId)->copy(ptr, components[i].data);
				return components[i].data;
			}
		}

		components.emplace_back();
		DynamicObjectBuffer &comp = components.back();
		comp.set(classId, ptr);
		return comp.data;
	}

	void* Prefab::getComponent(int classId) {
		for (int i = 0; i < components.size(); i++) {
			if (components[i].classId == classId) {
				return components[i].data;
			}
		}
		return nullptr;
	}

	void Prefab::removeComponent(int classId) {
		for (int i = 0; i < components.size(); i++) {
			if (components[i].classId == classId) {
				components[i].clear();
				components.erase(components.begin() + i);
				break;
			}
		}
	}

	Prefab* Prefab::addChild() {
		auto child = Ref<Prefab>::make();
		childs.push_back(child);
		return child.get();
	}

	void Prefab::clear() {
		components.clear();
		childs.clear();
	}

	const std::vector<DynamicObjectBuffer>& Prefab::getComponents() {
		return components;
	}

	const std::vector<Ref<Prefab>>& Prefab::getChilds() {
		return childs;
	}

	bool Prefab::load(const std::string& file) {
		SerialData data;
		if (env->serializer->loadFromFile(data, file)) {
			env->serializer->deserializePrefab(this, data);
			return true;
		}
		return false;
	}

	bool Prefab::save(const std::string& file) {
		SerialData data;
		env->serializer->saveToFile(data, file);
		env->serializer->serializePrefab(this, data);
		return true;
		return false;
	}

}