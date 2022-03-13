//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "RenderBatch.h"

namespace tri {

    void DrawList::sort() {
        std::sort(keys.begin(), keys.end());
    }

    void DrawList::clear() {
        entries.clear();
        keys.clear();
    }
    
    void DrawList::add(const glm::mat4& transform, const glm::vec3& position, Mesh* mesh, Shader* shader, Material* material, Color color, uint32_t id, int meshIndex, int shaderIndex) {
        Entry &i = entries.emplace_back();
        i.instance.transform = transform;
        i.instance.color = color;
        i.instance.id = id;
        i.instance.materialIndex = -1;
        i.material = material;
        i.shader = shader;
        i.mesh = mesh;
        i.texture = nullptr;

        bool opaque = material->isOpaque() && color.a == 255;
        uint32_t depth = glm::length(eyePosition - position) / 0.0001;
        depth = std::min(depth, (uint32_t)(1 << 24) - 1);

        uint64_t key = 0;
        key |= (uint64_t)!opaque << 57;
        if (opaque) {
            key |= ((uint64_t)meshIndex & 0xffff) << 40;
            key |= ((uint64_t)shaderIndex & 0xffff) << 24;
            key |= ((uint64_t)depth & 0xffffff) << 0;
        }
        else {
            key |= ((uint64_t)-depth & 0xffffff) << 32;
            key |= ((uint64_t)meshIndex & 0xffff) << 16;
            key |= ((uint64_t)shaderIndex & 0xffff) << 0;
        }

        auto& keyEntry = keys.emplace_back();
        keyEntry.entryIndex = entries.size() - 1;
        keyEntry.key = key;
    }

    bool FrustumCulling::checkClipSpace(const glm::vec4& p) {
        if (p.x > p.w) { return false; }
        if (p.x < -p.w) { return false; }
        if (p.y > p.w) { return false; }
        if (p.y < -p.w) { return false; }
        if (p.z > p.w) { return false; }
        if (p.z < -p.w) { return false; }
        return true;
    }

    bool FrustumCulling::checkFrustum(const glm::mat4& transform, Mesh* mesh) {
        bool cull = true;
        glm::vec3 scale = transform * glm::vec4(1, 1, 1, 0);
        if (std::abs(scale.x) > 10) { cull = false; }
        if (std::abs(scale.y) > 10) { cull = false; }
        if (std::abs(scale.z) > 10) { cull = false; }

        if (cull && checkClipSpace(viewProjectionMatrix * transform * glm::vec4(0, 0, 0, 1.0f))) {
            cull = false;
        }
        if (mesh && cull) {
            glm::vec3 min = mesh->boundingMin;
            glm::vec3 max = mesh->boundingMax;
            for (int i = 0; i < 8; i++) {
                float x = i % 2;
                float y = (i / 2) % 2;
                float z = (i / 4) % 2;

                if (checkClipSpace(viewProjectionMatrix * transform * glm::vec4(
                    min.x + x * (max.x - min.x),
                    min.y + y * (max.y - min.y),
                    min.z + z * (max.z - min.z),
                    1.0f))) {
                    cull = false;
                    break;
                }
            }
        }
        return !cull;
    }

    RenderBatch* RenderBatchList::getBatch(Mesh* mesh, Shader* shader) {
        if (batches.size() > shader->getId()) {
            auto& list = batches[shader->getId()];
            if (list.size() > mesh->vertexArray.getId()) {
                RenderBatch* batch = list[mesh->vertexArray.getId()].get();
                if (batch && batch->meshChangeCounter == mesh->changeCounter) {
                    return batch;
                }
            }
            else {
                list.resize(mesh->vertexArray.getId() + 1);
            }
        }
        else {
            batches.resize(shader->getId() + 1);
            auto& list = batches[shader->getId()];
            list.resize(mesh->vertexArray.getId() + 1);
        }

        //create batch
        Ref<RenderBatch> batch = Ref<RenderBatch>::make();
        batchesToRemove.push_back(batches[shader->getId()][mesh->vertexArray.getId()]);
        batches[shader->getId()][mesh->vertexArray.getId()] = batch;
        batch->mesh = mesh;
        batch->meshChangeCounter = mesh->changeCounter;
        batch->shader = shader;
        env->renderThread->addTask([batch, this]() {
            batch->instances = Ref<BatchBuffer>::make();
            batch->instances->init(sizeof(InstanceData));

            batch->materialBuffer = Ref<BatchBuffer>::make();
            batch->materialBuffer->init(sizeof(MaterialData), UNIFORM_BUFFER);

            batch->vertexArray = Ref<VertexArray>::make(batch->mesh->vertexArray);
            batch->vertexArray->addVertexBuffer(batch->instances->buffer, {
                {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4}, //transform
                {UINT8, 4, true}, //color
                {FLOAT, 1}, //material index
                {UINT8, 4, true}, //id
                }, 1);

            batchesToRemove.clear();
        });
        return batch.get();
    }

}
