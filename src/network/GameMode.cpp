//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "GameMode.h"
#include "core/core.h"
#include "NetworkManager.h"
#include "NetworkReplication.h"
#include "NetworkComponent.h"
#include "engine/Camera.h"
#include "engine/EntityUtil.h"
#include "engine/EntityInfo.h"

namespace tri {

	TRI_COMPONENT_CATEGORY(GameMode, "Network");
	TRI_PROPERTIES2(GameMode, playerPrefab, localPlayer);

	class GameModeSystem : public System {
	public:

		void startup() {
			env->networkManager->packetCallbacks[NetOpcode::MAP_LOADED] = [&](Connection* conn, NetOpcode opcode, Packet& packet) {
				if (env->networkManager->hasAuthority()) {
					env->eventManager->postTick.addListener([conn, this]() {
						join(conn);
					}, true);
				}
			};
			env->eventManager->onMapBegin.addListener([&](World* world, std::string file) {
				setup();
			});
		}

		void setup() {
			env->world->each<GameMode>([&](GameMode& gameMode) {
				for (int i = 0; i < gameMode.playerPrefab.size(); i++) {
					if (!gameMode.playerPrefab[i]) {
						gameMode.playerPrefab.erase(gameMode.playerPrefab.begin() + i);
						i--;
					}
				}
				if (gameMode.playerPrefab.empty()) {
					for (auto& id : gameMode.localPlayer) {
						Ref<Prefab> prefab = Ref<Prefab>::make();
						prefab->copyEntity(id, env->world, true);
						gameMode.playerPrefab.push_back(prefab);
					}
				}
				if (env->networkManager->getMode() != NetMode::STANDALONE) {
					for (auto& id : gameMode.localPlayer) {
						env->world->removeEntity(id);
					}
					gameMode.localPlayer.clear();
				}
			});

			if (env->networkManager->getMode() != NetMode::STANDALONE) {
				join(nullptr);
			}
		}

		void join(Connection* conn) {
			env->world->each<GameMode>([&](GameMode& gameMode) {
				std::map<EntityId, EntityId> idMap;
				for (auto& prefab : gameMode.playerPrefab) {
					prefab->createEntity(env->world, -1, &idMap);
				}

				for (auto& i : idMap) {
					if (!env->world->getComponentPending<NetworkComponent>(i.second)) {
						env->world->addComponent(i.second, NetworkComponent{false});
					}
				}

				if (conn) {
					for (auto& i : idMap) {
						Guid guid;
						if (auto* info = env->world->getComponentPending<EntityInfo>(i.second)) {
							guid = info->guid;
						}
						env->networkReplication->setOwning(guid, conn);
					}
				}
			});
		}
	};

	TRI_SYSTEM(GameModeSystem);

}
