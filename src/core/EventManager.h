//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "System.h"

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
		}

		int addListener(const std::function<void(Args...)> &callback) {
			Listener listener;
			listener.callback = callback;
			listener.id = nextId++;
			listeners.push_back(listener);
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
	};

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
	};

}
