//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "MeshComponent.h"
#include "entity/World.h"
#include "core/core.h"

namespace tri {

    TRI_COMPONENT(MeshComponent);
    TRI_PROPERTIES3(MeshComponent, mesh, material, color);
    TRI_CLASS(Color);

}
