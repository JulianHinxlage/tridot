//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Reflection.h"
#include "Console.h"
#include "ModuleManager.h"
#include "EventManager.h"

namespace tri {

	std::vector<ClassDescriptor*>& Reflection::getDescriptorsImpl() {
		static std::vector<ClassDescriptor*> descriptors;
		return descriptors;
	}

	std::map<std::string, ClassDescriptor*>& Reflection::getDescriptorsByNameImpl() {
		static std::map<std::string, ClassDescriptor*> descriptorsByName;
		return descriptorsByName;
	}

	void Reflection::onClassRegister(int classId) {
		getEnvironment()->systemManager->addSystem<EventManager>()->onClassRegister.invoke(classId);
	}
	
	void Reflection::onClassUnregister(int classId) {
		if (env && env->eventManager) {
			env->eventManager->onClassUnregister.invoke(classId);
		}
	}

	void Reflection::handleDuplicatedClass(ClassDescriptor* desc, void* address1, void* address2) {
		env->console->fatal("the same class \"%s\" was registerd in multiple modules: %s and %s", 
			desc->name.c_str(), 
			ModuleManager::getModuleNameByAddress(address1).c_str(),
			ModuleManager::getModuleNameByAddress(address2).c_str()
		);
	}


}
