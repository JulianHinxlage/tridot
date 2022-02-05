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
uniform int steps = 10;
uniform vec2 spread = vec2(0.001, 0);

out vec4 oColor;
out vec4 oId;

void main(){
    vec4 color = vec4(0);
    for(int i = -steps; i <= steps; i++){
        color += texture(uTextures[0], clamp(fTexCoords.xy + i * spread, 0.0f, 0.999f));
    }
    oColor = color / (steps * 2 + 1);
}
