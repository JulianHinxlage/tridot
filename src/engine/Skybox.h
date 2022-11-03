//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/util/Ref.h"
#include "render/objects/Texture.h"

namespace tri {

    class Skybox {
    public:
        Ref<Texture> texture;
        Color color;
    };

}
