//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "ComponentStorage.h"
#include "core/config.h"

namespace tri {

	ComponentStorage::ComponentStorage(int classId)
		: classId(classId), componentSize(Reflection::getDescriptor(classId)->size) {
		pageCount = 0;
		mutex = std::make_shared<Mutex>();
#ifdef TRACY_ENABLE
		std::string name = Reflection::getDescriptor(classId)->name;
		mutex->mutex.CustomName(name.c_str(), name.size());
#endif
		componentData = nullptr;
		componentDataCapacity = 0;
		componentDataSize = 0;
	}

	ComponentStorage::~ComponentStorage() {
		clear();
	}

	void* ComponentStorage::getComponentByIdUnchecked(EntityId id) {
		return getComponentByIndex(getIndexByIdUnchecked(id));
	}

	void* ComponentStorage::getComponentById(EntityId id) {
		uint32_t index = getIndexById(id);
		if (index != -1) {
			return getComponentByIndex(index);
		}
		else {
			return nullptr;
		}
	}

	void* ComponentStorage::getComponentByIndex(uint32_t index) {
		TRI_ASSERT(index < idData.size(), "index out of bounds");
		return (void*)(componentData + (index * componentSize));
	}

	EntityId ComponentStorage::getIdByIndex(uint32_t index) {
		return idData[index];
	}

	int ComponentStorage::size() {
		return idData.size() - deactiveComponentCount;
	}

	int ComponentStorage::deactiveSize() {
		return deactiveComponentCount;
	}

	EntityId* ComponentStorage::getIdData() {
		return idData.data() + deactiveComponentCount;
	}

	void* ComponentStorage::getComponentData() {
		return componentData + deactiveComponentCount * componentSize;
	}

	void ComponentStorage::clear() {
		idData.clear();
		resizeData(0);
		for (int i = 0; i < indexByIdPages.size(); i++) {
			if (indexByIdPages[i]) {
				delete indexByIdPages[i];
			}
		}
		indexByIdPages.clear();
		indexByIdPageEntries.clear();
		pageCount = 0;

		for (auto& g : groups) {
			g->size = 0;
		}
	}

	uint32_t ComponentStorage::getIndexById(EntityId id) {
		uint32_t pageIndex = id >> pageSizeBits;
		uint32_t inPageIndex = id & ~(-1u << pageSizeBits);
		if (indexByIdPages.size() <= pageIndex) {
			return -1;
		}
		if (indexByIdPages[pageIndex] == nullptr) {
			return -1;
		}
		return indexByIdPages[pageIndex][inPageIndex];
	}

	uint32_t ComponentStorage::getIndexByIdUnchecked(EntityId id) {
		uint32_t pageIndex = id >> pageSizeBits;
		uint32_t inPageIndex = id & ~(-1u << pageSizeBits);
		TRI_ASSERT(pageIndex < indexByIdPages.size(), "index out of bounds");
		TRI_ASSERT(indexByIdPages[pageIndex], "index out of bounds");
		return indexByIdPages[pageIndex][inPageIndex];
	}

	bool ComponentStorage::hasComponent(EntityId id) {
		return getIndexById(id) != -1;
	}

	void ComponentStorage::swapIndex(uint32_t index1, uint32_t index2) {
		if (index1 == index2) {
			return;
		}

		EntityId id1 = getIdByIndex(index1);
		EntityId id2 = getIdByIndex(index2);

		auto* desc = Reflection::getDescriptor(classId);
		desc->swap(getComponentByIndex(index1), getComponentByIndex(index2));
		idData[index1] = id2;
		idData[index2] = id1;

		uint32_t pageIndex1 = id1 >> pageSizeBits;
		uint32_t inPageIndex1 = id1 & ~(-1u << pageSizeBits);
		indexByIdPages[pageIndex1][inPageIndex1] = index2;

		uint32_t pageIndex2 = id2 >> pageSizeBits;
		uint32_t inPageIndex2 = id2 & ~(-1u << pageSizeBits);
		indexByIdPages[pageIndex2][inPageIndex2] = index1;
	}

	void* ComponentStorage::addComponent(EntityId id, const void* ptr) {
		TRI_ASSERT(!hasComponent(id), "component already present");

		uint32_t index = idData.size();

		//component data
		if (componentDataCapacity < componentDataSize + 1) {
			if (componentDataCapacity == 0) {
				resizeData(1);
			}else{
				resizeData(componentDataCapacity * 2);
			}
		}
		componentDataSize++;

		uint32_t pageIndex = id >> pageSizeBits;
		uint32_t inPageIndex = id & ~(-1u << pageSizeBits);

		//index pages
		if (indexByIdPages.size() <= pageIndex) {
			indexByIdPages.resize(pageIndex + 1, nullptr);
			indexByIdPageEntries.resize(pageIndex + 1, 0);
		}
		if (indexByIdPages[pageIndex] == nullptr) {
			uint32_t* page = new uint32_t[1 << pageSizeBits];
			indexByIdPages[pageIndex] = page;
			indexByIdPageEntries[pageIndex] = 0;
			pageCount++;
			for (int i = 0; i < 1 << pageSizeBits; i++) {
				page[i] = -1;
			}
		}

		//set index page
		indexByIdPages[pageIndex][inPageIndex] = index;
		indexByIdPageEntries[pageIndex]++;

		//set id map
		if (idData.size() <= index) {
			idData.resize(index + 1);
		}
		idData[index] = id;

		//alignement of groups
		for (auto& group : groups) {
			if (group) {

				bool haveAll = true;
				for (auto* s : group->storages) {
					if (!s->hasComponent(id)) {
						haveAll = false;
						break;
					}
				}

				if (haveAll) {
					if (getIndexById(id) >= group->size) {
						for (auto* s : group->storages) {
							s->swapIndex(group->size, s->getIndexById(id));
						}
						group->size++;
					}
				}

			}
		}
		index = getIndexById(id);

		//construct
		void* comp = getComponentByIndex(index);
		auto *desc = Reflection::getDescriptor(classId);
		if (ptr) {
			desc->copy(ptr, comp);
		}
		else {
			desc->construct(comp);
		}
		return comp;
	}

	void ComponentStorage::removeComponent(EntityId id) {
		uint32_t index = getIndexById(id);
		uint32_t endIndex = idData.size() - 1;

		//alignement of groups
		for (int i = groups.size() - 1; i >= 0; i--) {
			auto& group = groups[i];
			if (group) {
				if (getIndexById(id) < group->size) {
					group->size--;
					for (auto* s : group->storages) {
						s->swapIndex(group->size, s->getIndexById(id));
					}
				}
			}
		}
		index = getIndexById(id);

		if (index < deactiveComponentCount) {
			swapIndex(index, --deactiveComponentCount);
			index = getIndexById(id);
		}

		swapIndex(index, endIndex);

		auto* desc = Reflection::getDescriptor(classId);
		desc->destruct(getComponentByIndex(endIndex));
		idData.pop_back();
		componentDataSize--;
		if (componentDataSize < componentDataCapacity / 2) {
			resizeData(componentDataCapacity / 2);
			idData.shrink_to_fit();
		}



		uint32_t pageIndex = id >> pageSizeBits;
		uint32_t inPageIndex = id & ~(-1u << pageSizeBits);
		indexByIdPages[pageIndex][inPageIndex] = -1;

		indexByIdPageEntries[pageIndex]--;
		if (indexByIdPageEntries[pageIndex] == 0) {
			delete indexByIdPages[pageIndex];
			indexByIdPages[pageIndex] = nullptr;
			pageCount--;

			int removeCount = 0;
			for (int i = indexByIdPages.size() - 1; i >= 0; i--) {
				if (!indexByIdPages[i]) {
					removeCount++;
				}
				else {
					break;
				}
			}
			if (removeCount > 0) {
				indexByIdPages.resize(indexByIdPages.size() - removeCount);
				if (indexByIdPages.size() < indexByIdPages.capacity() / 2) {
					indexByIdPages.shrink_to_fit();
				}
			}
		}
	}

	EntityId ComponentStorage::getIdByComponent(const void* comp) {
		int offset = (uint8_t*)comp - componentData;
		if (offset < 0) {
			return -1;
		}
		int index = offset / componentSize;
		if (index >= idData.size()) {
			return -1;
		}
		return getIdByIndex(index);
	}

	bool ComponentStorage::isComponentActive(EntityId id) {
		int index = getIndexById(id);
		if (index == -1) {
			return false;
		}
		return index >= deactiveComponentCount;
	}

	void ComponentStorage::setComponentActive(EntityId id, bool active) {
		int index = getIndexById(id);
		if (index == -1) {
			return;
		}

		if (active) {
			if (index < deactiveComponentCount) {
				swapIndex(index, --deactiveComponentCount);
			}
		}
		else {
			if (index >= deactiveComponentCount) {
				swapIndex(index, deactiveComponentCount++);
			}
		}
	}

	void ComponentStorage::addGroup(const std::shared_ptr<Group>& group) {
		if (checkGroup(group) == 0) {
			//insert group, groups should be in descending order
			bool inserted = false;
			for (int j = 0; j < groups.size(); j++) {
				auto& g = groups[j];
				if (g->storages.size() > group->storages.size()) {
					groups.insert(groups.begin() + j, group);
					inserted = true;
					break;
				}
			}
			if (!inserted) {
				groups.push_back(group);
			}
			initialGroupSorting(group.get());
		}
	}

	int ComponentStorage::checkGroup(const std::shared_ptr<Group>& group) {
		bool conflict = false;
		bool groupAlreadyPresent = false;
		for (auto& g : groups) {
			int count = 0;
			for (auto* s : g->storages) {
				for (auto* s2 : group->storages) {
					if (s == s2) {
						count++;
						break;
					}
				}
			}
			if (count != g->storages.size() && count != group->storages.size()) {
				conflict = true;
			}
			else if (count == g->storages.size() && count == group->storages.size()) {
				groupAlreadyPresent = true;
			}
		}
		if (conflict) {
			return 2;
		}
		if (groupAlreadyPresent) {
			return 1;
		}
		return 0;
	}

	void ComponentStorage::initialGroupSorting(Group* group) {
		uint32_t endIndex = idData.size();
		for (uint32_t index = 0; index < endIndex; index++) {
			EntityId id = getIdByIndex(index);

			bool haveAll = true;
			for (auto* s : group->storages) {
				if (!s->hasComponent(id)) {
					haveAll = false;
					break;
				}
			}

			if (haveAll) {
				if (getIndexById(id) >= group->size) {
					for (auto* s : group->storages) {
						s->swapIndex(group->size, s->getIndexById(id));
					}
					group->size++;
				}
			}
		}
	}

	int ComponentStorage::getGroupSize(const std::vector<int>& classIds) {
		for (auto& group : groups) {
			if (group->storages.size() == classIds.size()) {
				bool match = true;
				for (auto& s : group->storages) {

					bool hit = false;
					for (auto id : classIds) {
						if (id == s->classId) {
							hit = true;
							break;
						}
					}

					if (!hit) {
						match = false;
						break;
					}
				}
				if (match) {
					return group->size;
				}
			}
		}
		return -1;
	}

	void ComponentStorage::copy(ComponentStorage& from) {
		TRI_ASSERT(classId == from.classId, "component type must match when copying a storage");
		TRI_ASSERT(componentSize == from.componentSize, "component type must match when copying a storage");

		//delete current memory
		auto* desc = Reflection::getDescriptor(classId);
		if (desc) {
			desc->destruct(componentData, componentDataSize);
		}
		delete componentData;

		//allocate memory and copy data
		componentDataSize = from.componentDataSize;
		componentDataCapacity = from.componentDataCapacity;
		if (componentDataCapacity == 0) {
			componentData = nullptr;
		}
		else {
			componentData = new uint8_t[componentDataCapacity * componentSize];
		}
		if (desc) {
			desc->copy(from.componentData, componentData, componentDataSize);
		}

		//copy ids
		idData = from.idData;
		deactiveComponentCount = from.deactiveComponentCount;
		indexByIdPageEntries = from.indexByIdPageEntries;
		pageCount = from.pageCount;

		//delete pages
		for (int i = 0; i < indexByIdPages.size(); i++) {
			if (indexByIdPages[i]) {
				delete indexByIdPages[i];
				indexByIdPages[i] = nullptr;
			}
		}

		//copy pages
		indexByIdPages.resize(from.indexByIdPages.size());
		for (int i = 0; i < indexByIdPages.size(); i++) {
			if (from.indexByIdPages[i]) {
				indexByIdPages[i] = new uint32_t[1 << pageSizeBits];
				for (int j = 0; j < 1 << pageSizeBits; j++) {
					indexByIdPages[i][j] = from.indexByIdPages[i][j];
				}
			}
		}
	}

	void ComponentStorage::reserve(int count) {
		if (count > componentDataCapacity) {
			idData.reserve(count);
			resizeData(count);
		}
	}

	void ComponentStorage::resizeData(int count) {
		uint8_t *oldData = componentData;
		uint32_t newCapacity = count;
		
		if (count == 0) {
			componentData = nullptr;
		}
		else {
			componentData = new uint8_t[newCapacity * componentSize];
		}
		
		int size = std::min(componentDataSize, newCapacity);
		auto *desc = Reflection::getDescriptor(classId);
		if (desc) {
			desc->move(oldData, componentData, size);
			if (newCapacity < componentDataSize) {
				desc->destruct(oldData + newCapacity * componentSize, componentDataSize - newCapacity);
			}
		}

		componentDataCapacity = newCapacity;
		if (componentDataSize > componentDataCapacity) {
			componentDataSize = componentDataCapacity;
		}

		delete oldData;
	}

	int ComponentStorage::memoryUsage() {
		int componentMem = componentDataCapacity * componentSize;
		int idMem = idData.capacity() * sizeof(decltype(idData[0]));
		int pageMem = pageCount * (1 << pageSizeBits) * sizeof(decltype(indexByIdPages[0][0]));
		int pageMap = indexByIdPages.capacity() * sizeof(void*);
		return componentMem + idMem + pageMem + pageMap;
	}

	void ComponentStorage::lock() {
		size_t id = std::hash<std::thread::id>{}(std::this_thread::get_id());
		if (id != lockId) {
			mutex->mutex.lock();
			lockId = id;
		}
		else {
			lockCount++;
		}
	}

	void ComponentStorage::unlock() {
		size_t id = std::hash<std::thread::id>{}(std::this_thread::get_id());
		if (id == lockId && lockCount == 0) {
			lockId = 0;
			mutex->mutex.unlock();
		}
		else {
			lockCount--;
		}
	}

}
