//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "core/core.h"
#include "render/Window.h"

void printDescriptor(const tri::Reflection::TypeDescriptor* desc) {
	if (desc) {
		env->console->info("type ", desc->name, ": size = ", desc->size, ", typeId = ", desc->typeId);
		for (auto& m : desc->member) {
			env->console->info(" member ", m.name, ": type = ", m.type->name, ", offset = ", m.offset);
		}
		for (auto& c : desc->constants) {
			env->console->info(" constant: ", c.name, " = ", c.value);
		}
	}
}

TRI_REGISTER_CALLBACK() {
	env->console->addCommand("module_load", [](const std::vector<std::string>& args) {
		if (args.size() > 1) {
			env->modules->loadModule(args[1]);
		}
		else {
			env->console->info("usage: module_load <module>");
		}
	});

	env->console->addCommand("module_unload", [](const std::vector<std::string>& args) {
		if (args.size() > 1) {
			env->modules->unloadModule(env->modules->getModule(args[1]));
		}
		else {
			env->console->info("usage: module_unload <module>");
		}
	});

	env->console->setVariable<bool>("running", true);
	env->console->addCommand("exit", []() {
		env->console->setVariable<bool>("running", false);
		env->window->close();
	});
	env->console->addCommand("quit", []() {
		env->console->setVariable<bool>("running", false);
		env->window->close();
	});

	env->console->addCommand("print_types", []() {
		for (auto& desc : env->reflection->getDescriptors()) {
			if (desc) {
				printDescriptor(desc.get());
			}
		}
	});
	env->console->addCommand("print_callbacks", [](const std::vector<std::string>& args) {
		if (args.size() > 1) {
			for (auto& o : env->signals->getSignal<>(args[1]).getObservers()) {
				env->console->info("callback: ", o.name);
			}
		}
		else {
			env->console->info("usage: print_callbacks <signal>");
		}
	});

	env->console->addCommand("signal_activate", [](const std::vector<std::string>& args) {
		if (args.size() >= 3) {
			std::vector<std::string> callbacks(args.begin() + 2, args.end());
			env->signals->getSignal<>(args[1]).setActiveCallbacks(callbacks, true);
		}
		else {
			env->console->info("usage: signal_activate <signal> <callbacks>");
		}
	});
	env->console->addCommand("signal_deactivate", [](const std::vector<std::string>& args) {
		if (args.size() >= 3) {
			std::vector<std::string> callbacks(args.begin() + 2, args.end());
			env->signals->getSignal<>(args[1]).setActiveCallbacks(callbacks, false);
		}
		else {
			env->console->info("usage: signal_deactivate <signal> <callbacks>");
		}
	});
	env->console->addCommand("signal_order", [](const std::vector<std::string>& args) {
		if (args.size() >= 3) {
			std::vector<std::string> callbacks(args.begin() + 2, args.end());
			env->signals->getSignal<>(args[1]).callbackOrder(callbacks);
		}
		else {
			env->console->info("usage: signal_order <signal> <callbacks>");
		}
	});
}

TRI_REGISTER_CALLBACK() {
	env->signals->preStartup.addCallback("version", []() {
#if TRI_WINDOWS
		env->console->options.color = false;
#endif
		env->console->info("Tridot version ", TRI_VERSION);
	});
}
