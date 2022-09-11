//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "entity/Prefab.h"

namespace tri {

	class ComponentIdentifier {
	public:
		int classId = -1;

		void *getComponent(EntityId id, World *world);
	};

	class PropertyIdentifier : public ComponentIdentifier {
	public:
		int propertyIndex = -1;

		const PropertyDescriptor* getPropertyDescriptor();
		void *getProperty(EntityId id, World* world);
	};

	class PropertyValueIdentifier : public PropertyIdentifier {
	public:
		DynamicObjectBuffer value;
	};

	class FunctionIdentifier : public ComponentIdentifier {
	public:
		int functionIndex = -1;
		
		const FunctionDescriptor* getFunctionDescriptor();
	};

}
