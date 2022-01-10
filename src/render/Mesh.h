//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "VertexArray.h"
#include "engine/Asset.h"
#include <glm/glm.hpp>
#include <string>

namespace tri {

    class Mesh : public Asset {
    public:
        Mesh();

        bool load(const std::string &file) override;
        bool loadActivate() override;
        void create(float *vertices, int vertexCount, int *indices, int indexCount, std::vector<Attribute> layout = {{FLOAT, 3}, {FLOAT, 3}, {FLOAT, 2}});

        const std::vector<float>& getVertexData() { return vertexData; }
        const std::vector<int>& getIndexData() { return indexData; }

        VertexArray vertexArray;
        glm::vec3 boundingMin;
        glm::vec3 boundingMax;
    private:
        std::vector<float> vertexData;
        std::vector<int> indexData;
    };

}

