//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_MESH_H
#define TRIDOT_MESH_H

#include "VertexArray.h"

namespace tridot {

    class Mesh {
    public:
        Mesh();

        bool load(const std::string &file);
        bool preLoad(const std::string &file);
        bool postLoad();
        void create(float *vertices, int vertexCount, int *indices, int indexCount, std::vector<Attribute> layout = {{FLOAT, 3}, {FLOAT, 3}, {FLOAT, 2}});

        VertexArray vertexArray;
        bool rescale;
    private:
        std::vector<float> vertexData;
        std::vector<int> indexData;
    };

}

#endif //TRIDOT_MESH_H
