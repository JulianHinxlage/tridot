//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "World.h"

namespace tri {

	class DynamicObjectBuffer {
	public:
		int classId;
		uint8_t* data;
		int count;

		DynamicObjectBuffer();
		DynamicObjectBuffer(const DynamicObjectBuffer &buffer);
		~DynamicObjectBuffer();
		void set(int classId, const void* ptr = nullptr, int count = 1);
		void clear();
	};

	class Prefab {
	public:
		EntityId createEntity(World* world = nullptr);
		void copyEntity(EntityId id, World* world = nullptr);

		template<typename T>
		T* addComponent(const T& t = T()) {
			return (T*)addComponent(Reflection::getClassId<T>(), &t);
		}
		template<typename T>
		T* getComponent() {
			return (T*)getComponent(Reflection::getClassId<T>());
		}
		template<typename T>
		void removeComponent() {
			removeComponent(Reflection::getClassId<T>());
		}

		void* addComponent(int classId, const void *ptr = nullptr);
		void* getComponent(int classId);
		void removeComponent(int classId);
		Prefab* addChild();
		void clear();

	private:
		std::vector<DynamicObjectBuffer> components;
		std::vector<Ref<Prefab>> childs;
	};

}
