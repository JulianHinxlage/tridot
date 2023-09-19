//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#include "core/core.h"
#include "engine/Transform.h"
#include "entity/World.h"
#include "engine/Time.h"

class ExampleComponent {
public:
	float rotationSpeed;
};
TRI_COMPONENT_CATEGORY(ExampleComponent, "Template");
TRI_PROPERTIES1(ExampleComponent, rotationSpeed);


class ExampleSystem : public tri::System {
public:
	void tick() override {
		env->world->each<ExampleComponent, tri::Transform>([](EntityId id, ExampleComponent &comp, tri::Transform &transform) {
			transform.rotation.x += comp.rotationSpeed * env->time->deltaTime;
		});
	}
};
TRI_SYSTEM(ExampleSystem);