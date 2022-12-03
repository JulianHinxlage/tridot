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


				BinaryArchive binaryArchive;
				packet.classArchive = &binaryArchive;
				packet.stringArchive = &conn->readStringArchive;
				binaryArchive.bytesArchive = &packet;
				binaryArchive.stringArchive = &conn->readStringArchive;
				conn->readStringArchive.bytesArchive = &packet;

				Guid guid;
				EntityId id;
				bool ignoreProperty = true;
				while (true) {
					NextField next = PACKET_END;
					packet.readBin(next);

					if (next == PROPERTY) {

						std::string name;
						uint8_t index = 0;
						packet.readStr(name);
						packet.readBin(index);

						bool processedProperty = false;
						for (auto* desc : Reflection::getDescriptors()) {
							if (desc && desc->name == name) {
								if (desc->properties.size() > index) {
									auto& prop = desc->properties[index];

									if (void* comp = env->world->getComponent(id, desc->classId)) {
										void* ptr = (uint8_t*)comp + prop.offset;
										if (!ignoreProperty && (prop.flags & PropertyDescriptor::REPLICATE)) {
											packet.readClass(ptr, prop.type->classId);
										}
										else {
											//todo: cache tmp buffers
											DynamicObjectBuffer tmp;
											tmp.set(prop.type->classId);
											packet.readClass(tmp.get(), prop.type->classId);
										}
									}
									else {
										env->console->log(LogLevel::TRACE, "Network", "property %s index %i dose not exists on entity %s", name.c_str(), index, guid.toString().c_str());
									}
									break;
								}
							}
						}
					}
					else if (next == ENTITY) {
						packet.readBin(guid);
						ignoreProperty = true;

						if (env->networkManager->hasAuthority()) {
							if (env->networkReplication->getOwningConnection(guid) == conn) {
								id = EntityUtil::getEntityByGuid(guid);
								if (id != -1) {
									ignoreProperty = false;
								}
								else {
									env->console->log(LogLevel::TRACE, "Network", "entity with guid %s dose not exist", guid.toString().c_str());
								}
							}
							else {
								env->console->log(LogLevel::TRACE, "Network", "client tries to update not owning entity with guid %s", guid.toString().c_str());
							}
						}
						else {
							if (!env->networkReplication->isOwning(guid)) {
								id = EntityUtil::getEntityByGuid(guid);
								if (id != -1) {
									ignoreProperty = false;
								}
								else {
									env->console->log(LogLevel::TRACE, "Network", "entity with guid %s dose not exist", guid.toString().c_str());
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
				packet.reset();
				env->networkManager->sendToAll(packet, conn);
			}

			packet.classArchive = nullptr;
			packet.stringArchive = nullptr;
			conn->readStringArchive.bytesArchive = nullptr;
		};

	}

	void PropertyReplication::tick() {
		if (env->networkManager->getMode() == CLIENT) {
			replicateToConnection(env->networkManager->getConnection().get());
		}
		else if (env->networkManager->getMode() == SERVER || env->networkManager->getMode() == HOST) {
			for (auto& conn : env->networkManager->getConnections()) {
				replicateToConnection(conn.get());
			}
		}
	}

	void PropertyReplication::replicateToConnection(Connection* conn) {
		Packet packet;
		BinaryArchive binaryArchive;
		packet.classArchive = &binaryArchive;
		packet.stringArchive = &conn->writeStringArchive;
		binaryArchive.bytesArchive = &packet;
		binaryArchive.stringArchive = &conn->writeStringArchive;
		conn->writeStringArchive.bytesArchive = &packet;

		packet.writeBin(NetOpcode::PROPERTY_DATA);
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
					Guid& guid = guids[i];

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
														packet.writeBin(NextField::ENTITY);
														packet.writeBin(guid);
													}

													packet.writeBin(NextField::PROPERTY);
													packet.writeStr(desc->name);
													packet.writeBin((uint8_t)j);

													packet.writeClass(ptr, prop.type->classId);
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
			packet.writeBin(NextField::PACKET_END);
			conn->write(packet);
		}

		conn->writeStringArchive.bytesArchive = nullptr;
	}

}
