//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Prefab.h"

namespace tri {

	DynamicObjectBuffer::DynamicObjectBuffer() {
		data = nullptr;
		classId = -1;
		count = 0;
	}

	DynamicObjectBuffer::DynamicObjectBuffer(const DynamicObjectBuffer& buffer) {
		data = nullptr;
		classId = -1;
		count = 0;
		set(buffer.classId, buffer.data, buffer.count);
	}

	DynamicObjectBuffer::~DynamicObjectBuffer() {
		clear();
	}

	void DynamicObjectBuffer::set(int classId, const void* ptr, int count) {
		clear();
		this->classId = classId;
		this->count = count;
		auto* desc = Reflection::getDescriptor(classId);
		data = new uint8_t[count * desc->size];
		if (ptr) {
			desc->copy(ptr, data, count);
		}
		else {
			desc->construct(data, count);
		}
	}

	void DynamicObjectBuffer::clear() {
		if (data) {
			Reflection::getDescriptor(classId)->destruct(data, count);
			delete data;
		}
		data = nullptr;
		count = 0;
		classId = -1;
	}

	EntityId Prefab::createEntity(World* world) {
		if (!world) {
			world = env->world;
		}
		EntityId id = world->addEntity();
		for (int i = 0; i < components.size(); i++) {
			world->addComponent(id, components[i].classId, components[i].data);
		}
		return id;
	}

	void Prefab::copyEntity(EntityId id, World* world) {
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

}