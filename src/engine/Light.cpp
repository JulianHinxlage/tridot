//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Light.h"
#include "core/core.h"

namespace tri {

	TRI_COMPONENT_CATEGORY(AmbientLight, "Light");
	TRI_PROPERTIES2(AmbientLight, color, intensity);

	TRI_COMPONENT_CATEGORY(DirectionalLight, "Light");
	TRI_PROPERTIES4(DirectionalLight, color, intensity, shadows, shadowMap);

	TRI_COMPONENT_CATEGORY(PointLight, "Light");
	TRI_PROPERTIES4(PointLight, color, intensity, range, falloff);

	TRI_COMPONENT_CATEGORY(SpotLight, "Light");
	TRI_PROPERTIES5(SpotLight, color, intensity, range, falloff, spotAngle);
	TRI_PROPERTY_RANGE(SpotLight, spotAngle, 0, 90);

}
