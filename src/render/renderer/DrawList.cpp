//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "DrawList.h"

namespace tri {

    void DrawList::add(const Entry& entry) {
        uint64_t key = 0;
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
    }

}
