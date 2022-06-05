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
		if (classId != -1) {
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
	}

	void DynamicObjectBuffer::get(void* ptr) const {
		auto* desc = Reflection::getDescriptor(classId);
		desc->copy(data, ptr);
	}

	void* DynamicObjectBuffer::get() const {
		return data;
	}

	void DynamicObjectBuffer::clear() {
		if (data) {
			auto* desc = Reflection::getDescriptor(classId);
			if (desc) {
				desc->destruct(data, count);
			}
			else {
				env->console->warning("can't properly free memory for component");
			}
			delete data;
		}
		data = nullptr;
		count = 0;
		classId = -1;
	}

	EntityId Prefab::createEntity(World* world, EntityId hint) {
		if (!world) {
			world = env->world;
		}
		EntityId id = world->addEntity(hint);
		copyIntoEntity(id, world);
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

	void Prefab::copyIntoEntity(EntityId id, World* world) {
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

}