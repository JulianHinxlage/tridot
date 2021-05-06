//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_MESHFACTORY_H
#define TRIDOT_MESHFACTORY_H

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

#endif //TRIDOT_MESHFACTORY_H
