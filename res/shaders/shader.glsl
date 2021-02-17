#type vertex
#version 400 core

layout (location=0) in vec3 vPosition;
layout (location=1) in vec3 vColor;
layout (location=2) in vec2 vTexCoords;

out vec4 fColor;
out vec2 fTexCoords;
uniform mat4 uTransform = mat4(1);
uniform mat4 uProjection = mat4(1);
uniform vec4 uColor = vec4(1, 1, 1, 1);

void main(){
    gl_Position = uProjection * uTransform * vec4(vPosition, 1.0);
    fColor = vec4(vColor, 1) * uColor;
    fTexCoords = vTexCoords;
}

#type fragment
#version 400 core

in vec4 fColor;
in vec2 fTexCoords;
out vec4 oColor;

uniform sampler2D uTexture;

void main(){
    oColor =  texture(uTexture, fTexCoords) * fColor;
}
