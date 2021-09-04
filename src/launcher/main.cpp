//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "core/core.h"
#include "util/Clock.h"

using namespace tri;

int main(int argc, char* argv[]) {
	Environment::startup();

	env->signals->preStartup.invoke();

	std::string configFile = "../res/config.txt";
	if (argc > 1) {
		configFile = argv[1];
	}
	env->console->loadConfigFile(configFile);

	env->signals->startup.invoke();
	env->signals->postStartup.invoke();

	while (*env->console->getVariable<bool>("running")) {
		env->signals->preUpdate.invoke();
		env->signals->update.invoke();
		env->signals->postUpdate.invoke();
	}

	env->signals->preShutdown.invoke();
	env->signals->shutdown.invoke();
	env->signals->postShutdown.invoke();

	Environment::shutdown();
	return 0;
}
