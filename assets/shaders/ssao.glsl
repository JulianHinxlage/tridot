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
uniform vec3 samples[256];
uniform int kernalSize = 64;
uniform float sampleRadius = 1.0;
uniform float bias = 0.025;
uniform float occlusionStrength = 1.0;

layout(std140) uniform uEnvironment {
    mat4 projection;
    mat4 view;
    mat4 viewProjection;
    vec3 eyePosition;
    int align1;
    int lightCount;
    float environmentMapIntensity;
    int radianceMapIndex;
    int irradianceMapIndex;
};

out vec4 oColor;

void main(){
    vec3 position = texture(uTextures[0], fTexCoords).xyz;
    vec3 normal = texture(uTextures[1], fTexCoords).xyz;
    if(normal == vec3(0, 0, 0)){
        oColor.r = 1;
        return;
    }
    normal = normalize(normal * 2.0 - 1.0);

    //view space
    position = vec3(view * vec4(position, 1.0));
    normal = normalize(vec3(transpose(inverse(view)) * vec4(normal, 1.0)));


    vec2 noiseScale = textureSize(uTextures[0], 0) / textureSize(uTextures[2], 0);
    vec3 noise = texture(uTextures[2], fTexCoords * noiseScale).xyz;

    vec3 tangent = normalize(noise - normal * dot(noise, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal);

    float occlusion = 0;
    for(int i = 0; i < kernalSize; i++){
        vec3 samplePos = position + (tbn * samples[i]) * sampleRadius;
        vec4 samplePosUV = projection * vec4(samplePos, 1.0);
        samplePosUV.xyz /= samplePosUV.w;
        samplePosUV.xyz = samplePosUV.xyz * 0.5 + 0.5;

        vec3 actualPos = texture(uTextures[0], samplePosUV.xy).xyz;
        actualPos = vec3(view * vec4(actualPos, 1.0));

        float rangeCheck = smoothstep(0.0, 1.0, sampleRadius / abs(position.z - actualPos.z));
        occlusion += (actualPos.z >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / kernalSize);
    oColor.r = pow(occlusion, occlusionStrength);
}
