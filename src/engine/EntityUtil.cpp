//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "EntityUtil.h"
#include "core/core.h"
#include "engine/RuntimeMode.h"
#include "engine/EntityInfo.h"

namespace tri {
	
	void eachProperty(int classId, void* comp, std::function<void(void* prop, int classId)> callback) {
		auto* desc = Reflection::getDescriptor(classId);
		if (desc) {
			if (desc->flags & ClassDescriptor::VECTOR) {
				int size = desc->vectorSize(comp);
				for (int i = 0; i < size; i++) {
					void* elm = desc->vectorGet(comp, i);
					callback((uint8_t*)elm, desc->elementType->classId);
					eachProperty(desc->elementType->classId, elm, callback);
				}
			}
			for (auto& prop : desc->properties) {
				callback((uint8_t*)comp + prop.offset, prop.type->classId);
				eachProperty(prop.type->classId, (uint8_t*)comp + prop.offset, callback);
			}
		}
	}

	void EntityUtil::replaceIds(const std::map<EntityId, EntityId>& idMap, World* world) {
		for (auto& i : idMap) {
			for (auto* desc : Reflection::getDescriptors()) {
				if (desc) {
					if (desc->flags & ClassDescriptor::COMPONENT) {
						void* comp = world->getComponentPending(i.second, desc->classId);
						if (comp) {
							eachProperty(desc->classId, comp, [&](void* prop, int classId) {
								if (classId == Reflection::getClassId<EntityId>()) {
									EntityId* id = (EntityId*)prop;
									auto j = idMap.find(*id);
									if (j != idMap.end()) {
										*id = j->second;
									}
								}
							});
						}
					}
				}
			}
		}
	}

	void EntityUtil::removeEntityWithChilds(EntityId id) {
		for (auto& child : Transform::getChilds(id)) {
			removeEntityWithChilds(child);
		}
		env->world->removeEntity(id);
	}

	void EntityUtil::eachChild(EntityId id, bool recursive, const std::function<void(EntityId id)>& callback) {
		for (auto& child : Transform::getChilds(id)) {
			callback(child);
			if (recursive) {
				eachChild(child, recursive, callback);
			}
		}
	}

	Camera* EntityUtil::getPrimaryCamera() {
		Camera* cam = nullptr;
		env->world->each<Camera>([&](Camera& c) {
			if (c.active && c.isPrimary) {
				cam = &c;
			}
		});
		return cam;
	}

	Guid EntityUtil::getGuid(EntityId id) {
		if (auto *info = env->world->getComponent<EntityInfo>(id)) {
			return info->guid;
		}
		return Guid();
	}

	const std::string& EntityUtil::getName(EntityId id) {
		if (auto* info = env->world->getComponent<EntityInfo>(id)) {
			return info->name;
		}
		return "";
	}

	class EntityUtilSystem : public System {
	public:
		std::map<Guid, EntityId> guidMap;
		std::unordered_map<std::string, EntityId> nameMap;

		void init() override {
			env->systemManager->addSystem<RuntimeMode>();
			env->runtimeMode->setActiveSystem<EntityUtilSystem>({ RuntimeMode::LOADING, RuntimeMode::EDIT, RuntimeMode::PAUSED }, true);
		}

		void tick() override {
			std::map<Guid, EntityId> tmpGuidMap;
			std::unordered_map<std::string, EntityId> tmpNameMap;

			env->world->each<const EntityInfo>([&](EntityId id, EntityInfo &info) {
				tmpGuidMap[info.guid] = id;
				tmpNameMap[info.name] = id;
			});

			guidMap.swap(tmpGuidMap);
			nameMap.swap(tmpNameMap);
		}
	};
	TRI_SYSTEM(EntityUtilSystem);

	EntityId EntityUtil::getEntityByGuid(Guid guid) {
		auto &map = env->systemManager->getSystem<EntityUtilSystem>()->guidMap;
		auto entry = map.find(guid);
		if (entry == map.end()) {
			return -1;
		}
		return entry->second;
	}

	EntityId EntityUtil::getEntityByName(const std::string& name) {
		auto& map = env->systemManager->getSystem<EntityUtilSystem>()->nameMap;
		auto entry = map.find(name);
		if (entry == map.end()) {
			return -1;
		}
		return entry->second;
	}

}
