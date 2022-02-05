#type vertex
#version 400 core

layout (location=0) in vec3 vPosition;
layout (location=1) in vec3 vNormal;
layout (location=2) in vec2 vTexCoords;

uniform mat4 uProjection = mat4(1);
uniform mat4 uTransform = mat4(1);

out vec2 fTexCoords;
out vec3 fPosition;

void main(){
    gl_Position = uProjection * uTransform * vec4(normalize(vPosition), 1.0);
    fPosition = vPosition;
    fTexCoords = vTexCoords;
}

#type fragment
#version 400 core

in vec2 fTexCoords;
in vec3 fPosition;

uniform samplerCube uTextures[32];
uniform vec4 uColor = vec4(1);

out vec4 oColor;

void main(){
    vec3 v = fPosition;
    oColor = texture(uTextures[0], vec3(v.x, -v.z, v.y)) * uColor;
}
