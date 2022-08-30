//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Light.h"
#include "core/core.h"

namespace tri {

	TRI_CLASS(Light::Type);
	TRI_ENUM4(Light::Type, AMBIENT_LIGHT, DIRECTIONAL_LIGHT, POINT_LIGHT, SPOT_LIGHT);

	TRI_COMPONENT(Light);
	TRI_PROPERTIES6(Light, type, color, intensity, range, falloff, spotAngle);

	Light::Light() {
		type = AMBIENT_LIGHT;
		color = color::white;
		intensity = 0.5;
		range = 5;
		falloff = 2;
		spotAngle = 15;
	}

}
