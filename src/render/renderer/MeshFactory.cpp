//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "MeshFactory.h"

namespace tri {

	TRI_SYSTEM(MeshFactory);

	Ref<Mesh> MeshFactory::generateQuad(float width, float height, glm::vec3 normal) {
        Ref<Mesh> mesh = Ref<Mesh>::make();
        float quadVertices[] = {
            -0.5, +0.0, -0.5, 0.0, 1.0, 0.0, 0.0, 0.0,
            +0.5, +0.0, -0.5, 0.0, 1.0, 0.0, 1.0, 0.0,
            +0.5, +0.0, +0.5, 0.0, 1.0, 0.0, 1.0, 1.0,
            -0.5, +0.0, +0.5, 0.0, 1.0, 0.0, 0.0, 1.0,
        };
        int quadIndices[] = {
            0, 2, 1,
            0, 3, 2,
        };

        for (int i = 0; i < 8 * 4; i += 8) {
            glm::vec3* pos = (glm::vec3*)(&quadVertices[i]);
            pos->x *= width;
            pos->z *= height;
        }

        mesh->create(quadVertices, sizeof(quadVertices) / sizeof(quadVertices[0]), quadIndices, sizeof(quadIndices) / sizeof(quadIndices[0]), { {FLOAT, 3}, {FLOAT, 3} ,{FLOAT, 2} });
        return mesh;
	}

    Ref<Mesh> MeshFactory::generateRegularPoligon(int vertexCount, glm::vec3 normal) {
        Ref<Mesh> mesh = Ref<Mesh>::make();
        return mesh;
    }

    Ref<Mesh> MeshFactory::generateCube(float width, float height, float depth) {
        Ref<Mesh> mesh = Ref<Mesh>::make();
        return mesh; 
    }

    Ref<Mesh> MeshFactory::generateCubeSphere(int faceSections) {
        Ref<Mesh> mesh = Ref<Mesh>::make();

        glm::vec3 faceCenters[6] = {
            { 1,  0,  0},
            {-1,  0,  0},
            { 0,  1,  0},
            { 0, -1,  0},
            { 0,  0,  1},
            { 0,  0, -1},
        };
        glm::vec3 faceX[6] = {
            { 0,  1,  0},
            { 0,  1,  0},
            { 0,  0,  1},
            { 0,  0,  1},
            { 1,  0,  0},
            { 1,  0,  0},
        };
        glm::vec3 faceY[6] = {
            { 0,  0,  1},
            { 0,  0, -1},
            { 1,  0,  0},
            {-1,  0,  0},
            { 0,  1,  0},
            { 0, -1,  0},
        };

        std::vector<float> vs;

        auto add = [&](glm::vec3 v) {
            vs.push_back(v.x);
            vs.push_back(v.y);
            vs.push_back(v.z);
            for (int i = 0; i < 5; i++) {
                vs.push_back(0);
            }
        };

        for (int face = 0; face < 6; face++) {
            glm::vec3 c = faceCenters[face];
            glm::vec3 x = faceX[face];
            glm::vec3 y = faceY[face];

            for (int i = 0; i < faceSections; i++) {
                for (int j = 0; j < faceSections; j++) {
                    glm::vec3 v0 = c * 0.5f - x * 0.5f - y * 0.5f;
                    v0 += x / (float)faceSections * (float)i;
                    v0 += y / (float)faceSections * (float)j;

                    glm::vec3 v1 = v0 + x / (float)faceSections;
                    glm::vec3 v2 = v0 + x / (float)faceSections + y / (float)faceSections;
                    glm::vec3 v3 = v0 + y / (float)faceSections;

                    add(v0);
                    add(v1);
                    add(v3);

                    add(v1);
                    add(v2);
                    add(v3);
                }
            }
        }

        std::vector<int> is;
        for (int i = 0; i < vs.size(); i += 8) {
            glm::vec3 v = { vs[i + 0], vs[i + 1], vs[i + 2] };
            v = glm::normalize(v);

            vs[i + 0] = v.x;
            vs[i + 1] = v.y;
            vs[i + 2] = v.z;

            vs[i + 3] = v.x;
            vs[i + 4] = v.y;
            vs[i + 5] = v.z;

            is.push_back(i / 8);
        }

        mesh = Ref<Mesh>::make();
        mesh->create(vs.data(), vs.size(), is.data(), is.size(), { {FLOAT, 3}, {FLOAT, 3} ,{FLOAT, 2} });
        return mesh;
    }

}
