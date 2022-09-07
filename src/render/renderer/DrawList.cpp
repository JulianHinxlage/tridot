//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "DrawList.h"

namespace tri {

    void DrawList::add(const Entry& entry) {
        uint64_t sid = shaderIds.getIndex(entry.shader);
        uint64_t mid = meshIds.getIndex(entry.mesh);

        glm::vec3 position = { entry.transform[3][0], entry.transform[3][1], entry.transform[3][2]};
        int64_t logDistance = (int64_t)glm::log2(glm::dot(position - eyePosition, position - eyePosition) * 1024 * 1024);
        bool opaque = entry.color.a == 255 && entry.material->color.a == 255;

        uint64_t key = 0;
        if (opaque) {
            key |= (uint64_t)1 << 49;
            key |= (sid & 0xffff) << 48;
            key |= (mid & 0xffff) << 32;
            key |= ((uint64_t)logDistance & 0xffffffff) << 0;
        }
        else {
            key |= (uint64_t)0 << 49;
            key |= (sid & 0xffff) << 48;
            key |= (mid & 0xffff) << 32;
            key |= ((uint64_t)(-logDistance - 1) & 0xffffffff) << 0;
        }

        ordering.push_back(entries.size());
        entries.push_back({ key, entry });
    }

    void DrawList::sort() {
        std::sort(ordering.begin(), ordering.end(), [&](auto &a, auto &b) {
            return entries[a].first < entries[b].first;
        });
    }

    void DrawList::submit(RenderBatchList& batches) {
        if (ordering.size() == 0) {
            return;
        }
        RenderBatch* batch = batches.get(entries[ordering[0]].second.shader, entries[ordering[0]].second.mesh);
        for (auto i : ordering) {
            auto& entry = entries[i].second;
            if (batch->mesh != entry.mesh || batch->shader != entry.shader) {
                batch = batches.get(entry.shader, entry.mesh);
            }
            if (batch->isInitialized()) {
                batch->add(entry.transform, entry.material, entry.color, entry.id);
            }
        }
    }

    void DrawList::reset() {
        entries.clear();
        ordering.clear();
        shaderIds.reset();
        meshIds.reset();
    }

}
