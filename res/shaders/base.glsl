#type vertex
#version 400 core

layout (location=0) in vec3 vPosition;
layout (location=1) in vec3 vNormal;
layout (location=2) in vec2 vTexCoords;

uniform mat4 uProjection = mat4(1);

out vec2 fTexCoords;

void main(){
    gl_Position = uProjection * vec4(vPosition, 1.0);
    fTexCoords = vTexCoords;
}

#type fragment
#version 400 core

in vec2 fTexCoords;

uniform sampler2D uTextures[32];

out vec4 oColor;

void main(){
    oColor = texture(uTextures[0], fTexCoords);
}
