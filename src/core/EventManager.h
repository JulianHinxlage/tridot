//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "config.h"
#include "System.h"
#include "Reflection.h"

namespace tri {

	template<typename... Args>
	class Event {
	public:
		void invoke(Args... args) {
			for (auto& listener : listeners) {
				if (listener.callback) {
					listener.callback(args...);
				}
			}
			for (auto& listener : invokeOnceListeners) {
				if (listener.callback) {
					listener.callback(args...);
				}
			}
			invokeOnceListeners.clear();
		}

		int addListener(const std::function<void(Args...)> &callback, bool invokeOnlyOnce = false) {
			Listener listener;
			listener.callback = callback;
			listener.id = nextId++;
			if (invokeOnlyOnce) {
				invokeOnceListeners.push_back(listener);
			}
			else {
				listeners.push_back(listener);
			}
			return listener.id;
		}

		void removeListener(int id) {
			for (int i = 0; i < listeners.size(); i++) {
				if (listeners[i].id == id) {
					listeners.erase(listeners.begin() + i);
					break;
				}
			}
		}

	private:
		class Listener {
		public:
			std::function<void(Args...)> callback;
			int id;
		};
		int nextId = 0;
		std::vector<Listener> listeners;
		std::vector<Listener> invokeOnceListeners;
	};

	class World;

	class EventManager : public System {
	public:
		Event<> preStartup;
		Event<> startup;
		Event<> postStartup;
		
		Event<> preTick;
		Event<> tick;
		Event<> postTick;

		Event<> preShutdown;
		Event<> shutdown;
		Event<> postShutdown;

		Event<std::string> onModuleLoad;
		Event<std::string> onModuleUnload;
		
		Event<int> onClassRegister;
		Event<int> onClassUnregister;


		Event<World*, EntityId> onEntityAdd;
		Event<World*, EntityId> onEntityRemove;

		template<typename T>
		Event<World*, EntityId>& onComponentAdd() {
			return onComponentAdd(Reflection::getClassId<T>());
		}
		
		template<typename T>
		Event<World*, EntityId>& onComponentRemove() {
			return onComponentRemove(Reflection::getClassId<T>());
		}

		Event<World*, EntityId>& onComponentAdd(int classId) {
			if (classId >= onComponentAddEvents.size()) {
				onComponentAddEvents.resize(classId + 1);
			}
			return onComponentAddEvents[classId];
		}

		Event<World*, EntityId>& onComponentRemove(int classId) {
			if (classId >= onComponentRemoveEvents.size()) {
				onComponentRemoveEvents.resize(classId + 1);
			}
			return onComponentRemoveEvents[classId];
		}
	private:
		std::vector<Event<World*, EntityId>> onComponentAddEvents;
		std::vector<Event<World*, EntityId>> onComponentRemoveEvents;

	};

}


#define TRI_EVENT_IMPL(func, event) void func(); static tri::impl::GlobalInitializationCallback TRI_UNIQUE_IDENTIFIER(init)([](){ env->eventManager->event.addListener(&func); }); void func()

#define TRI_PRE_TICK() TRI_EVENT_IMPL(TRI_UNIQUE_IDENTIFIER(preTick), preTick)
#define TRI_TICK() TRI_EVENT_IMPL(TRI_UNIQUE_IDENTIFIER(tick), tick)
#define TRI_POST_TICK() TRI_EVENT_IMPL(TRI_UNIQUE_IDENTIFIER(postTick), postTick)

#define TRI_PRE_STARTUP() TRI_EVENT_IMPL(TRI_UNIQUE_IDENTIFIER(preStartup), preStartup)
#define TRI_STARTUP() TRI_EVENT_IMPL(TRI_UNIQUE_IDENTIFIER(startup), startup)
#define TRI_POS_TSTARTUP() TRI_EVENT_IMPL(TRI_UNIQUE_IDENTIFIER(postStartup), postStartup)

#define TRI_PRE_SHUTDOWN() TRI_EVENT_IMPL(TRI_UNIQUE_IDENTIFIER(preShutdown), preShutdown)
#define TRI_SHUTDOWN() TRI_EVENT_IMPL(TRI_UNIQUE_IDENTIFIER(shutdown), shutdown)
#define TRI_POST_SHUTDOWN() TRI_EVENT_IMPL(TRI_UNIQUE_IDENTIFIER(postShutdown), postShutdown)
