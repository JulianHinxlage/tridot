//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_RENDERCOMPONENT_H
#define TRIDOT_RENDERCOMPONENT_H

#include "tridot/render/Mesh.h"
#include "tridot/render/Texture.h"
#include "tridot/render/Shader.h"
#include "tridot/render/Material.h"

namespace tridot {

    class RenderComponent {
    public:
        Ref<Mesh> mesh;
        Ref<Material> material;
        Color color;

        RenderComponent(const Color &color = Color::white);

        RenderComponent &setMesh(const Ref<Mesh> &mesh);
        RenderComponent &setMaterial(const Ref<Material> &material);
        RenderComponent &setTexture(const Ref<Texture> &texture);
        RenderComponent &setShader(const Ref<Shader> &shader);
    };

}

#endif //TRIDOT_RENDERCOMPONENT_H
