//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "render/Material.h"
#include "render/Mesh.h"

namespace tri {

	class MeshComponent {
	public:
		Ref<Mesh> mesh;
		Ref<Material> material;
		Color color;

		MeshComponent(Ref<Mesh> mesh = nullptr, Ref<Material> material = nullptr, Color color = Color::white)
			: mesh(mesh), material(material), color(color) {}
	};

}
