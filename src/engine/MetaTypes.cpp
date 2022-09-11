//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "MetaTypes.h"
#include "core/core.h"
#include "editor/Editor.h"
#include "editor/CLassUI.h"

namespace tri {

	TRI_CLASS(ComponentIdentifier);
	TRI_PROPERTIES1(ComponentIdentifier, classId);

	TRI_CLASS(PropertyIdentifier);
	TRI_PROPERTIES2(PropertyIdentifier, classId, propertyIndex);

	TRI_CLASS(PropertyValueIdentifier);
	TRI_PROPERTIES3(PropertyValueIdentifier, classId, propertyIndex, value);

	TRI_CLASS(FunctionIdentifier);
	TRI_PROPERTIES2(FunctionIdentifier, classId, functionIndex);


	void* ComponentIdentifier::getComponent(EntityId id, World* world) {
		if (world && classId >= 0) {
			return world->getComponent(id, classId);
		}
		else {
			return nullptr;
		}
	}

	const PropertyDescriptor* PropertyIdentifier::getPropertyDescriptor() {
		if (classId >= 0) {
			auto *desc = Reflection::getDescriptor(classId);
			if (desc) {
				if (propertyIndex >= 0 && propertyIndex < desc->properties.size()) {
					return &desc->properties[propertyIndex];
				}
			}
		}
		return nullptr;
	}

	void* PropertyIdentifier::getProperty(EntityId id, World* world) {
		void* comp = getComponent(id, world);
		if (comp) {
			auto* desc = getPropertyDescriptor();
			if (desc) {
				return (uint8_t*)comp + desc->offset;
			}
		}
		return nullptr;
	}

	const FunctionDescriptor* FunctionIdentifier::getFunctionDescriptor() {
		if (classId >= 0) {
			auto* desc = Reflection::getDescriptor(classId);
			if (desc) {
				if (functionIndex >= 0 && functionIndex < desc->functions.size()) {
					return desc->functions[functionIndex];
				}
			}
		}
		return nullptr;
	}

}
