#include "core/core.h"

class ExampleComponent {
public:
	float x = 0;
	float y = 0;
	float z = 0;
};

TRI_COMPONENT(ExampleComponent);
TRI_PROPERTIES3(ExampleComponent, x, y, z);


class ExampleSystem : public tri::System {
public:
	virtual void startup() {
		env->console->info("TemplateSystem::startup()");
	}
	virtual void shutdown() {
		env->console->info("TemplateSystem::shutdown()");
	}
};

TRI_SYSTEM(ExampleSystem);

