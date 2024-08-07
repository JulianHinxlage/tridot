#type vertex
#version 400 core

layout (location=0) in vec3 vPosition;
layout (location=1) in vec3 vNormal;
layout (location=2) in vec2 vTexCoords;

layout (location=3) in mat4 iTransform;
layout (location=7) in float iMaterialIndex;
layout (location=8) in vec4 iColor;
layout (location=9) in vec4 iId;

uniform mat4 uProjection = mat4(1);

out vec3 fLocalPosition;
out vec3 fLocalNormal;
out vec2 fTexCoords;
flat out float fMaterialIndex;
out vec4 fColor;
out vec3 fPosition;
out vec3 fNormal;
out vec3 fScale;
out vec4 fId;

void main(){
    fLocalPosition = vPosition;
    fLocalNormal = vNormal;
    fTexCoords = vTexCoords;
    fMaterialIndex = iMaterialIndex;
    fColor = iColor;
    fId = iId;

    vec4 pos = iTransform * vec4(vPosition, 1.0);
    fPosition = pos.xyz;
    gl_Position = uProjection * pos;
    fScale = vec3(length(iTransform[0].xyz), length(iTransform[1].xyz), length(iTransform[2].xyz));
    fNormal = vec3(iTransform * vec4(vNormal / fScale, 0.0));
}

#type fragment
#version 400 core

in vec3 fLocalPosition;
in vec3 fLocalNormal;
in vec2 fTexCoords;
flat in float fMaterialIndex;
in vec4 fColor;
in vec3 fPosition;
in vec3 fNormal;
in vec3 fScale;
in vec4 fId;

struct Light{
    vec3 position;
    int align1;
    vec3 color;
    int align2;
    float intensity;
    int type;
    int align3;
    int align4;
};
layout(std140) uniform uLights {
    Light lights[128];
};

struct Material{
    vec4 color;
    int mapping;
    float roughness;
    float metallic;
    float normalMapFactor;
    int texture;
    int normalMap;
    int roughnessMap;
    int metallicMap;
    vec2 textureOffset;
    vec2 textureScale;
    vec2 normalMapOffset;
    vec2 normalMapScale;
    vec2 roughnessMapOffset;
    vec2 roughnessMapScale;
    vec2 metallicMapOffset;
    vec2 metallicMapScale;
};
layout(std140) uniform uMaterials {
    Material materials[128];
};

layout(std140) uniform uEnvironment {
    mat4 projection;
    vec3 cameraPosition;
    int align1;
    int lightCount;
    float environmentMapIntensity;
    int environmentMapIndex;
    int irradianceMapIndex;
};

uniform sampler2D uTextures[30];
uniform samplerCube uCubeTextures[2];
const float PI = 3.14159265359;

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oId;

vec4 sampleTexture(int textureIndex, int mapping, vec2 textureScale, vec2 textureOffset);
float distributionFunction(vec3 N, vec3 H, float roughness);
float geometrySubFunction(float NdotV, float roughness);
float geometryFunction(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelFunction(float cosTheta, vec3 F0);

vec3 fresnelFunctionRoughness(float cosTheta, vec3 F0, float roughness){
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

vec4 sampleTextureIndexed(int textureIndex, vec2 textureCoords){
    for(int i = 0; i < 30; i++){
        if(i == textureIndex){
            return texture(uTextures[i], textureCoords);
        }
    }
    return vec4(0, 0, 0, 1);
}

vec4 sampleCubeTextureIndexed(int textureIndex, vec3 textureCoords){
    for(int i = 0; i < 2; i++){
        if(i == textureIndex){
            return texture(uCubeTextures[i], textureCoords);
        }
    }
    return vec4(0, 0, 0, 1);
}

vec4 sampleCubeTextureIndexedLod(int textureIndex, vec3 textureCoords, int lod){
    for(int i = 0; i < 2; i++){
        if(i == textureIndex){
            return texture(uCubeTextures[i], textureCoords, lod);
        }
    }
    return vec4(0, 0, 0, 1);
}

void main(){
    Material material = materials[int(fMaterialIndex)];

    //albedo/color
    vec4 albedo = material.color * fColor;
    if(material.texture != -1){
        albedo *= sampleTexture(material.texture, material.mapping, material.textureScale, material.textureOffset);
    }

    //normal
    vec3 normal = normalize(fNormal);
    if(material.normalMap != -1){
        vec3 n = sampleTexture(material.normalMap, material.mapping, material.normalMapScale, material.normalMapOffset).rgb;
        n = normalize(normalize(n) * 2.0 - 1.0);
        vec3 tangent = cross(normal, normalize(normal + vec3(0.2, 0.2, 0.2)));
        mat3 tbn = mat3(tangent, cross(tangent, normal), normal);
        normal = normalize(tbn * n) * material.normalMapFactor + normal * (1.0 - material.normalMapFactor);
    }

    //roughness
    float roughness = material.roughness;
    if(material.roughnessMap != -1){
        roughness *= (sampleTexture(material.roughnessMap, material.mapping, material.roughnessMapScale, material.roughnessMapOffset).r);
    }

    //metallic
    float metallic = material.metallic;
    if(material.metallicMap != -1){
        metallic *= (sampleTexture(material.metallicMap, material.mapping, material.metallicMapScale, material.metallicMapOffset).r);
    }

    vec3 viewDirection = normalize(cameraPosition - fPosition);
    vec3 surfaceReflection = mix(vec3(0.04), albedo.rgb, metallic);
    vec3 lightOutput = vec3(0.0);

    for(int i = 0; i < lightCount; i++){
        Light light = lights[i];

        vec3 lightDirection = normalize(-light.position);
        float lightDistance = 1;

        if (light.type == 2){ //point light
            lightDirection = normalize(light.position - fPosition);
            lightDistance = length(light.position - fPosition);
        }

        if(light.type == 0){//ambient light
            lightOutput += albedo.rgb * light.color * light.intensity;
        }else {//directional light or point light
            if(fNormal != vec3(0, 0, 0)){
                vec3 halfwayDirection = normalize(lightDirection + viewDirection);
                float attenuationFallof = 1;
                float attenuation = 1.0 / pow(lightDistance, attenuationFallof);
                vec3 radiance = light.color * light.intensity * attenuation;


                float distribution = distributionFunction(normal, halfwayDirection, roughness);
                float geometry = geometryFunction(normal, viewDirection, lightDirection, roughness);
                vec3 fresnel = fresnelFunction(max(dot(halfwayDirection, viewDirection), 0.0), surfaceReflection);

                vec3 diffuse = (vec3(1.0) - fresnel) * (1.0 - metallic);

                vec3 numerator = distribution * geometry * fresnel;
                float denominator = 4.0 * max(dot(normal, viewDirection), 0.0) * max(dot(normal, lightDirection), 0.0);
                vec3 specular = numerator / max(denominator, 0.001);

                float cosTheta = max(dot(normal, lightDirection), 0.0);
                lightOutput += (diffuse * albedo.rgb / PI + specular) * radiance * cosTheta;
                //lightOutput += (diffuse * albedo.rgb / PI) * radiance * cosTheta;
            }
        }
    }

    //environment mapping
    if(environmentMapIntensity != 0){
        vec3 reflectionDirection = normalize(-reflect(viewDirection, normal));

        vec3 radiance = sampleCubeTextureIndexed(environmentMapIndex, reflectionDirection).xyz * environmentMapIntensity;
        vec3 irradiance = sampleCubeTextureIndexed(irradianceMapIndex, normal).xyz * environmentMapIntensity;

        lightOutput += albedo.rgb * irradiance * (1.0 - metallic);
        lightOutput += radiance * (1.0f - roughness);
    }

    //flat shading for no lights
    if(lightCount == 0 && environmentMapIntensity == 0){
        lightOutput = albedo.rgb;
    }

    oColor = vec4(lightOutput, albedo.a);
    oId = fId;

    //gamma correction
    //float gamma = 2.2;
    //oColor.rgb = pow(oColor.rgb, vec3(1.0 / gamma));
}

vec4 sampleTexture(int textureIndex, int mapping, vec2 textureScale, vec2 textureOffset){
    vec4 color = vec4(1, 1, 1, 1);
    if(mapping == 0){//uv mapping
        color = sampleTextureIndexed(textureIndex, fTexCoords * textureScale + textureOffset);
    }else if(mapping == 1){//tri planar mapping
        vec3 n = abs(normalize(fLocalNormal));
        n = pow(n, vec3(1, 1, 1) * 3.0f);
        color =
        sampleTextureIndexed(textureIndex, fLocalPosition.xy * textureScale + textureOffset) * n.z +
        sampleTextureIndexed(textureIndex, fLocalPosition.xz * textureScale + textureOffset) * n.y +
        sampleTextureIndexed(textureIndex, fLocalPosition.yz * textureScale + textureOffset) * n.x;
        color /= n.x + n.y + n.z;
    }else if(mapping == 2){//scaled tri planar mapping
        vec3 n = abs(normalize(fLocalNormal));
        n = pow(n, vec3(1, 1, 1) * 3.0f);
        color =
        sampleTextureIndexed(textureIndex, fLocalPosition.xy * fScale.xy * textureScale + textureOffset) * n.z +
        sampleTextureIndexed(textureIndex, fLocalPosition.xz * fScale.xz * textureScale + textureOffset) * n.y +
        sampleTextureIndexed(textureIndex, fLocalPosition.yz * fScale.yz * textureScale + textureOffset) * n.x;
        color /= n.x + n.y + n.z;
    }
    return color;
}

float distributionFunction(vec3 normal, vec3 halfwayDirection, float roughness){
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(normal, halfwayDirection), 0.0);
    float NdotH2 = NdotH * NdotH;
    float numerator = a2;
    float denominator = (NdotH2 * (a2 - 1.0) + 1.0);
    denominator = PI * denominator * denominator;
    return numerator / denominator;
}

float geometrySubFunction(float NdotV, float roughness){
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float numerator = NdotV;
    float denominator = NdotV * (1.0 - k) + k;
    return numerator / denominator;
}

float geometryFunction(vec3 normal, vec3 viewDirection, vec3 lightDirection, float roughness){
    float NdotV = max(dot(normal, viewDirection), 0.0);
    float NdotL = max(dot(normal, lightDirection), 0.0);
    float sub2 = geometrySubFunction(NdotV, roughness);
    float sub1 = geometrySubFunction(NdotL, roughness);
    return sub1 * sub2;
}

vec3 fresnelFunction(float cosTheta, vec3 surfaceReflection){
    return surfaceReflection + (1.0 - surfaceReflection) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}
