//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Serializer.h"
#include "entity/World.h"
#include "engine/Color.h"
#include "engine/AssetManager.h"
#include "engine/ComponentCache.h"
#include "engine/EntityEvent.h"
#include "engine/Map.h"
#include "engine/MetaTypes.h"
#include "EntityUtil.h"
#include "Archive.h"
#include <glm/glm.hpp>

namespace tri {

	TRI_SYSTEM_INSTANCE(Serializer, env->serializer);

	void Serializer::serializeClass(int classId, void* ptr, SerialData& data, bool replication) {
		if (classId < 0) {
			return;
		}
		if (serializeCallbacks.size() > classId) {
			if (serializeCallbacks[classId]) {
				serializeCallbacks[classId](ptr, data);
				return;
			}
		}

		auto* desc = Reflection::getDescriptor(classId);
		if (desc->enumValues.size() > 0) {
			int value = *(int*)ptr;
			for (auto& e : desc->enumValues) {
				if (e.second == value) {
					*data.emitter << YAML::Value << e.first;
					return;
				}
			}
			*data.emitter << YAML::Value << 0;
			return;
		}

		if (desc->flags & ClassDescriptor::REFERENCE) {
			std::string file = env->assetManager->getFile(*(Ref<Asset>*)ptr);
			if (!file.empty()) {
				*data.emitter << YAML::Value << file;
			}
			else {
				*data.emitter << YAML::Value << YAML::Null;
			}
			return;
		}

		if (desc->flags & ClassDescriptor::VECTOR) {
			*data.emitter << YAML::BeginSeq;
			int size = desc->vectorSize(ptr);
			for (int i = 0; i < size; i++) {
				void *element = desc->vectorGet(ptr, i);
				serializeClass(desc->elementType->classId, element, data);
			}
			*data.emitter << YAML::EndSeq;
			return;
		}

		*data.emitter << YAML::BeginMap;
		for (auto& prop : desc->properties) {
			if (!(prop.flags & PropertyDescriptor::NO_SERIALIZE)) {
				if (!replication || (prop.flags & PropertyDescriptor::REPLICATE)) {
					*data.emitter << YAML::Key << prop.name << YAML::Value;
					serializeClass(prop.type->classId, (uint8_t*)ptr + prop.offset, data);
				}
			}
		}
		*data.emitter << YAML::EndMap;
	}

	void Serializer::deserializeClass(int classId, void* ptr, SerialData& data) {
		if (classId < 0) {
			return;
		}
		if (deserializeCallbacks.size() > classId) {
			if (deserializeCallbacks[classId]) {
				deserializeCallbacks[classId](ptr, data);
				return;
			}
		}
		
		auto* desc = Reflection::getDescriptor(classId);
		if (desc) {

			if (desc->enumValues.size() > 0) {
				std::string str = data.node.as<std::string>("");
				for (auto& e : desc->enumValues) {
					if (e.first == str) {
						*(int*)ptr = e.second;
						return;
					}
				}
				*(int*)ptr = 0;
				return;
			}

			if (desc->flags & ClassDescriptor::REFERENCE) {
				if (!data.node.IsNull()) {
					std::string file = data.node.as<std::string>("");
					if (!file.empty()) {
						*(Ref<Asset>*)ptr = env->assetManager->get(desc->elementType->classId, file);
					}
				}
				return;
			}

			if (desc->flags & ClassDescriptor::VECTOR) {

				int index = 0;
				for (auto node : data.node) {
					desc->vectorInsert(ptr, index, nullptr);
					void* element = desc->vectorGet(ptr, index);
					index++;
					SerialData d(node);
					deserializeClass(desc->elementType->classId, element, d);
				}
				return;
			}

			for (auto& prop : desc->properties) {
				if (data.node.IsMap()) {
					if (auto i = data.node[prop.name]) {
						SerialData d(i);
						deserializeClass(prop.type->classId, (uint8_t*)ptr + prop.offset, d);
					}
				}
			}
		}
	}

	void Serializer::serializeEntity(EntityId id, World* world, SerialData& data, bool replication) {
		*data.emitter << YAML::BeginMap;
		if (!replication) {
			*data.emitter << YAML::Key << "id" << YAML::Value << id;
			*data.emitter << YAML::Key << "active" << YAML::Value << world->isEntityActive(id);
		}

		for (auto *desc : Reflection::getDescriptors()) {
			if (desc && desc->flags & ClassDescriptor::COMPONENT) {
				if (!(desc->flags & ClassDescriptor::NO_SERIALIZE)) {
					if (!replication || (desc->flags & PropertyDescriptor::REPLICATE)) {
						if (void* comp = world->getComponent(id, desc->classId)) {
							*data.emitter << YAML::Key << desc->name << YAML::Value;
							serializeClass(desc->classId, comp, data, replication);
						}
					}
				}
			}
		}
		env->systemManager->getSystem<ComponentCache>()->serialize(id, world, data);

		*data.emitter << YAML::EndMap;
	}

	void Serializer::deserializeEntity(EntityId id, World* world, SerialData& data, std::map<EntityId, EntityId>* idMap) {
		bool active = data.node["active"].as<bool>(true);
		EntityId hint = data.node["id"].as<int>(-1);


		for (auto i : data.node) {
			if (i.first.Scalar() != "id" && i.first.Scalar() != "active") {
				auto* desc = Reflection::getDescriptor(i.first.Scalar());
				if (desc) {
					void* comp = world->getOrAddComponent(id, desc->classId);
					SerialData d(i.second);
					deserializeClass(desc->classId, comp, d);
				}
				else {
					//unknown component will be added to the component cache to not lose data
					YAML::Emitter e;
					e << i.second;
					env->systemManager->getSystem<ComponentCache>()->addComponent(id, world, i.first.Scalar(), e.c_str());
				}
			}
		}

		if (hint != id && idMap) {
			(*idMap)[hint] = id;
		}

		if (!active) {
			world->setEntityActive(id, active);
		}
	}


	void Serializer::deserializeEntity(World* world, SerialData& data, std::map<EntityId, EntityId> *idMap) {
		EntityId id = data.node["id"].as<int>(-1);
		id = world->addEntity(id);
		deserializeEntity(id, world, data, idMap);
	}

	void Serializer::serializeWorld(World* world, SerialData& data) {
		*data.emitter << YAML::BeginMap;

		*data.emitter << YAML::Key << "Header";
		*data.emitter << YAML::BeginMap;
		*data.emitter << YAML::Key << "Entity" << YAML::Value << world->getEntityStorage()->size();
		for (auto* desc : Reflection::getDescriptors()) {
			if (desc && desc->flags & ClassDescriptor::COMPONENT) {
				auto *storage = world->getComponentStorage(desc->classId);
				if (storage && storage->size() > 0) {
					*data.emitter << YAML::Key << desc->name << YAML::Value << storage->size();
				}
			}
		}
		*data.emitter << YAML::EndMap;

		*data.emitter << YAML::Key << "Body";
		*data.emitter << YAML::BeginSeq;
		world->each<>([&](EntityId id) {
			serializeEntity(id, world, data);
		});
		*data.emitter << YAML::EndSeq;

		*data.emitter << YAML::EndMap;
	}

	void Serializer::deserializeWorld(World* world, SerialData& data) {
		world->clear();
		bool enablePending = world->enablePendingOperations;
		world->enablePendingOperations = false;

		if (auto header = data.node["Header"]) {

			for (auto i : header) {
				if (i.first.Scalar() == "Entity") {
					int count = i.second.as<int>(0);
					world->getEntityStorage()->reserve(count);
				}
				else {
					auto* desc = Reflection::getDescriptor(i.first.Scalar());
					if (desc) {
						int count = i.second.as<int>(0);
						auto* storage = world->getComponentStorage(desc->classId);
						if (storage) {
							storage->reserve(count);
						}
					}
				}
			}

		}

		if (auto body = data.node["Body"]) {
			for (auto entity : body) {
				if (entity) {
					SerialData d(entity);
					deserializeEntity(world, d);
				}
			}
		}

		world->enablePendingOperations = enablePending;
	}

	void Serializer::serializePrefab(Prefab* prefab, SerialData& data) {
		*data.emitter << YAML::BeginMap;
		*data.emitter << YAML::Key << "id" << YAML::Value << prefab->getEntityId();

		for (auto& comp : prefab->getComponents()) {
			auto* desc = Reflection::getDescriptor(comp.classId);
			if (desc) {
				*data.emitter << YAML::Key << desc->name << YAML::Value;
				serializeClass(comp.classId, comp.data, data);
			}
		}

		if (prefab->getChilds().size() > 0) {
			*data.emitter << YAML::Key << "Childs" << YAML::Value;
			*data.emitter << YAML::BeginSeq;
			for (auto& child : prefab->getChilds()) {
				if (child) {
					serializePrefab(child.get(), data);
				}
			}
			*data.emitter << YAML::EndSeq;
		}

		*data.emitter << YAML::EndMap;
	}

	void Serializer::deserializePrefab(Prefab* prefab, SerialData& data) {
		prefab->setEntityId(data.node["id"].as<int>(-1));

		for (auto i : data.node) {
			if (i.first.Scalar() != "Childs") {
				auto* desc = Reflection::getDescriptor(i.first.Scalar());
				if (desc) {
					void *comp = prefab->addComponent(desc->classId);
					SerialData d(i.second);
					deserializeClass(desc->classId, comp, d);
				}
			}
		}

		if (auto childs = data.node["Childs"]) {
			for (auto i : childs) {
				SerialData d(i);
				deserializePrefab(prefab->addChild(), d);
			}
		}
	}

	void Serializer::saveToFile(SerialData& data, const std::string& file) {
		data.stream = std::make_shared<std::ofstream>(file);
		data.emitter = std::make_shared<YAML::Emitter>(*data.stream);
	}

	bool Serializer::loadFromFile(SerialData& data, const std::string& file) {
		try {
			data.node = YAML::LoadFile(file);
			return (bool)data.node;
		}
		catch (...) { return false; }
	}


	void Serializer::serializeWorld(World* world, const std::string& file) {
		Clock clock;
		SerialData data;
		//saveToFile(data, file);
		std::ofstream stream(file);
		data.emitter = std::make_shared<YAML::Emitter>(stream);
		serializeWorld(world, data);
		env->console->info("saving world to \"%s\" took %f s", std::filesystem::path(file).filename().string().c_str(), clock.elapsed());
	}

	bool Serializer::deserializeWorld(World* world, const std::string& file) {
		if (std::filesystem::exists(file)) {
			Clock clock;
			SerialData data;
			if (!loadFromFile(data, file)) {
				env->console->warning("failed to load file %s", file.c_str());
				return false;
			}
			deserializeWorld(world, data);
			env->console->info("loading world \"%s\" took %f s", std::filesystem::path(file).filename().string().c_str(), clock.elapsed());
		}
		else {
			env->console->warning("file %s not found", file.c_str());
			return false;
		}
		return true;
	}

	void Serializer::init() {

		addSerializeCallback<int>([](int* v, SerialData& data) {
			*data.emitter << *v;
		});
		addSerializeCallback<EntityId>([](EntityId* v, SerialData& data) {
			*data.emitter << (int)*v;
		});
		addSerializeCallback<Guid>([](Guid* v, SerialData& data) {
			*data.emitter << v->toString();
		});
		addSerializeCallback<float>([](float* v, SerialData& data) {
			*data.emitter << *v;
		});
		addSerializeCallback<double>([](double* v, SerialData& data) {
			*data.emitter << *v;
		}); 
		addSerializeCallback<bool>([](bool* v, SerialData& data) {
			*data.emitter << *v;
		});
		addSerializeCallback<glm::vec2>([](glm::vec2* v, SerialData& data) {
			*data.emitter << YAML::Flow << YAML::BeginSeq << v->x << v->y << YAML::EndSeq;
		});
		addSerializeCallback<glm::vec3>([](glm::vec3* v, SerialData& data) {
			*data.emitter << YAML::Flow << YAML::BeginSeq << v->x << v->y << v->z << YAML::EndSeq;
		});
		addSerializeCallback<glm::vec4>([](glm::vec4* v, SerialData& data) {
			*data.emitter << YAML::Flow << YAML::BeginSeq << v->x << v->y << v->z << v->w << YAML::EndSeq;
		});
		addSerializeCallback<std::string>([](std::string* v, SerialData& data) {
			*data.emitter << *v;
		});
		addSerializeCallback<Color>([](Color* v, SerialData& data) {
			*data.emitter << YAML::Flow << YAML::BeginSeq << (int)v->r << (int)v->g << (int)v->b << (int)v->a << YAML::EndSeq;
		});


		addSerializeCallback<ComponentIdentifier>([](ComponentIdentifier* v, SerialData& data) {
			if (v->classId != -1) {
				auto* desc = Reflection::getDescriptor(v->classId);
				if (desc) {
					*data.emitter << YAML::Key << "component" << YAML::Value << desc->name;
				}
			}
		});
		addSerializeCallback<PropertyIdentifier>([](PropertyIdentifier* v, SerialData& data) {
			*data.emitter << YAML::BeginMap;
			env->serializer->serializeClass((ComponentIdentifier*)v, data);
			if (v->classId != -1) {
				auto* desc = Reflection::getDescriptor(v->classId);
				if (desc) {
					if (v->propertyIndex >= 0 && v->propertyIndex < desc->properties.size()) {
						auto property = desc->properties[v->propertyIndex];
						*data.emitter << YAML::Key << "property" << YAML::Value << property.name;
					}
				}
			}
			*data.emitter << YAML::EndMap;
		});
		addSerializeCallback<PropertyValueIdentifier>([](PropertyValueIdentifier* v, SerialData& data) {
			*data.emitter << YAML::BeginMap;
			env->serializer->serializeClass((ComponentIdentifier*)v, data);
			if (v->classId != -1) {
				auto* desc = Reflection::getDescriptor(v->classId);
				if (desc) {
					if (v->propertyIndex >= 0 && v->propertyIndex < desc->properties.size()) {
						auto property = desc->properties[v->propertyIndex];
						*data.emitter << YAML::Key << "property" << YAML::Value << property.name;
					}
				}
			}
			*data.emitter << YAML::Key << "value";
			env->serializer->serializeClass(v->value.classId, v->value.get(), data);
			*data.emitter << YAML::EndMap;
		});
		addSerializeCallback<FunctionIdentifier>([](FunctionIdentifier* v, SerialData& data) {
			*data.emitter << YAML::BeginMap;
			env->serializer->serializeClass((ComponentIdentifier*)v, data);
			if (v->classId != -1) {
				auto* desc = Reflection::getDescriptor(v->classId);
				if (desc) {
					if (v->functionIndex >= 0 && v->functionIndex < desc->functions.size()) {
						auto *function = desc->functions[v->functionIndex];
						*data.emitter << YAML::Key << "function" << YAML::Value << function->name;
					}
				}
			}
			*data.emitter << YAML::EndMap;
		});
		
		addDeserializeCallback<int>([](int* v, SerialData& data) {
			*v = data.node.as<int>(0);
		});
		addDeserializeCallback<EntityId>([](EntityId* v, SerialData& data) {
			*v = data.node.as<int>(-1);
		});
		addDeserializeCallback<Guid>([](Guid* v, SerialData& data) {
			v->fromString(data.node.as<std::string>(""));
		});
		addDeserializeCallback<float>([](float* v, SerialData& data) {
			*v = data.node.as<float>(0);
		});
		addDeserializeCallback<double>([](double* v, SerialData& data) {
			*v = data.node.as<double>(0);
		}); 
		addDeserializeCallback<bool>([](bool* v, SerialData& data) {
			*v = data.node.as<bool>(0);
		});
		addDeserializeCallback<glm::vec2>([](glm::vec2* v, SerialData& data) {
			v->x = data.node[0].as<float>(0);
			v->y = data.node[1].as<float>(0);
		});
		addDeserializeCallback<glm::vec3>([](glm::vec3* v, SerialData& data) {
			v->x = data.node[0].as<float>(0);
			v->y = data.node[1].as<float>(0);
			v->z = data.node[2].as<float>(0);
		});
		addDeserializeCallback<glm::vec4>([](glm::vec4* v, SerialData& data) {
			v->x = data.node[0].as<float>(0);
			v->y = data.node[1].as<float>(0);
			v->z = data.node[2].as<float>(0);
			v->w = data.node[3].as<float>(0);
		});
		addDeserializeCallback<std::string>([](std::string* v, SerialData& data) {
			*v = data.node.as<std::string>("");
		});
		addDeserializeCallback<Color>([](Color* v, SerialData& data) {
			v->r = data.node[0].as<int>(255);
			v->g = data.node[1].as<int>(255);
			v->b = data.node[2].as<int>(255);
			v->a = data.node[3].as<int>(255);
		});

		addDeserializeCallback<ComponentIdentifier>([](ComponentIdentifier* v, SerialData& data) {
			std::string component = data.node["component"].as<std::string>("");
			auto* desc = Reflection::getDescriptor(component);
			if (desc) {
				v->classId = desc->classId;
			}
		});
		addDeserializeCallback<PropertyIdentifier>([](PropertyIdentifier* v, SerialData& data) {
			env->serializer->deserializeClass((ComponentIdentifier*)v, data);
			std::string property = data.node["property"].as<std::string>("");
			if (v->classId != -1) {
				auto* desc = Reflection::getDescriptor(v->classId);
				if (desc) {
					for (int i = 0; i < desc->properties.size(); i++) {
						if (desc->properties[i].name == property) {
							v->propertyIndex = i;
							break;
						}
					}
				}
			}
		});
		addDeserializeCallback<PropertyValueIdentifier>([](PropertyValueIdentifier* v, SerialData& data) {
			env->serializer->deserializeClass((PropertyIdentifier*)v, data);
			SerialData d;
			d.node = data.node["value"];
			if (d.node) {
				if (v->classId != -1) {
					auto* desc = Reflection::getDescriptor(v->classId);
					if (desc) {
						if (v->propertyIndex >= 0 && v->propertyIndex < desc->properties.size()) {
							auto &property = desc->properties[v->propertyIndex];
							v->value.set(property.type->classId);
						}
					}
				}
				env->serializer->deserializeClass(v->value.classId, v->value.get(), d);
			}
		});
		addDeserializeCallback<FunctionIdentifier>([](FunctionIdentifier* v, SerialData& data) {
			env->serializer->deserializeClass((ComponentIdentifier*)v, data);
			std::string function = data.node["function"].as<std::string>("");
			auto* desc = Reflection::getDescriptor(v->classId);
			if (desc) {
				for (int i = 0; i < desc->functions.size(); i++) {
					if (desc->functions[i]->name == function) {
						v->functionIndex = i;
						break;
					}
				}
			}
		});

		env->console->addCommand("loadMap", [](auto &args) {
			if (args.size() > 0) {
				auto file = args[0];
				if (std::filesystem::path(file).extension() == ".bin") {
					env->serializer->deserializeWorldBinary(env->world, file);
				}
				else {
					Map::loadAndSetToActiveWorld(file, env->editor ? RuntimeMode::EDIT : RuntimeMode::PLAY);
				}
			}
			else {
				env->console->info("usage: loadMap <file>");
			}
		});
	}

	void Serializer::addSerializeCallback(int classId, const std::function<void(void* ptr, SerialData& data)>& callback) {
		if (serializeCallbacks.size() <= classId) {
			serializeCallbacks.resize(classId + 1);
		}
		serializeCallbacks[classId] = callback;
	}

	void Serializer::addDeserializeCallback(int classId, const std::function<void(void* ptr, SerialData& data)>& callback) {
		if (deserializeCallbacks.size() <= classId) {
			deserializeCallbacks.resize(classId + 1);
		}
		deserializeCallbacks[classId] = callback;
	}


	void Serializer::serializeEntityBinary(EntityId id, World* world, Archive& memoryArchive) {
		BinaryArchive archive(&memoryArchive);
		
		archive.writeBin(id);
		for (auto* desc : Reflection::getDescriptors()) {
			if (desc && desc->flags & ClassDescriptor::COMPONENT) {
				if (void* comp = world->getComponent(id, desc->classId)) {
					archive.writeStr(desc->name);
					archive.writeClass(comp, desc->classId);
				}
			}
		}
	}

	void Serializer::deserializeEntityBinary(World* world, Archive& memoryArchive, std::map<EntityId, EntityId>* idMap) {
		deserializeEntityBinary(-1, world, memoryArchive, idMap);
	}

	void Serializer::deserializeEntityBinary(EntityId id, World* world, Archive& memoryArchive, std::map<EntityId, EntityId>* idMap) {
		BinaryArchive archive(&memoryArchive);

		EntityId savedId = -1;
		archive.readBin(savedId);

		if (id == -1) {
			id = world->addEntity(savedId);
		}
		if (id != savedId && idMap) {
			(*idMap)[savedId] = id;
		}

		while(archive.hasDataLeft()){
			std::string name;
			archive.readStr(name);

			if (name == "") {
				break;
			}

			auto* desc = Reflection::getDescriptor(name);
			if (desc) {
				void* comp = world->getOrAddComponentPending(id, desc->classId);
				archive.readClass(comp, desc->classId);
			}
			else {
				break;
			}
		}
	}

	void Serializer::serializeWorldBinary(World* world, const std::string& file) {

		/*
			layout:

			// Header //
			4B magic
			4B entity count
			4B component count
			for component count:
				4B storage size
				4B element size
				str name

			// Body //
			for entity count
				4B id
			for component count:
				4B id
				(element size Bytes) data
		*/

		FileArchive fileArchive;
		BinaryArchive archive(&fileArchive);
		fileArchive.openFileForWrite(file);

		int magic = 'pamt';
		archive.writeBin(magic);

		int componentCount = 0;
		for (auto* desc : Reflection::getDescriptors()) {
			if (desc && desc->flags & ClassDescriptor::COMPONENT) {
				auto* storage = world->getComponentStorage(desc->classId);
				if (storage && storage->size() > 0) {
					componentCount++;
				}
			}
		}

		int entityCount = world->getEntityStorage()->size();
		archive.writeBin(entityCount);
		archive.writeBin(componentCount);

		//component header
		for (auto* desc : Reflection::getDescriptors()) {
			if (desc && desc->flags & ClassDescriptor::COMPONENT) {
				auto* storage = world->getComponentStorage(desc->classId);
				if (storage && storage->size() > 0) {

					archive.writeBin(storage->size());
					archive.writeBin(desc->size);
					archive.writeStr(desc->name);

				}
			}
		}

		for (int i = 0; i < entityCount; i++) {
			EntityId id = world->getEntityStorage()->getIdByIndex(i);
			archive.writeBin(id);
		}

		for (auto* desc : Reflection::getDescriptors()) {
			if (desc && desc->flags & ClassDescriptor::COMPONENT) {
				auto* storage = world->getComponentStorage(desc->classId);
				if (storage && storage->size() > 0) {
					int count = storage->size();

					for (int i = 0; i < count; i++) {
						EntityId id = storage->getIdByIndex(i);
						archive.writeBin(id);
						archive.writeClass(storage->getComponentByIndex(i), desc->classId);
					}
				}
			}
		}

	}

	bool Serializer::deserializeWorldBinary(World* world, const std::string& file) {

		/*
			layout:

			// Header //
			4B magic
			4B entity count
			4B component count
			for component count:
				4B storage size
				4B element size
				str name

			// Body //
			for entity count
				4B id
			for component count:
				4B id
				(element size Bytes) data
		*/

		world->clear();
		bool enablePending = world->enablePendingOperations;
		world->enablePendingOperations = false;

		FileArchive fileArchive;
		BinaryArchive archive(&fileArchive);
		if(fileArchive.openFileForRead(file)) {

			int magic = 0;
			archive.readBin(magic);
			if (magic != 'pamt') {
				env->console->debug("file dose not match magic number");
				world->enablePendingOperations = enablePending;
				return false;
			}

			int entityCount = 0;
			int componentCount = 0;

			archive.readBin(entityCount);
			archive.readBin(componentCount);

			std::vector<ComponentStorage*> storages;
			std::vector<int> storageSizes;
			world->getEntityStorage()->reserve(entityCount);

			for (int i = 0; i < componentCount; i++) {
				int storageSize = 0;
				int elementSize = 0;
				std::string name;
				
				archive.readBin(storageSize);
				archive.readBin(elementSize);
				archive.readStr(name);

				auto *desc = Reflection::getDescriptor(name);

				if (!desc) {
					env->console->debug("component not found %s", name.c_str());
					world->enablePendingOperations = enablePending;
					return false;
				}

				if (desc->size != elementSize) {
					env->console->debug("component size dose not match for component %s", name.c_str());
					world->enablePendingOperations = enablePending;
					return false;
				}

				auto *storage = world->getComponentStorage(desc->classId);
				if (!storage) {
					env->console->debug("component size dose not match for component %s", name.c_str());
					world->enablePendingOperations = enablePending;
					return false;
				}

				storage->reserve(storageSize);
				storages.push_back(storage);
				storageSizes.push_back(storageSize);
			}


			// Body //
			for (int i = 0; i < entityCount; i++) {
				EntityId id = -1;
				archive.readBin(id);
				world->addEntity(id);
			}

			for (int i = 0; i < componentCount; i++) {

				int storageSize = storageSizes[i];
				auto* storage = storages[i];
				auto *desc = Reflection::getDescriptor(storage->classId);
				void* data = desc->alloc();

				for (int j = 0; j < storageSize; j++) {
					EntityId id = -1;
					archive.readBin(id);

					archive.readClass(data, desc->classId);
					if (id != -1) {
						world->addComponent(id, desc->classId, data);
					}
				}
				desc->free(data);
			}
		}
		else {
			env->console->debug("file not found %s", file.c_str());
			world->enablePendingOperations = enablePending;
			return false;
		}

		world->enablePendingOperations = enablePending;
		return true;
	}

}
