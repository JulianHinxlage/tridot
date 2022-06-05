//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"
#include "engine/Serializer.h"

namespace tri {

	class ComponentCache : public System {
	public:
		bool enableComponentCaching = true;

		void init() override;
		void shutdown() override;

		void addComponent(EntityId id, World* world, const std::string &componentName, const std::string& data);
		void serialize(EntityId id, World* world, SerialData &data);

	private:
		class Cache {
		public:
			World* world;
			std::string componentName;
			std::unordered_map<EntityId, std::string> data;
		};
		std::vector<std::shared_ptr<Cache>> caches;
		int registerListener = -1;
		int unregisterListener = -1;

		Cache* getCache(World* world, int classId, bool create);
		Cache* getCache(World *world, const std::string& componentName, bool create);
	};

}
