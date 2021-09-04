

#include "core/core.h"

class Module1 : public tri::Module {
public:
	virtual void startup() override {
		env->console->info("module1 startup");
	}
	virtual void update() override {
		//env->console->info("module1 update");
	}
	virtual void shutdown() override {
		env->console->info("module1 shutdown");
	}
};
TRI_MODULE(Module1)

class System1 : public tri::System {
public:
	virtual void startup() override {
		env->console->info("system1 startup");
	}
	virtual void update() override {
		//env->console->info("system1 update");
	}
	virtual void shutdown() override {
		env->console->info("system1 shutdown");
	}
};
TRI_REGISTER_SYSTEM(System1)
