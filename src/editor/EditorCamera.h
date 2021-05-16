//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_EDITORCAMERA_H
#define TRIDOT_EDITORCAMERA_H

#include "tridot/render/Camera.h"
#include "tridot/components/Transform.h"

namespace tridot {

    class EditorCamera {
    public:
        float speed = 1;
        glm::vec2 startMousePosition = {0, 0};
        bool dragMiddle = false;
        bool dragRight = false;

        void update(PerspectiveCamera &camera, Transform &transform, bool hovered, bool useKeyboard = true);
    };

}

#endif //TRIDOT_EDITORCAMERA_H
