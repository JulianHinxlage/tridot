

#include "core/core.h"

class Module2 : public tri::Module {
public:
	virtual void startup() override {
		env->console->info("module2 startup");
	}
	virtual void update() override {
		//env->console->info("module2 update");
	}
	virtual void shutdown() override {
		env->console->info("module2 shutdown");
	}
};
TRI_MODULE(Module2)
