//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "Mesh.h"

namespace tridot {

    class MeshFactory{
    public:
        static Ref<Mesh> createQuad(Ref<Mesh> mesh = nullptr);
        static Ref<Mesh> createCube(Ref<Mesh> mesh = nullptr);
        static Ref<Mesh> createRegularPolygon(int vertexCount, Ref<Mesh> mesh = nullptr);
        static Ref<Mesh> createSphere(int vertexCountX, int vertexCountY, Ref<Mesh> mesh = nullptr);
    };

}

