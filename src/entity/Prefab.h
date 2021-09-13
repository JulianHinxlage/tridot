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
		Prefab(const Components&... comps) {
			addComponents(comps...);
		}

		template<typename... Components>
		void addComponents(const Components&... comps) {
			((addComponent<Components>(comps)), ...);
		}

		template<typename Component, typename... Args>
		Component& addComponent(const Args&... args) {
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
			if (void* data = getComponent(typeId)) {
				return data;
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
				void *ptr = scene->addComponent(comp.typeId, id);
				for (int j = 0; j < comp.data.size(); j++) {
					*((uint8_t*)ptr + j) = comp.data[j];
				}
			}
			//todo: childs
			return id;
		}

		void copyEntity(EntityId id, Scene *scene){
            clear();
            for(auto &desc : env->reflection->getDescriptors()){
                if(scene->hasComponent(desc->typeId, id)){
                    desc->copy(scene->getComponent(desc->typeId, id), addComponent(desc->typeId));
                }
            }
            //todo: childs
		}

		void operator=(const Prefab& prefab) {
			comps = prefab.comps;
			childs.resize(prefab.childs.size());
			for (int i = 0; i < childs.size(); i++) {
				childs[i] = std::make_shared<Prefab>(*prefab.childs[i]);
			}
		}

		void clear(){
		    comps.clear();
		    childs.clear();
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
