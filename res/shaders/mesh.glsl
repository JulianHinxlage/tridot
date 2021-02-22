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
out vec2 fTexCoords;
out vec3 fNormal;
flat out float fTextureUnit;

uniform mat4 uProjection = mat4(1);
uniform bool uTextureScale = false;

void main(){
    vec4 pos = iTransform * vec4(vPosition, 1.0);
    fPosition = vec3(pos);
    gl_Position = uProjection * pos;
    fNormal = (iTransform * vec4(vNormal, 0.0)).xyz;
    fTexCoords = vTexCoords * (iTexCoordsBottomRight - iTexCoordsTopLeft) + iTexCoordsTopLeft;
    fColor = iColor;
    fTextureUnit = iTextureUnit;

    if(uTextureScale){
        vec3 scale;
        scale.x = length(iTransform * vec4(1, 0, 0, 0));
        scale.y = length(iTransform * vec4(0, 1, 0, 0));
        scale.z = length(iTransform * vec4(0, 0, 1, 0));
        vec3 face = vNormal * 2;
        vec2 texCoords = fTexCoords;
        fTexCoords = texCoords * scale.xy * face.z + texCoords * scale.xz * face.y + texCoords * scale.zy * face.x;
    }
}

#type fragment
#version 400 core

in vec4 fColor;
in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
flat in float fTextureUnit;
out vec4 oColor;

uniform sampler2D uTextures[32];
uniform vec3 uCameraPosition = vec3(0, 0, 0);

uniform vec3 uAmbientColor = vec3(1, 1, 1);
uniform float uAmbientStrenght = 0.4;
uniform vec3 uDirectionalLightDirection = vec3(0.2, 0.4, -1);
uniform vec3 uDirectionalLightColor = vec3(1, 1, 1);
uniform float uDirectionalStrenght = 0.5;
uniform vec3 uSpecularColor = vec3(1, 1, 1);
uniform float uSpecularStrenght = 0.4;
uniform float shininess = 4;

vec4 sampleTexture(){
    vec4 color = vec4(1, 1, 1, 1);
    int index = int(fTextureUnit);
    for(int i = 0; i < 32; i++){
        if(i == index){
            color = texture(uTextures[i], fTexCoords) * fColor;
            break;
        }
    }
    return color;
}

void main(){
    oColor = sampleTexture();

    //ambient
    vec3 ambient = uAmbientColor * uAmbientStrenght;

    //diffuse
    vec3 diffuse = vec3(0, 0, 0);
    if(fNormal != vec3(0, 0, 0)){
        float theta = dot(normalize(fNormal), -normalize(uDirectionalLightDirection));
        theta = max(theta, 0.0);
        diffuse = uDirectionalLightColor * theta * uDirectionalStrenght;
    }

    //specular
    vec3 viewDir = normalize(uCameraPosition - fPosition);
    vec3 reflectDir = reflect(uDirectionalLightDirection, normalize(fNormal));
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = uSpecularStrenght * spec * uSpecularColor * uDirectionalLightColor;

    oColor.xyz = (ambient + diffuse + specular) * oColor.xyz;
}
