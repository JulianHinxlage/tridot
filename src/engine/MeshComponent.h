//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "render/objects/Material.h"
#include "render/objects/Mesh.h"

namespace tri {

    class Mesh;
    class Material;

    class MeshComponent {
    public:
        Ref<Mesh> mesh;
        Ref<Material> material;
        Color color;

        MeshComponent(Ref<Mesh> mesh = nullptr, Ref<Material> material = nullptr, Color color = color::white)
            : mesh(mesh), material(material), color(color) {}
    };

}
