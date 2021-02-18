#type vertex
#version 400 core

layout (location=0) in vec3 vPosition;
layout (location=1) in vec3 vNormal;
layout (location=2) in vec2 vTexCoords;

out vec4 fColor;
out vec2 fTexCoords;
out vec3 fNormal;
uniform mat4 uTransform = mat4(1);
uniform mat4 uProjection = mat4(1);
uniform vec4 uColor = vec4(1, 1, 1, 1);

void main(){
    gl_Position = uProjection * uTransform * vec4(vPosition, 1.0);
    fColor = uColor;
    fTexCoords = vTexCoords;
    fNormal = vNormal;
}

#type fragment
#version 400 core

in vec4 fColor;
in vec3 fNormal;
in vec2 fTexCoords;
out vec4 oColor;

uniform sampler2D uTexture;
uniform vec3 uAmbientLightDirection = vec3(1, 0.7, 0.5);
uniform float uAmbientLightStrenght = 0.6;

void main(){
    oColor =  texture(uTexture, fTexCoords) * fColor;

    if(fNormal != vec3(0, 0, 0)){
        float theta = dot(normalize(fNormal), -normalize(uAmbientLightDirection));
        float intensity = (theta / 2.0 + 0.5) * (1.0 - uAmbientLightStrenght) + uAmbientLightStrenght;
        oColor.xyz = oColor.xyz * intensity;
    }
}
