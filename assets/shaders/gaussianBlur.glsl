#type vertex
#version 400 core

#include "base/vertexInput.glsl"

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
uniform int steps = 10;
uniform vec2 spread = vec2(1, 0);

out vec4 oColor;

void main(){
    vec2 texelSize = 1.0f / textureSize(uTextures[0], 0);
    vec4 color = vec4(0);
    float sum = 0;
    for(int i = -steps; i <= steps; i++){
        float a = float(i) / float(steps) * 2.0;
        float weight = exp(-0.5 * a * a);
        vec2 uv = fTexCoords.xy + i * spread * texelSize;
        color += texture(uTextures[0], clamp(uv, 0.0f, 0.999f)) * weight;
        sum += weight;
    }
    oColor = color / sum;
}
