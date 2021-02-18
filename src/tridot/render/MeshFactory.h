//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_MESHFACTORY_H
#define TRIDOT_MESHFACTORY_H

#include "Mesh.h"

namespace tridot {

    class MeshFactory{
    public:
        static Ref<Mesh> createQuad();
        static Ref<Mesh> createCube();
        static Ref<Mesh> createRegularPolygon(int vertexCount);
    };

}

#endif //TRIDOT_MESHFACTORY_H
