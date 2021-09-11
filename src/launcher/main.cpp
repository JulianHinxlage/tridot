//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "core/core.h"
#include "entity/Scene.h"
#include "core/util/Clock.h"
#include "render/Window.h"
#include "render/Renderer.h"
#include "engine/Time.h"

using namespace tri;

int main(int argc, char* argv[]) {
	Environment::startup();

	env->signals->preStartup.invoke();
	env->console->setVariable<bool>("vsync", true);

	std::string configFile = "../res/config.txt";
	if (argc > 1) {
		configFile = argv[1];
	}
	env->console->loadConfigFile(configFile);

	env->window->init(1080, 720, "Tridot Launcher");
	env->window->setBackgroundColor(Color(130, 130, 130));

	env->signals->startup.invoke();
	env->signals->postStartup.invoke();


	while (env->window->isOpen()) {
		env->signals->preUpdate.invoke();
		env->signals->update.invoke();
		env->signals->postUpdate.invoke();

		if (env->time->frameTicks(0.5)) {
			env->console->info("fps: ", env->time->framesPerSecond);
		}
		env->window->setVSync(*env->console->getVariable<bool>("vsync"));
	}

	env->signals->preShutdown.invoke();
	env->signals->shutdown.invoke();
	env->signals->postShutdown.invoke();

	Environment::shutdown();
	return 0;
}
