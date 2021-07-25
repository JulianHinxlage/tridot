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
out vec3 fPosition;
out vec3 fModelPosition;
out vec3 fNormal;
out vec3 fModelNormal;
out vec2 fTexCoords;
out vec2 fTexCoordsTopLeft;
out vec2 fTexCoordsBottomRight;
flat out float fTextureUnit;

uniform mat4 uProjection = mat4(1);

void main(){
    vec4 pos = iTransform * vec4(vPosition, 1.0);
    fPosition = vec3(pos);
    fModelPosition = vPosition;
    fModelNormal = vNormal;
    gl_Position = uProjection * pos;
    fNormal = (iTransform * vec4(vNormal, 0.0)).xyz;
    fTexCoords = vTexCoords * (iTexCoordsBottomRight - iTexCoordsTopLeft) + iTexCoordsTopLeft;
    fColor = iColor;
    fTextureUnit = iTextureUnit;
    fTexCoordsTopLeft = iTexCoordsTopLeft;
    fTexCoordsBottomRight = iTexCoordsBottomRight;

    vec3 scale;
    scale.x = length(iTransform[0].xyz);
    scale.y = length(iTransform[1].xyz);
    scale.z = length(iTransform[2].xyz);
    fModelPosition *= scale;
}

#type fragment
#version 400 core

in vec4 fColor;
in vec3 fPosition;
in vec3 fModelPosition;
in vec3 fNormal;
in vec3 fModelNormal;
in vec2 fTexCoords;
in vec2 fTexCoordsTopLeft;
in vec2 fTexCoordsBottomRight;
flat in float fTextureUnit;
out vec4 oColor;

uniform sampler2D uTextures[32];
uniform vec3 uCameraPosition = vec3(0, 0, 0);
uniform bool uTriPlanarMapping = true;

vec4 sampleTexture(){
    vec4 color = vec4(1, 1, 1, 1);
    int index = int(fTextureUnit);
    for(int i = 0; i < 32; i++){
        if(i == index){
            if(uTriPlanarMapping){
                vec3 n = abs(fModelNormal);
                n = pow(n, vec3(3.0f));
                n /= n.x + n.y + n.z;
                color = (
                texture(uTextures[i], fModelPosition.xy * (fTexCoordsBottomRight - fTexCoordsTopLeft) + fTexCoordsTopLeft) * n.z +
                texture(uTextures[i], fModelPosition.xz * (fTexCoordsBottomRight - fTexCoordsTopLeft) + fTexCoordsTopLeft) * n.y +
                texture(uTextures[i], fModelPosition.yz * (fTexCoordsBottomRight - fTexCoordsTopLeft) + fTexCoordsTopLeft) * n.x
                ) * fColor;
            }else{
                color = texture(uTextures[i], fTexCoords) * fColor;
            }
            break;
        }
    }
    return color;
}

void main(){
    oColor = sampleTexture();
}
