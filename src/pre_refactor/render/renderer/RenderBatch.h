//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/Environment.h"
#include "core/util/Ref.h"
#include "core/System.h"
#include "render/FrameBuffer.h"
#include "render/Mesh.h"
#include "render/Texture.h"
#include "render/Material.h"
#include "render/Light.h"
#include "render/RenderPass.h"
#include "render/BatchBuffer.h"
#include "render/RenderThread.h"
#include "ShaderStructs.h"

namespace tri {

    //map asstes to an index and keep a list off the assets
    template<typename T>
    class AssetList {
    public:
        std::vector<T*> assets;
        std::unordered_map<T*, int> indexMap;

        int getIndex(T* asset) {
            if (asset == nullptr) {
                return -1;
            }
            auto x = indexMap.find(asset);
            if (x != indexMap.end()) {
                return x->second;
            }
            assets.push_back(asset);
            indexMap[asset] = assets.size() - 1;
            return assets.size() - 1;
        }

        void reset() {
            indexMap.clear();
            assets.clear();
        }
    };

    //collection of all instances with the same shader mesh combination
    //includes buffers for instances, textures and materials
    class RenderBatch {
    public:
        Mesh* mesh = nullptr;
        int meshChangeCounter = 0;
        Shader* shader = nullptr;
        Ref<VertexArray> vertexArray;
        Ref<BatchBuffer> instances;
        AssetList<Texture> textures;
        AssetList<Material> materials;
        Ref<BatchBuffer> materialBuffer;
        int instanceCount = 0;
    };

    class FrustumCulling {
    public:
        glm::mat4 viewProjectionMatrix;
        bool checkClipSpace(const glm::vec4& p);
        bool checkFrustum(const glm::mat4& transform, Mesh* mesh);
    };

    //a list of instance submit calls to be sorted
    class DrawList {
    public:
        class Entry {
        public:
            InstanceData instance;
            Material* material;
            Texture* texture;
            Shader* shader;
            Mesh* mesh;
        };

        class Key {
        public:
            uint64_t key;
            int entryIndex;

            bool operator<(const Key& key) {
                return this->key < key.key;
            }
        };

        std::vector<Entry> entries;
        std::vector<Key> keys;

        glm::vec3 eyePosition;

        void sort();
        void clear();
        void add(const glm::mat4& transform, const glm::vec3& position, Mesh* mesh, Shader* shader, Material* material, Color color, uint32_t id, int meshIndex = 0, int shaderIndex = 0);
    };

    //a list of batches for all shader mesh combinations
    class RenderBatchList {
    public:
        AssetList<Shader> shaders;
        AssetList<Mesh> meshes;
        std::vector<std::vector<Ref<RenderBatch>>> batches;
        std::vector<Ref<RenderBatch>> batchesToRemove;

        RenderBatch* getBatch(Mesh* mesh, Shader* shader);
    };

}
