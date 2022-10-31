//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "EntityUtil.h"
#include "core/core.h"

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

}
