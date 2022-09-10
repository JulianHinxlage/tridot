//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "ViewFrustum.h"
#include "engine/Transform.h"

namespace tri {

	bool ViewFrustum::inClipSpace(const glm::vec4& p) {
        if (p.x > p.w) { return false; }
        if (p.x < -p.w) { return false; }
        if (p.y > p.w) { return false; }
        if (p.y < -p.w) { return false; }
        if (p.z > p.w) { return false; }
        if (p.z < -p.w) { return false; }
        return true;
	}

    bool ViewFrustum::inFrustumBox(const glm::mat4& transform, const Mesh* mesh) {
        if (inClipSpace(viewProjectionMatrix * transform * glm::vec4(0, 0, 0, 1))) {
            return true;
        }

        for (int i = 0; i < 8; i++) {
            glm::vec3 p;
            p.x = mesh->boundingMin.x + (mesh->boundingMax.x - mesh->boundingMin.x) * (float)((i / 1) % 2);
            p.y = mesh->boundingMin.y + (mesh->boundingMax.y - mesh->boundingMin.y) * (float)((i / 2) % 2);
            p.z = mesh->boundingMin.z + (mesh->boundingMax.z - mesh->boundingMin.z) * (float)((i / 4) % 2);

            if (inClipSpace(viewProjectionMatrix * transform * glm::vec4(p, 1))) {
                return true;
            }
        }

        return false;
    }

    bool ViewFrustum::inFrustumSphere(const glm::mat4& transform, const Mesh* mesh) {
        glm::vec3 size = mesh->boundingMax - mesh->boundingMin;
        float radius = std::max(size.x, std::max(size.y, size.z));

        glm::vec4 center = glm::vec4(0, 0, 0, 1);
        center = viewProjectionMatrix * transform * center;

        glm::vec4 border = glm::vec4(radius, radius, radius, 1);
        border = viewProjectionMatrix * transform * border;

        float ssRadius = glm::length(glm::vec3(center - border));

        if (center.x - ssRadius > center.w) { return false; }
        if (center.x + ssRadius < -center.w) { return false; }
        if (center.y - ssRadius > center.w) { return false; }
        if (center.y + ssRadius < -center.w) { return false; }
        if (center.z - ssRadius > center.w) { return false; }
        if (center.z + ssRadius < -center.w) { return false; }
        return true;
    }

}
