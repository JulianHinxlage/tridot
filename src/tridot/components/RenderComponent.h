//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_RENDERCOMPONENT_H
#define TRIDOT_RENDERCOMPONENT_H

#include "tridot/render/Mesh.h"
#include "tridot/render/Texture.h"
#include "tridot/render/Shader.h"

namespace tridot {

    class RenderComponent {
    public:
        Ref<Mesh> mesh;
        Ref<Texture> texture;
        Ref<Shader> shader;
        Color color;
        glm::vec2 textureScale;

        RenderComponent(const Color &color = Color::white, const glm::vec2 &textureScale = {1, 1})
            : color(color), textureScale(textureScale) {}

        RenderComponent &setMesh(const Ref<Mesh> &mesh);
        RenderComponent &setTexture(const Ref<Texture> &texture);
        RenderComponent &setShader(const Ref<Shader> &shader);
    };

}

#endif //TRIDOT_RENDERCOMPONENT_H
