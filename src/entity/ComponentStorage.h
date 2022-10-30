//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/config.h"
#include "core/Reflection.h"
#include <unordered_map>
#include <tracy/Tracy.hpp>

namespace tri {

	class ComponentStorage;

	class ComponentStorage {
	public:
		const int classId;
		const int componentSize;

		ComponentStorage(int classId);
		ComponentStorage(const ComponentStorage &) = delete;
		~ComponentStorage();

		void* getComponentById(EntityId id);
		void *getComponentByIdUnchecked(EntityId id);
		void *getComponentByIndex(uint32_t index);
		uint32_t getIndexById(EntityId id);
		uint32_t getIndexByIdUnchecked(EntityId id);
		EntityId getIdByIndex(uint32_t index);

		bool hasComponent(EntityId id);
		void* addComponent(EntityId id, const void *ptr = nullptr);
		void removeComponent(EntityId id);
		EntityId getIdByComponent(const void* comp);

		int size();
		EntityId* getIdData();
		void* getComponentData();
		void clear();
		void copy(ComponentStorage &from);
		void reserve(int count);
		int memoryUsage();

		void lock();
		void unlock();

		class Group {
		public:
			std::vector<ComponentStorage*> storages;
			int size;
		};
		void addGroup(const std::shared_ptr<Group>& group);

		//0 = fine, 1 = already present, 2 = conflicting
		int checkGroup(const std::shared_ptr<Group>& group);

		//size of a given group, returns -1 if no matching group exists
		template<typename... Components>
		int getGroupSize() {
			return getGroupSize({ Reflection::getClassId<Components>()... });
		}

		//size of a given group, returns -1 if no matching group exists
		int getGroupSize(const std::vector<int>& classIds);

	private:
		std::vector<EntityId> idData;

		uint8_t* componentData;
		//count of components not bytes
		uint32_t componentDataCapacity;
		//count of components not bytes
		uint32_t componentDataSize;

		static const int pageSizeBits = 10;
		std::vector<uint32_t*> indexByIdPages;

		//count of entries in a page with a valid value (value != -1)
		std::vector<uint32_t> indexByIdPageEntries;
		int pageCount = 0;


		std::vector<std::shared_ptr<Group>> groups;

		class Mutex {
		public:
			TracyLockable(std::mutex, mutex);
		};
		std::shared_ptr<Mutex> mutex;
		size_t lockId = 0;
		size_t lockCount = 0;

		void resizeData(int count);
		void swapIndex(uint32_t index1, uint32_t index2);
		void initialGroupSorting(Group* group);
	};

}
