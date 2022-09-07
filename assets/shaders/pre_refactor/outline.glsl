#type vertex
#version 400 core

layout (location=0) in vec3 vPosition;
layout (location=1) in vec3 vNormal;
layout (location=2) in vec2 vTexCoords;

uniform mat4 uProjection = mat4(1);
uniform mat4 uTransform = mat4(1);

out vec2 fTexCoords;

void main(){
    gl_Position = uProjection * uTransform * vec4(vPosition, 1.0);
    fTexCoords = vTexCoords;
}

#type fragment
#version 400 core

in vec2 fTexCoords;

uniform sampler2D uTextures[32];
uniform vec4 uColor = vec4(1);
uniform int steps = 2;

out vec4 oColor;
out vec4 oId;

void main() {
    vec4 color;
    vec2 texSize = textureSize(uTextures[0], 0);
    for(int i = -steps; i <= steps; i++){
        for(int j = -steps; j <= steps; j++){
            color += texture(uTextures[0], clamp(fTexCoords.xy + vec2(i, j) * (1.0 / texSize), 0.0f, 0.999f));
        }
    }
    color /= (steps * 2 + 1) * (steps * 2 + 1);
    oColor = vec4(uColor.xyz, color.a == 1 ? 0 : (color.a == 0 ? 0 : 1 ));
}
