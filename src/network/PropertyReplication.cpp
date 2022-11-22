//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "PropertyReplication.h"
#include "core/core.h"
#include "engine/EntityUtil.h"
#include "NetworkComponent.h"
#include "NetworkManager.h"
#include "NetworkReplication.h"
#include "engine/Time.h"
#include "engine/Serializer.h"
#include "engine/RuntimeMode.h"
#include <sstream>

namespace tri {

	TRI_SYSTEM(PropertyReplication);

	bool floatEquals(float* v1, float* v2, int count, float epsilon) {
		for (int i = 0; i < count; i++) {
			float diff = v1[i] - v2[i];
			if (diff > epsilon || diff < -epsilon) {
				return false;
			}
		}
		return true;
	}

	bool classEquals(void* v1, void* v2, ClassDescriptor* desc, float epsilon) {
		if (desc->classId == Reflection::getClassId<float>() ||
			desc->classId == Reflection::getClassId<glm::vec2>() ||
			desc->classId == Reflection::getClassId<glm::vec3>() ||
			desc->classId == Reflection::getClassId<glm::vec4>()) {
			return floatEquals((float*)v1, (float*)v2, desc->size / sizeof(float), epsilon);
		}
		else {
			return desc->equals(v1, v2);
		}
	}

	template<typename T>
	void writeBin(const T& t, std::ostream& stream) {
		stream.write((char*)&t, sizeof(T));
	}
	template<typename T>
	void readBin(T& t, std::istream& stream) {
		stream.read((char*)&t, sizeof(T));
	}

	enum NextField : uint8_t {
		PACKET_END,
		ENTITY,
		PROPERTY,
	};

	void PropertyReplication::init() {
		env->runtimeMode->setActiveSystem<PropertyReplication>({ RuntimeMode::LOADING, RuntimeMode::EDIT, RuntimeMode::PAUSED }, true);
		env->jobManager->addJob("Network")->addSystem<PropertyReplication>();
	}

	void PropertyReplication::startup() {
		env->networkManager->packetCallbacks[NetOpcode::PROPERTY_DATA] = [&](Connection* conn, NetOpcode opcode, Packet& packet) {
			if (!env->networkReplication || !env->networkManager) {
				return;
			}

			{
				TRI_PROFILE("process property update");
				std::unique_lock<std::mutex> lock(env->world->performePendingMutex);
				std::stringstream stream(std::string(packet.data(), packet.data() + packet.size()));

				Guid guid;
				EntityId id;
				bool ignoreProperty = true;
				while (true) {
					NextField next = PACKET_END;
					readBin(next, stream);

					if (next == PROPERTY) {

						size_t hashCode = 0;
						uint8_t index = 0;
						readBin(hashCode, stream);
						readBin(index, stream);

						bool processedProperty = false;
						for (auto* desc : Reflection::getDescriptors()) {
							if (desc && desc->hashCode == hashCode) {
								if (desc->properties.size() > index) {
									auto& prop = desc->properties[index];

									if (void* comp = env->world->getComponent(id, desc->classId)) {
										void* ptr = (uint8_t*)comp + prop.offset;
										if (!ignoreProperty && (prop.flags & PropertyDescriptor::REPLICATE)) {
											env->serializer->getMapper(prop.type->classId)->read(ptr, stream);
										}
										else {
											//todo: cache tmp buffers
											DynamicObjectBuffer tmp;
											tmp.set(prop.type->classId);
											env->serializer->getMapper(prop.type->classId)->read(tmp.get(), stream);
										}
										processedProperty = true;
									}

									break;
								}
							}
						}
						if (!processedProperty) {
							env->console->warning("could not process property %i %i", hashCode, index);
							break;
						}
					}
					else if (next == ENTITY) {
						readBin(guid, stream);
						ignoreProperty = true;

						if (env->networkManager->hasAuthority()) {
							if (env->networkReplication->getOwningConnection(guid) == conn) {
								id = EntityUtil::getEntityByGuid(guid);
								if (id != -1) {
									ignoreProperty = false;
								}
							}
						}
						else {
							if (!env->networkReplication->isOwning(guid)) {
								id = EntityUtil::getEntityByGuid(guid);
								if (id != -1) {
									ignoreProperty = false;
								}
							}
						}
					}
					else if (next == PACKET_END) {
						break;
					}
				}
			}


			if (env->networkManager->hasAuthority()) {
				TRI_PROFILE("relay property update");
				//relay packet to other clients
				packet.unskip(packet.readIndex);
				env->networkManager->sendToAll(packet, conn);
			}
		};
	}

	void PropertyReplication::tick() {
		std::stringstream stream;
		writeBin(NetOpcode::PROPERTY_DATA, stream);
		bool hasData = false;

		if (env->time->frameTicks(1.0f / 60.0f)) {
			if (env->networkManager->isConnected()) {

				std::vector<EntityId> ids;
				std::vector<Guid> guids;
				env->world->each<NetworkComponent>([&](EntityId id, NetworkComponent& net) {
					if (net.syncAlways) {
						Guid guid = EntityUtil::getGuid(id);
						if (env->networkReplication->isOwning(guid)) {
							ids.push_back(id);
							guids.push_back(guid);
						}
					}
				});

				for (int i = 0; i < ids.size(); i++) {
					EntityId& id = ids[i];
					Guid &guid = guids[i];

					for (auto* desc : Reflection::getDescriptors()) {
						if (desc && (desc->flags & ClassDescriptor::COMPONENT)) {
							if (desc->flags & ClassDescriptor::REPLICATE) {
								bool first = true;
								if (void* comp = env->world->getComponent(id, desc->classId)) {

									if (storages.size() <= desc->classId) {
										storages.resize(desc->classId + 1);
										storages[desc->classId].resize(desc->properties.size());
									}

									auto& buffers = storages[desc->classId];
									for (int j = 0; j < desc->properties.size(); j++) {
										auto& prop = desc->properties[j];
										if (prop.flags & PropertyDescriptor::REPLICATE) {

											if (!buffers[j]) {
												buffers[j] = std::make_shared<ComponentStorage>(prop.type->classId);
											}
											auto& buffer = buffers[j];

									
											void* ptr = (uint8_t*)comp + prop.offset;
											void* ptr2 = buffer->getComponentById(id);

											if (ptr2) {
												if (!classEquals(ptr, ptr2, prop.type, 0.0001f)) {

													if (first) {
														first = false;
														writeBin(NextField::ENTITY, stream);
														writeBin(guid, stream);
													}

													writeBin(NextField::PROPERTY, stream);
													writeBin(desc->hashCode, stream);
													writeBin((uint8_t)j, stream);

													env->serializer->getMapper(prop.type->classId)->write(ptr, stream);
													hasData = true;
													prop.type->copy(ptr, ptr2);
												}
											}
											else {
												buffer->addComponent(id, ptr);
											}

										}
									}
								}
							}

						}
					}
				}

			}
		}

		if (hasData) {
			writeBin(NextField::PACKET_END, stream);
			std::string str = stream.str();
			env->networkManager->sendToAll(str.data(), str.size());
		}
	}

}
