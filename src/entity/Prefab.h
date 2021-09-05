//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once
#include "pch.h"
#include "core/core.h"
#include "Scene.h"

namespace tri {

	class Prefab {
	public:
		Prefab() {}

		Prefab(const Prefab& prefab) {
			operator=(prefab);
		}

		template<typename... Components>
		Prefab(const Components&... &comps) {
			addComponents(comps...);
		}

		template<typename... Components>
		void addComponents(const Components&... comps) {
			((addComponent<Components>(comps)), ...);
		}

		template<typename Component, typename... Args>
		Component& addComponent(const ARgs&... args) {
			Component *comp = (Component*)addComponent(env->reflection->getTypeId<Component>());
			new (comp) Component(args...);
			return *comp;
		}

		template<typename Component>
		Component& getComponent() {
			return *(Component*)getComponent(env->reflection->getTypeId<Component>());
		}

		template<typename Component>
		bool hasComponent() {
			return hasComponent(env->reflection->getTypeId<Component>());
		}
		
		template<typename Component>
		bool removeComponent() {
			return removeComponent(env->reflection->getTypeId<Component>());
		}

		void *addComponent(int typeId) {
			if (void* comp = getComponent()) {
				return comp;
			}
			else {
				comps.push_back({ typeId });
				Comp& comp = comps.back();
				comp.data.resize(env->reflection->getType(typeId)->size);
				return comp.data.data();
			}
		}

		void *getComponent(int typeId) {
			for (int i = 0; i < comps.size(); i++) {
				if (comps[i].typeId == typeId) {
					return comps[i].data.data();
				}
			}
			return nullptr;
		}

		bool hasComponent(int typeId) {
			return getComponent(typeId) != nullptr;
		}

		bool removeComponent(int typeId) {
			for (int i = 0; i < comps.size(); i++) {
				if (comps[i].typeId == typeId) {
					comps.erase(comps.begin() + i);
					return true;
				}
			}
			return false;
		}

		template<typename... Components>
		Prefab& addChild(const Components&... comps) {
			childs.push_back(std::make_shared<Prefab>(comps...));
			return *childs.back().get();
		}

		EntityId createEntity(Scene* scene) {
			EntityId id = scene->addEntity();
			for (int i = 0; i < comps.size(); i++) {
				Comp& comp = comps[i];
				void *ptr = scene->addComponent(comp.typeId);
				for (int j = 0; j < comp.data.size(); i++) {
					*((uint8_t*)ptr + j) = comp.data[j];
				}
			}
			return id;
		}

		void operator=(const Prefab& prefab) {
			comps = prefab.comps;
			childs.resize(prefabs.size());
			for (int i = 0; i < childs.size(); i++) {
				childs[i] = std::make_shared<Prefab>(*prefab.childs[i]);
			}
		}

	private:
		class Comp {
		public:
			int typeId;
			std::vector<uint8_t> data;
		};
		std::vector<Comp> comps;
		std::vector<std::shared_ptr<Prefab>> childs;
	};

}
