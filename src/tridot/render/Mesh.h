//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_MESH_H
#define TRIDOT_MESH_H

#include "VertexArray.h"
#include <glm/glm.hpp>
#include <string>

namespace tridot {

    class Mesh {
    public:
        Mesh();

        bool load(const std::string &file);
        bool preLoad(const std::string &file);
        bool postLoad();
        void create(float *vertices, int vertexCount, int *indices, int indexCount, std::vector<Attribute> layout = {{FLOAT, 3}, {FLOAT, 3}, {FLOAT, 2}});

        VertexArray vertexArray;
        glm::vec3 boundingMin;
        glm::vec3 boundingMax;
    private:
        std::vector<float> vertexData;
        std::vector<int> indexData;
    };

}

#endif //TRIDOT_MESH_H
