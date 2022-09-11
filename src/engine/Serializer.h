//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"
#include <yaml-cpp/yaml.h>

namespace tri {

	class SerialData {
	public:
		YAML::Node node;
		std::shared_ptr<YAML::Emitter> emitter;
		std::shared_ptr<std::ostream> stream;
	};

	class Serializer : public System {
	public:
		void init() override;

		void serializeClass(int classId, void *ptr, SerialData &data);
		void deserializeClass(int classId, void *ptr, SerialData& data);
		
		void serializeEntity(EntityId id, World *world, SerialData& data);
		void deserializeEntity(World* world, SerialData& data);

		void serializePrefab(SerialData& data);
		void deserializePrefab(SerialData& data);
		
		void serializeWorld(World* world, SerialData& data);
		void deserializeWorld(World* world, SerialData& data);

		void saveToFile(SerialData& data, const std::string& file);
		bool loadFromFile(SerialData& data, const std::string& file);

		void serializeWorld(World* world, const std::string& file);
		bool deserializeWorld(World* world, const std::string& file);

		void addSerializeCallback(int classId, const std::function<void(void* ptr, SerialData& data)>& callback);
		void addDeserializeCallback(int classId, const std::function<void(void* ptr, SerialData& data)>& callback);

		void serializeWorldBinary(World* world, const std::string& file);
		bool deserializeWorldBinary(World* world, const std::string& file);

		template<typename T>
		void serializeClass(T* ptr, SerialData& data) {
			serializeClass(Reflection::getClassId<T>(), ptr, data);
		}
		template<typename T>
		void deserializeClass(T* ptr, SerialData& data) {
			deserializeClass(Reflection::getClassId<T>(), ptr, data);
		}

		template<typename T>
		void addSerializeCallback(const std::function<void(T* ptr, SerialData& data)>& callback) {
			addSerializeCallback(Reflection::getClassId<T>(), [callback](void* ptr, SerialData& data) {
				callback((T*)ptr, data);
			});
		}
		template<typename T>
		void addDeserializeCallback(const std::function<void(T* ptr, SerialData& data)>& callback) {
			addDeserializeCallback(Reflection::getClassId<T>(), [callback](void* ptr, SerialData& data) {
				callback((T*)ptr, data);
			});
		}

	private:
		std::vector<std::function<void(void *ptr, SerialData& data)>> serializeCallbacks;
		std::vector<std::function<void(void *ptr, SerialData& data)>> deserializeCallbacks;
	};

}
