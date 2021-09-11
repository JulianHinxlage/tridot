//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Mesh.h"
#include "core/core.h"
#include <fstream>
#include <map>
#include <algorithm>

namespace tri {

    Mesh::Mesh() {
        boundingMin = {-0.5, -0.5, -0.5};
        boundingMax = {+0.5, +0.5, +0.5};
    }

    bool Mesh::load(const std::string &file) {
        if(!preLoad(file)){
            return false;
        }
        return postLoad();
    }

    bool Mesh::postLoad() {
        create(vertexData.data(), vertexData.size(), indexData.data(), indexData.size(), {{FLOAT, 3}, {FLOAT, 3}, {FLOAT, 2}});
        return true;
    }

    void Mesh::create(float *vertices, int vertexCount, int *indices, int indexCount, std::vector<Attribute> layout) {
        vertexArray.clear();

        Ref<Buffer> vertexBuffer(true);
        Ref<Buffer> indexBuffer(true);

        int stride = 0;
        for(auto &a : layout){
            a.offset = stride;
            stride += a.size * a.count;
        }

        vertexBuffer->init(vertices, vertexCount * sizeof(vertices[0]), stride, VERTEX_BUFFER, false);
        indexBuffer->init(indices, indexCount * sizeof(indices[0]), sizeof(indices[0]), INDEX_BUFFER, false);

        vertexArray.addIndexBuffer(indexBuffer, UINT32);
        vertexArray.addVertexBuffer(vertexBuffer, layout);
    }

    std::vector<std::string> split(const std::string &str, char delimiter = ' '){
        std::vector<std::string> result;
        result.push_back("");
        for(char c : str){
            if(c == delimiter){
                if(result.back() != ""){
                    result.push_back("");
                }
            }else{
                result.back() += c;
            }
        }
        if(result.back() == ""){
            result.pop_back();
        }
        return result;
    }

    bool Mesh::preLoad(const std::string &file) {
        std::ifstream stream;
        stream.open(file);

        std::vector<float> vs;
        std::vector<float> ns;
        std::vector<float> ts;

        struct Index{
            int v = -1;
            int n = -1;
            int t = -1;

            bool operator<(const Index &i) const{
                if(v != i.v){
                    return v < i.v;
                }
                if(n != i.n){
                    return n < i.n;
                }
                if(t != i.t){
                    return t < i.t;
                }
                return false;
            }
        };
        std::vector<Index> is;

        if(stream.is_open()){
            std::string line;
            while(std::getline(stream, line)){
                std::vector<std::string> parts = split(line, ' ');
                if (parts.size() > 0) {
                    if (parts[0] == "v") {
                        for (int i = 0; i < 3; i++) {
                            if (parts.size() > i + 1) {
                                try {
                                    vs.push_back(std::stof(parts[i + 1]));
                                }
                                catch (...) {}
                            }
                            else {
                                vs.push_back(0);
                            }
                        }
                    }
                    else if (parts[0] == "vn") {
                        for (int i = 0; i < 3; i++) {
                            if (parts.size() > i + 1) {
                                try {
                                    ns.push_back(std::stof(parts[i + 1]));
                                }
                                catch (...) {}
                            }
                            else {
                                vs.push_back(0);
                            }
                        }
                    }
                    else if (parts[0] == "vt") {
                        for (int i = 0; i < 3; i++) {
                            if (parts.size() > i + 1) {
                                try {
                                    ts.push_back(std::stof(parts[i + 1]));
                                }
                                catch (...) {}
                            }
                            else {
                                vs.push_back(0);
                            }
                        }
                    }
                    else if (parts[0] == "f") {
                        for (int i = 1; i < parts.size(); i++) {
                            std::vector<std::string> parts2 = split(parts[i], '/');

                            is.push_back({});
                            if (parts2.size() > 0) {
                                try {
                                    is.back().v = std::stof(parts2[0]) - 1;
                                }
                                catch (...) {}
                            }
                            if (parts2.size() > 1) {
                                try {
                                    is.back().t = std::stof(parts2[1]) - 1;
                                }
                                catch (...) {}
                            }
                            if (parts2.size() > 2) {
                                try {
                                    is.back().n = std::stof(parts2[2]) - 1;
                                }
                                catch (...) {}
                            }
                            else {
                                is.back().n = is.back().v;
                            }
                        }
                    }
                }
            }

            if(vs.size() == 0 && is.size() == 0){
                return false;
            }

            std::map<Index, int> map;

            indexData.clear();
            vertexData.clear();

            boundingMin = {0, 0, 0};
            boundingMax = {0, 0, 0};

            for(auto &i : is){
                auto entry = map.find(i);
                int index = 0;
                if(entry == map.end()){
                    index = map.size();
                    map[i] = index;

                    if(i.v != -1 && vs.size() > i.v * 3 + 2){

                        float x = vs[i.v * 3 + 0];
                        float y = vs[i.v * 3 + 1];
                        float z = vs[i.v * 3 + 2];

                        if(index == 0){
                            boundingMin = {x, y, z};
                            boundingMax = {x, y, z};
                        }

                        boundingMin.x = std::min(x, boundingMin.x);
                        boundingMin.y = std::min(y, boundingMin.y);
                        boundingMin.z = std::min(z, boundingMin.z);

                        boundingMax.x = std::max(x, boundingMax.x);
                        boundingMax.y = std::max(y, boundingMax.y);
                        boundingMax.z = std::max(z, boundingMax.z);

                        vertexData.push_back(x);
                        vertexData.push_back(y);
                        vertexData.push_back(z);
                    }else{
                        vertexData.push_back(0);
                        vertexData.push_back(0);
                        vertexData.push_back(0);
                    }


                    if(i.n != -1 && ns.size() > i.n * 3 + 2){
                        vertexData.push_back(ns[i.n * 3 + 0]);
                        vertexData.push_back(ns[i.n * 3 + 1]);
                        vertexData.push_back(ns[i.n * 3 + 2]);
                    }else{
                        vertexData.push_back(0);
                        vertexData.push_back(0);
                        vertexData.push_back(0);
                    }

                    if(i.t != -1 && ts.size() > i.t * 2 + 1){
                        vertexData.push_back(ts[i.t * 2 + 0]);
                        vertexData.push_back(ts[i.t * 2 + 1]);
                    }else{
                        vertexData.push_back(0);
                        vertexData.push_back(0);
                    }

                }else{
                    index = entry->second;
                }
                indexData.push_back(index);
            }

            env->console->debug("loaded mesh ", file);
            return true;
        }else{
            env->console->warning("mesh: file ", file, " not found");
        }
        return false;
    }

}
