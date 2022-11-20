//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "EventManager.h"
#include "Environment.h"

namespace tri {

	TRI_SYSTEM_INSTANCE(EventManager, env->eventManager);


	std::vector<EventBase*> &allEvents() {
		static std::vector<EventBase*> events;
		return events;
	}


	EventBase::EventBase() {
		allEvents().push_back(this);
	}

	EventBase::~EventBase() {
		for (int i = 0; i < allEvents().size(); i++) {
			if (allEvents()[i] == this) {
				allEvents().erase(allEvents().begin() + i);
				break;
			}
		}
	}

	void EventBase::removeModuleListeners(const std::string& file) {}

	void EventManager::removeModuleListeners(const std::string& file) {
		for (auto* e : allEvents()) {
			e->removeModuleListeners(file);
		}
	}

}


