//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Serializer.h"
#include "entity/World.h"
#include "engine/Color.h"
#include "engine/AssetManager.h"
#include "engine/ComponentCache.h"
#include <glm/glm.hpp>

namespace tri {

	TRI_SYSTEM_INSTANCE(Serializer, env->serializer);

	void Serializer::serializeClass(int classId, void* ptr, SerialData& data) {
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
				*data.emitter << YAML::Key << prop.name << YAML::Value;
				serializeClass(prop.type->classId, (uint8_t*)ptr + prop.offset, data);
			}
		}
		*data.emitter << YAML::EndMap;
	}

	void Serializer::deserializeClass(int classId, void* ptr, SerialData& data) {
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
					SerialData d;
					d.node = (YAML::Node)node;
					deserializeClass(desc->elementType->classId, element, d);
				}
				return;
			}

			for (auto& prop : desc->properties) {
				if (auto i = data.node[prop.name]) {
					SerialData d;
					d.node = i;
					deserializeClass(prop.type->classId, (uint8_t*)ptr + prop.offset, d);
				}
			}
		}
	}

	void Serializer::serializeEntity(EntityId id, World* world, SerialData& data) {
		*data.emitter << YAML::BeginMap;
		*data.emitter << YAML::Key << "id" << YAML::Value << id;

		for (auto *desc : Reflection::getDescriptors()) {
			if (desc && desc->flags & ClassDescriptor::COMPONENT) {
				if (!(desc->flags & ClassDescriptor::NO_SERIALIZE)) {
					if (void* comp = world->getComponent(id, desc->classId)) {
						*data.emitter << YAML::Key << desc->name << YAML::Value;
						serializeClass(desc->classId, comp, data);
					}
				}
			}
		}
		env->systemManager->getSystem<ComponentCache>()->serialize(id, world, data);

		*data.emitter << YAML::EndMap;
	}

	void Serializer::deserializeEntity(World* world, SerialData& data) {
		EntityId id = data.node["id"].as<int>(-1);
		id = world->addEntity(id);
		
		for (auto i : data.node) {
			if (i.first.Scalar() != "id") {
				auto* desc = Reflection::getDescriptor(i.first.Scalar());
				if (desc) {
					void* comp = world->addComponent(id, desc->classId);
					SerialData d;
					d.node = i.second;
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
					SerialData d;
					d.node = (YAML::Node)entity;
					deserializeEntity(world, d);
				}
			}
		}

		world->enablePendingOperations = enablePending;
	}


	void Serializer::saveToFile(SerialData& data, const std::string& file) {
		data.stream = std::make_shared<std::ofstream>(file);
		data.emitter = std::make_shared<YAML::Emitter>(*data.stream);
	}

	bool Serializer::loadFromFile(SerialData& data, const std::string& file) {
		data.node = YAML::LoadFile(file);
		return (bool)data.node;
	}


	void Serializer::serializeWorld(World* world, const std::string& file) {
		SerialData data;
		//saveToFile(data, file);
		std::ofstream stream(file);
		data.emitter = std::make_shared<YAML::Emitter>(stream);
		serializeWorld(world, data);
	}

	void Serializer::deserializeWorld(World* world, const std::string& file) {
		if (std::filesystem::exists(file)) {
			SerialData data;
			loadFromFile(data, file);
			deserializeWorld(world, data);
		}
	}

	void Serializer::init() {

		addSerializeCallback<int>([](int* v, SerialData& data) {
			*data.emitter << *v;
		});
		addSerializeCallback<EntityId>([](EntityId* v, SerialData& data) {
			*data.emitter << (int)*v;
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


		
		addDeserializeCallback<int>([](int* v, SerialData& data) {
			*v = data.node.as<int>(0);
		});
		addDeserializeCallback<EntityId>([](EntityId* v, SerialData& data) {
			*v = data.node.as<int>(-1);
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

		env->console->addCommand("loadMap", [](auto &args) {
			if (args.size() > 0) {
				auto file = args[0];
				if (std::filesystem::path(file).extension() == ".bin") {
					env->serializer->deserializeWorldBinary(env->world, file);
				}
				else {
					env->serializer->deserializeWorld(env->world, file);
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




	template<typename T>
	void writeBin(const T& t, std::ostream& stream) {
		stream.write((char*)&t, sizeof(T));
	}

	void writeStr(const std::string& str, std::ostream& stream) {
		stream.write(str.c_str(), str.size() + 1);
	}

	template<typename T>
	void readBin(T& t, std::istream& stream) {
		stream.read((char*)&t, sizeof(T));
	}

	void readStr(std::string& str, std::istream& stream) {
		str.clear();
		char c = '\0';
		stream.read(&c, 1);
		while (c != '\0') {
			str += c;
			stream.read(&c, 1);
		}
	}

	class BinaryMapper {
	public:
		class Step {
		public:
			bool plain;
			int bytes;
			int offset;
			std::function<void(void* ptr, std::ostream& stream)> writeCallback;
			std::function<void(void* ptr, std::istream& stream)> readCallback;
		};
		std::vector<Step> steps;

		void read(void* ptr, std::istream &stream) {
			for (auto& step : steps) {
				if (step.plain) {
					stream.read((char*)ptr + step.offset, step.bytes);
				}
				else {
					step.readCallback((uint8_t*)ptr + step.offset, stream);
				}
			}
		}

		void write(void* ptr, std::ostream& stream) {
			for (auto& step : steps) {
				if (step.plain) {
					stream.write((char*)ptr + step.offset, step.bytes);
				}
				else {
					step.writeCallback((uint8_t*)ptr + step.offset, stream);
				}
			}
		}

		void create(int classId, int offset = 0) {
			auto* desc = Reflection::getDescriptor(classId);
			if (desc) {

				if (!(desc->flags & ClassDescriptor::NO_SERIALIZE)) {
					if (desc->isType<std::string>()) {
						Step step;
						step.plain = false;
						step.offset = offset;
						step.bytes = desc->size;
						step.writeCallback = [](void* ptr, std::ostream& stream) {
							writeStr(*(std::string*)ptr, stream);
						};
						step.readCallback = [](void* ptr, std::istream& stream) {
							readStr(*(std::string*)ptr, stream);
						};
						steps.push_back(step);
					}
					else if (desc->flags & ClassDescriptor::VECTOR) {
						Step step;
						step.plain = false;
						step.offset = offset;
						step.bytes = desc->size;
						int classId = desc->classId;
						step.writeCallback = [classId](void* ptr, std::ostream& stream) {
							auto* desc = Reflection::getDescriptor(classId);
							if (desc) {
								int size = desc->vectorSize(ptr);
								writeBin(size, stream);
								BinaryMapper mapper;
								mapper.create(desc->elementType->classId);
								for (int i = 0; i < size; i++) {
									mapper.write(desc->vectorGet(ptr, i), stream);
								}
							}
						};
						step.readCallback = [classId](void* ptr, std::istream& stream) {
							auto* desc = Reflection::getDescriptor(classId);
							if (desc) {
								int size = 0;
								readBin(size, stream);
								BinaryMapper mapper;
								mapper.create(desc->elementType->classId);
								for (int i = 0; i < size; i++) {
									desc->vectorInsert(ptr, i, nullptr);
									mapper.read(desc->vectorGet(ptr, i), stream);
								}
							}
						};
						steps.push_back(step);
					}
					else if (desc->flags & ClassDescriptor::REFERENCE) {
						if (desc->elementType->flags & ClassDescriptor::ASSET) {
							Step step;
							step.plain = false;
							step.offset = offset;
							step.bytes = desc->size;
							int classId = desc->elementType->classId;
							step.writeCallback = [](void* ptr, std::ostream& stream) {
								auto file = env->assetManager->getFile(*(Ref<Asset>*)ptr);
								writeStr(file, stream);
							};
							step.readCallback = [classId](void* ptr, std::istream& stream) {
								std::string file;
								readStr(file, stream);
								if (!file.empty()) {
									*(Ref<Asset>*)ptr = env->assetManager->get(classId, file);
								}
								else {
									*(Ref<Asset>*)ptr = nullptr;
								}
							};
							steps.push_back(step);
						}
					}
					else if (desc->properties.size() > 0) {
						for (auto& prop : desc->properties) {
							if (!(prop.flags & PropertyDescriptor::NO_SERIALIZE)) {
								create(prop.type->classId, prop.offset);
							}
						}
					}
					else {

						if (!(
							desc->isType<int>() ||
							desc->isType<bool>() ||
							desc->isType<float>() ||
							desc->isType<double>() ||
							desc->isType<EntityId>() ||
							desc->isType<Color>() ||
							desc->isType<glm::vec2>() ||
							desc->isType<glm::vec3>() ||
							desc->isType<glm::vec4>()
							)) {
							return;
						}

						Step step;
						step.plain = true;
						step.offset = offset;
						step.bytes = desc->size;
						bool hasMerged = false;
						if (steps.size() > 0) {
							auto& back = steps.back();
							if (back.plain) {
								if (step.offset == back.offset + back.bytes) {
									back.bytes += step.bytes;
									hasMerged = true;
								}
							}
						}
						if (!hasMerged) {
							steps.push_back(step);
						}
					}



				}

			}
		}
	};

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

		std::ofstream stream(file, std::ofstream::binary);

		int magic = 'pamt';
		writeBin(magic, stream);

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
		writeBin(entityCount, stream);
		writeBin(componentCount, stream);

		//component header
		for (auto* desc : Reflection::getDescriptors()) {
			if (desc && desc->flags & ClassDescriptor::COMPONENT) {
				auto* storage = world->getComponentStorage(desc->classId);
				if (storage && storage->size() > 0) {
					
					writeBin(storage->size(), stream);
					writeBin(desc->size, stream);
					writeStr(desc->name, stream);

				}
			}
		}

		for (int i = 0; i < entityCount; i++) {
			EntityId id = world->getEntityStorage()->getIdByIndex(i);
			writeBin(id, stream);
		}
		
		for (auto* desc : Reflection::getDescriptors()) {
			if (desc && desc->flags & ClassDescriptor::COMPONENT) {
				auto* storage = world->getComponentStorage(desc->classId);
				if (storage && storage->size() > 0) {
					int count = storage->size();

					BinaryMapper mapper;
					mapper.create(desc->classId);

					for (int i = 0; i < count; i++) {
						EntityId id = storage->getIdByIndex(i);
						writeBin(id, stream);
						mapper.write(storage->getComponentByIndex(i), stream);
					}
				}
			}
		}

		stream.close();
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

		std::ifstream stream(file, std::ifstream::binary);
		if (stream.is_open()) {

			int magic = 0;
			readBin(magic, stream);
			if (magic != 'pamt') {
				env->console->debug("file dose not match magic number");
				world->enablePendingOperations = enablePending;
				return false;
			}

			int entityCount = 0;
			int componentCount = 0;

			readBin(entityCount, stream);
			readBin(componentCount, stream);

			std::vector<ComponentStorage*> storages;
			std::vector<int> storageSizes;
			world->getEntityStorage()->reserve(entityCount);

			for (int i = 0; i < componentCount; i++) {
				int storageSize = 0;
				int elementSize = 0;
				std::string name;
				readBin(storageSize, stream);
				readBin(elementSize, stream);
				readStr(name, stream);

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
				readBin(id, stream);
				world->addEntity(id);
			}

			for (int i = 0; i < componentCount; i++) {

				int storageSize = storageSizes[i];
				auto* storage = storages[i];
				auto *desc = Reflection::getDescriptor(storage->classId);
				void* data = desc->alloc();

				BinaryMapper mapper;
				mapper.create(desc->classId);

				for (int j = 0; j < storageSize; j++) {
					EntityId id = -1;
					readBin(id, stream);

					mapper.read(data, stream);
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
