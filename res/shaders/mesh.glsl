#type vertex
#version 400 core

layout (location=0) in vec3 vPosition;
layout (location=1) in vec3 vNormal;
layout (location=2) in vec2 vTexCoords;

layout (location=3) in mat4 iTransform;
layout (location=7) in vec4 iColor;
layout (location=8) in float iTextureUnit;
layout (location=9) in vec2 iTexCoordsTopLeft;
layout (location=10) in vec2 iTexCoordsBottomRight;

out vec4 fColor;
out vec2 fTexCoords;
out vec3 fNormal;
flat out float fTextureUnit;

uniform mat4 uProjection = mat4(1);

void main(){
    gl_Position = uProjection * iTransform * vec4(vPosition, 1.0);
    fNormal = (iTransform * vec4(vNormal, 0.0)).xyz;
    fTexCoords = vTexCoords * (iTexCoordsBottomRight - iTexCoordsTopLeft) + iTexCoordsTopLeft;
    fColor = iColor;
    fTextureUnit = iTextureUnit;
}

#type fragment
#version 400 core

in vec4 fColor;
in vec3 fNormal;
in vec2 fTexCoords;
flat in float fTextureUnit;
out vec4 oColor;

uniform sampler2D uTextures[32];
uniform vec3 uAmbientLightDirection = vec3(0.2, 0.4, -1);
uniform float uAmbientLightStrenght = 0.75;

void main(){
    int index = int(fTextureUnit);
    for(int i = 0; i < 32; i++){
        if(i == index){
            oColor = texture(uTextures[i], fTexCoords) * fColor;
            break;
        }
    }

    if(fNormal != vec3(0, 0, 0)){
        float theta = dot(normalize(fNormal), -normalize(uAmbientLightDirection));
        float intensity = (theta / 2.0 + 0.5) * (1.0 - uAmbientLightStrenght) + uAmbientLightStrenght;
        oColor.xyz = oColor.xyz * intensity;
    }
}
