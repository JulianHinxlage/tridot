//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "ComponentCache.h"
#include "entity/World.h"
#include "engine/Serializer.h"

namespace tri {

	TRI_SYSTEM(ComponentCache);

	void ComponentCache::init() {
		if (enableComponentCaching) {
			unregisterListener = env->eventManager->onClassUnregister.addListener([&](int classId) {
				auto* desc = Reflection::getDescriptor(classId);
				if (desc) {
					for (auto* world : World::getAllWorlds()) {
						auto* storage = world->getComponentStorage(classId);
						if (storage) {
							auto* data = storage->getComponentData();
							auto* idData = storage->getIdData();
							int size = storage->size();

							Cache* cache = getCache(world, classId, true);

							for (int i = 0; i < storage->size(); i++) {
								EntityId id = idData[i];
								auto* comp = (uint8_t*)data + i * desc->size;

								SerialData ser;
								ser.emitter = std::make_shared<YAML::Emitter>();
								env->serializer->serializeClass(classId, comp, ser);
								cache->data[id] = ser.emitter->c_str();
							}
							world->removeComponentStorage(classId);
						}
					}
				}
			});
			registerListener = env->eventManager->onClassRegister.addListener([&](int classId) {
				env->eventManager->postTick.addListener([&, classId]() {
					auto* desc = Reflection::getDescriptor(classId);
					if (desc) {
						for (auto* world : World::getAllWorlds()) {
							Cache* cache = getCache(world, classId, false);
							if (cache) {
								for (auto& i : cache->data) {
									void* comp = world->addComponent(i.first, classId);
									SerialData ser;
									ser.node = YAML::Load(i.second);
									env->serializer->deserializeClass(classId, comp, ser);
								}
								cache->data.clear();
							}
						}
					}			
				}, true);
			});
		}

	}

	void ComponentCache::shutdown() {
		env->eventManager->onClassUnregister.removeListener(unregisterListener);
		env->eventManager->onClassRegister.removeListener(registerListener);
	}

	ComponentCache::Cache* ComponentCache::getCache(World* world, int classId, bool create) {
		auto* desc = Reflection::getDescriptor(classId);
		if (desc) {
			return getCache(world, desc->name, create);
		}
		else {
			return nullptr;
		}
	}

	ComponentCache::Cache* ComponentCache::getCache(World* world, const std::string& componentName, bool create) {
		for (auto& cache : caches) {
			if (cache->world == world) {
				if (cache->componentName == componentName) {
					return cache.get();
				}
			}
		}
		if (!create) {
			return nullptr;
		}
		auto cache = std::make_shared<Cache>();
		cache->world = world;
		cache->componentName = componentName;
		caches.push_back(cache);
		return cache.get();
	}

	void ComponentCache::addComponent(EntityId id, World* world, const std::string& componentName, const std::string& data) {
		if (enableComponentCaching) {
			auto* cache = getCache(world, componentName, true);
			cache->data[id] = data;
		}
	}

	void ComponentCache::serialize(EntityId id, World* world, SerialData& data) {
		if (enableComponentCaching) {
			for (auto& cache : caches) {
				if (cache->world == world) {
					auto i = cache->data.find(id);
					if (i != cache->data.end()) {
						*data.emitter << YAML::Key << cache->componentName;
						*data.emitter << YAML::Value << YAML::Load(i->second);
					}
				}
			}
		}
	}

	void ComponentCache::copyWorld(World* from, World* to) {
		if (enableComponentCaching) {
			for (auto& cache : caches) {
				if (cache->world == from) {
					for (auto& i : cache->data) {
						auto* toCache = getCache(to, cache->componentName, true);
						toCache->data[i.first] = i.second;
					}
				}
			}
		}
	}

}
