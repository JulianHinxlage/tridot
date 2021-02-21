//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "MeshFactory.h"
#include <glm/glm.hpp>

namespace tridot {

    Ref<Mesh> MeshFactory::createQuad() {
        Ref<Mesh> mesh(true);
        float vs[] = {
                -0.5, -0.5, 0, 0.0, 0.0, 1.0, 0.0, 0.0,
                -0.5, +0.5, 0, 0.0, 0.0, 1.0, 0.0, 1.0,
                +0.5, +0.5, 0, 0.0, 0.0, 1.0, 1.0, 1.0,
                +0.5, -0.5, 0, 0.0, 0.0, 1.0, 1.0, 0.0,
        };

        int is[] = {
                0, 1, 2,
                0, 2, 3,
        };
        mesh->create(vs, sizeof(vs) / sizeof(vs[0]), is, sizeof(is) / sizeof(is[0]), {{FLOAT, 3}, {FLOAT, 3}, {FLOAT, 2}});
        return mesh;
    }

    Ref<Mesh> MeshFactory::createCube() {
        Ref<Mesh> mesh(true);
        std::vector<float> vs;
        std::vector<int> is;

        std::vector<float> base = {
                -0.5, -0.5,
                -0.5, +0.5,
                +0.5, +0.5,
                +0.5, -0.5,
        };

        for(int i = 0; i < 6; i++){
            for(int j = 0; j < base.size(); j+= 2){
                float x = base[j+0];
                float y = base[j+1];
                float z = 0.5f;

                float tx = x + 0.5f;
                float ty = y + 0.5f;

                if(i >= 3){
                    ty = 1.0f - ty;
                }

                float factor = (i < 3) ? 1.0f : -1.0f;
                if(i % 3 == 0){
                    std::swap(x, z);
                    x *= factor;
                }else if(i % 3 == 1){
                    std::swap(y, z);
                    y *= factor;
                }else if(i % 3 == 2){
                    z *= factor;
                }

                vs.push_back(x); //x
                vs.push_back(y); //y
                vs.push_back(z); //z

                if(i % 3 == 0){
                    vs.push_back(x); //nx
                    vs.push_back(0); //ny
                    vs.push_back(0); //nz
                }else if(i % 3 == 1){
                    vs.push_back(0); //nx
                    vs.push_back(y); //ny
                    vs.push_back(0); //nz
                }else if(i % 3 == 2){
                    vs.push_back(0); //nx
                    vs.push_back(0); //ny
                    vs.push_back(z); //nz
                }

                vs.push_back(tx * 0.998f + 0.001f); //tx
                vs.push_back(ty * 0.998f + 0.001f); //ty
            }
            is.push_back(0 + i * 4);
            is.push_back(1 + i * 4);
            is.push_back(2 + i * 4);
            is.push_back(0 + i * 4);
            is.push_back(2 + i * 4);
            is.push_back(3 + i * 4);
        }

        mesh->create(vs.data(), vs.size(), is.data(), is.size(), {{FLOAT, 3}, {FLOAT, 3}, {FLOAT, 2}});
        return mesh;
    }

    Ref<Mesh> MeshFactory::createRegularPolygon(int vertexCount) {
        Ref<Mesh> mesh(true);

        std::vector<float> vs;
        std::vector<int> is;


        vs.push_back(0); //x
        vs.push_back(0); //y
        vs.push_back(0); //z

        vs.push_back(0); //nz
        vs.push_back(0); //nz
        vs.push_back(1); //nz

        vs.push_back(0.5); //tx
        vs.push_back(0.5); //ty

        for(int i = 0; i < vertexCount; i++){
            float angle = (360.0f / (float)vertexCount) * (float)i;
            angle = glm::radians(angle);

            float x = cos(angle) * 0.5;
            float y = sin(angle) * 0.5;

            vs.push_back(x); //x
            vs.push_back(y); //y
            vs.push_back(0); //z

            vs.push_back(0); //nz
            vs.push_back(0); //nz
            vs.push_back(1); //nz

            vs.push_back(x + 0.5); //tx
            vs.push_back(y + 0.5); //ty


            if(i == vertexCount - 1){
                is.push_back(0);
                is.push_back(i+1);
                is.push_back(1);
            }else{
                is.push_back(0);
                is.push_back(i + 1);
                is.push_back(i + 2);
            }
        }

        mesh->create(vs.data(), vs.size(), is.data(), is.size(), {{FLOAT, 3}, {FLOAT, 3}, {FLOAT, 2}});
        return mesh;
    }

    Ref<Mesh> MeshFactory::createSphere(int vertexCountX, int vertexCountY) {
        Ref<Mesh> mesh(true);

        std::vector<float> vs;
        std::vector<int> is;


        for(int x = 0;  x < vertexCountX + 1; x++){
            for(int y = 0; y < vertexCountY + 1; y++){
                float angleX = glm::radians(((float)x / (float)vertexCountX) * 360);
                float angleY = glm::radians(((float)y / (float)vertexCountY) * 360);

                float vx = sin(angleX) * cos(angleY) / 2.0f;
                float vy = cos(angleX) / 2.0f;
                float vz = sin(angleX) * sin(angleY) / 2.0f;
                if(x > (vertexCountX + 1) / 2){
                    vx = sin(angleX) * sin(angleY) / 2.0f;
                    vy = cos(angleX) / 2.0f;
                    vz = sin(angleX) * cos(angleY) / 2.0f;
                }

                vs.push_back(vx);
                vs.push_back(vy);
                vs.push_back(vz);

                vs.push_back(vx * 2.0f);
                vs.push_back(vy * 2.0f);
                vs.push_back(vz * 2.0f);

                vs.push_back(glm::degrees(angleX) / 90);
                vs.push_back(glm::degrees(angleY) / 90);
            }
        }

        for(int x = 0;  x < vertexCountX; x++){
            for(int y = 0; y < vertexCountY; y++){
                /*
                int i1 = x * vertexCountY + y;
                int i2 = x * vertexCountY + (y + 1) % vertexCountY;

                int i3 = ((x + 1) % vertexCountX) * vertexCountY + y;
                int i4 = ((x + 1) % vertexCountX) * vertexCountY + (y + 1) % vertexCountY;
                */

                int i1 = x * (vertexCountY + 1) + y;
                int i2 = x * (vertexCountY + 1) + y + 1;

                int i3 = (x + 1) * (vertexCountY + 1) + y;
                int i4 = (x + 1) * (vertexCountY + 1) + y + 1;


                is.push_back(i1);
                is.push_back(i2);
                is.push_back(i3);

                is.push_back(i2);
                is.push_back(i4);
                is.push_back(i3);
            }
        }

        mesh->create(vs.data(), vs.size(), is.data(), is.size(), {{FLOAT, 3}, {FLOAT, 3}, {FLOAT, 2}});
        return mesh;
    }

}