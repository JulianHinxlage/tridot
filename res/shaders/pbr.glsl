#type vertex
#version 400 core

layout (location=0) in vec3 vPosition;
layout (location=1) in vec3 vNormal;
layout (location=2) in vec2 vTexCoords;

layout (location=3) in mat4 iTransform;
layout (location=7) in vec4 iColor;
layout (location=8) in float iMaterialIndex;
layout (location=9) in vec4 iId;

layout(std140) uniform uEnvironment {
    mat4 projection;
    vec3 cameraPosition;
    int align1;
    int lightCount;
    float environmentMapIntensity;
    int radianceMapIndex;
    int irradianceMapIndex;
};

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
    gl_Position = projection * pos;
    fScale = vec3(length(iTransform[0].xyz), length(iTransform[1].xyz), length(iTransform[2].xyz));
    fNormal = normalize(vec3(iTransform * vec4(vNormal, 0.0)));
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
    vec3 direction;
    int align2;
    vec3 color;
    int align3;
    int type;
    float intensity;
    int shadowMapIndex;
    int align4;
    mat4 projection;
};
layout(std140) uniform uLights {
    Light lights[32];
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
    int ambientOcclusionMap;
    int displacementMap;
    int align1;
    int align2;
    vec2 textureOffset;
    vec2 textureScale;
    vec2 normalMapOffset;
    vec2 normalMapScale;
    vec2 roughnessMapOffset;
    vec2 roughnessMapScale;
    vec2 metallicMapOffset;
    vec2 metallicMapScale;
    vec2 ambientOcclusionMapOffset;
    vec2 ambientOcclusionMapScale;
    vec2 displacementMapOffset;
    vec2 displacementMapScale;
};
layout(std140) uniform uMaterials {
    Material materials[32];
};

layout(std140) uniform uEnvironment {
    mat4 projection;
    vec3 cameraPosition;
    int align1;
    int lightCount;
    float environmentMapIntensity;
    int radianceMapIndex;
    int irradianceMapIndex;
};

uniform sampler2D uTextures[30];
uniform samplerCube uCubeTextures[2];
const float PI = 3.14159265359;

out vec4 oColor;
out vec4 oId;

vec4 sampleTexture(int textureIndex, int mapping, vec2 textureScale, vec2 textureOffset);
vec4 sampleTextureIndexed(int textureIndex, vec2 textureCoords);
vec2 textureSizeIndexed(int textureIndex);
vec4 sampleCubeTextureIndexed(int textureIndex, vec3 textureCoords);
vec4 sampleCubeTextureIndexedLod(int textureIndex, vec3 textureCoords, int lod);

vec3 pbrLight(vec3 albedo, vec3 normal, vec3 viewDirection, vec3 lightDirection, float metallic, float roughness);
vec3 lighing(vec3 albedo, vec3 normal, float metallic, float roughness, float ao);

void main(){
    Material material = materials[int(fMaterialIndex)];

    //albedo
    vec4 albedo = material.color * fColor;
    if (material.texture != -1){
        albedo *= sampleTexture(material.texture, material.mapping, material.textureScale, material.textureOffset);
    }

    //normal
    vec3 normal = fNormal;
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

    //roughness
    float ao = 1;
    if(material.ambientOcclusionMap != -1){
        ao *= (sampleTexture(material.ambientOcclusionMap, material.mapping, material.ambientOcclusionMapScale, material.ambientOcclusionMapOffset).r);
    }

    //metallic
    float displacement = 0;
    if(material.displacementMap != -1){
        displacement = (sampleTexture(material.displacementMap, material.mapping, material.displacementMapScale, material.displacementMapOffset).r);
    }

    vec3 lightColor = lighing(albedo.rgb, normal, metallic, roughness, ao);
    oColor = vec4(lightColor, albedo.a);
    oId = vec4(fId.xyz, 1.0);
}

float shadowMapping(int lightIndex, float ndotl){
    Light light = lights[lightIndex];
    vec3 pos = vec3(light.projection * vec4(fPosition, 1.0)) * 0.5 + 0.5;

    float bias = max(0.005 * (1.0 - ndotl), 0.0005);
    if(pos.z > 1.0){
        pos.z = 1.0;
    }

    //float depth = sampleTextureIndexed(light.shadowMapIndex, pos.xy).r;
    //return (depth + bias) < pos.z ? 0.0 : 1.0;

    //vec2 texelSize = 1.0 / textureSizeIndexed(light.shadowMapIndex);
    vec2 texelSize = 1.0 / vec2(2048.0, 2048.0);
    float shadow = 0;
    int count = 2;
    for(int x = -count; x <= count; ++x){
        for(int y = -count; y <= count; ++y){
            float depth = sampleTextureIndexed(light.shadowMapIndex, pos.xy + vec2(x, y) * texelSize).r;
            shadow += (depth + bias) < pos.z ? 0.0 : 1.0;
        }
    }

    return shadow / float((count * 2 + 1) * (count * 2 + 1));
}

vec3 lighing(vec3 albedo, vec3 normal, float metallic, float roughness, float ao){
    vec3 lightOutput = vec3(0.0);
    vec3 viewDirection = normalize(cameraPosition - fPosition);

    //lights
    for(int i = 0; i < lightCount; i++){
        Light light = lights[i];

        if(light.type == 0){
            //ambient light
            lightOutput += albedo * light.color * light.intensity * ao;
        }else {
            //directional light or point light

            float attenuation = 1;
            vec3 lightDirection = vec3(0, 0, -1);
            float shadow = 1;

            if (light.type == 2){
                //point light
                lightDirection = normalize(light.position - fPosition);
                float lightDistance = length(light.position - fPosition);
                float attenuationFallof = 0.8;
                attenuation = 1.0 / pow(lightDistance, attenuationFallof);
            }else{
                //directional light
                lightDirection = normalize(-light.direction);
                float ndotl = max(0.0, dot(normal, lightDirection));
                shadow = shadowMapping(i, ndotl);
            }

            vec3 radiance = light.color * light.intensity * attenuation;
            lightOutput += pbrLight(albedo, normal, viewDirection, lightDirection, metallic, roughness) * radiance * shadow;
        }
    }

    //environment mapping
    if(environmentMapIntensity != 0){
        vec3 reflectionDirection = normalize(-reflect(viewDirection, normal));

        vec3 radiance = sampleCubeTextureIndexed(radianceMapIndex, reflectionDirection).xyz * environmentMapIntensity;
        vec3 irradiance = sampleCubeTextureIndexed(irradianceMapIndex, normal).xyz * environmentMapIntensity;
    
        lightOutput += albedo.rgb * irradiance * (1.0 - metallic);
        lightOutput += radiance * (1.0f - roughness);
    }

    //flat shading for no lights
    if (lightCount == 0 && environmentMapIntensity == 0){
        lightOutput = albedo * ao;
    }

    return lightOutput;
}

float ndfGGX(float ndoth, float roughness){
    float a   = roughness * roughness;
    float a2 = a * a;
    float denom = (ndoth * ndoth) * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float gaSchlickG1(float cosTheta, float k){
    return cosTheta / (cosTheta * (1.0 - k) + k);
}

float gaSchlickGGX(float ndotl, float ndotv, float roughness){
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return gaSchlickG1(ndotl, k) * gaSchlickG1(ndotv, k);
}

vec3 fresnelSchlick(vec3 f0, float cosTheta){
    return f0 + (vec3(1.0) - f0) * pow(1.0 - cosTheta, 5.0);
}

vec3 pbrLight(vec3 albedo, vec3 normal, vec3 viewDirection, vec3 lightDirection, float metallic, float roughness){
    vec3 n = normal;
    vec3 v = viewDirection;
    vec3 l = lightDirection;
    //halfway vector
    vec3 h = normalize(v + l);

    float ndotl = max(0.0, dot(n, l));
    float ndotv = max(0.0, dot(n, v));
    float ndoth = max(0.0, dot(n, h));
    float hdotv = max(0.0, dot(h, v));

    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 f = fresnelSchlick(f0, hdotv);
    float d = ndfGGX(ndoth, roughness);
    float g = gaSchlickGGX(ndotl, ndotv, roughness);

    vec3 diffuse = (vec3(1.0) - f) * (1.0 - metallic);
    vec3 specular = (f * d * g) / max(0.0001, 4.0 * ndotl * ndotv);

    return (diffuse * albedo / PI + specular) * ndotl;
}

vec4 sampleTexture(int textureIndex, int mapping, vec2 textureScale, vec2 textureOffset){
    vec4 color = vec4(1, 1, 1, 1);
    if(mapping == 0){
        //uv mapping
        color = sampleTextureIndexed(textureIndex, fTexCoords * textureScale + textureOffset);
    }else if(mapping == 1){
        //tri planar mapping
        vec3 n = abs(normalize(fLocalNormal));
        n = pow(n, vec3(1, 1, 1) * 3.0f);
        color =
        sampleTextureIndexed(textureIndex, fLocalPosition.xy * textureScale + textureOffset) * n.z +
        sampleTextureIndexed(textureIndex, fLocalPosition.xz * textureScale + textureOffset) * n.y +
        sampleTextureIndexed(textureIndex, fLocalPosition.yz * textureScale + textureOffset) * n.x;
        color /= n.x + n.y + n.z;
    }else if(mapping == 2){
        //scaled tri planar mapping
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

vec4 sampleTextureIndexed(int textureIndex, vec2 textureCoords){
    if(textureIndex == -1){
        return vec4(1, 1, 1, 1);
    }
    for(int i = 0; i < 30; i++){
        if(i == textureIndex){
            return texture(uTextures[i], textureCoords);
        }
    }
    return vec4(0, 0, 0, 1);
}

vec2 textureSizeIndexed(int textureIndex){
    for(int i = 0; i < 30; i++){
        if(i == textureIndex){
            return vec2(textureSize(uTextures[i], 0));
        }
    }
    return vec2(1, 1);
}

vec4 sampleCubeTextureIndexed(int textureIndex, vec3 textureCoords){
    if(textureIndex == -1){
        return vec4(1, 1, 1, 1);
    }
    for(int i = 0; i < 2; i++){
        if(i == textureIndex){
            vec3 v = textureCoords;
            return texture(uCubeTextures[i], vec3(v.x, -v.z, v.y));
        }
    }
    return vec4(0, 0, 0, 1);
}

vec4 sampleCubeTextureIndexedLod(int textureIndex, vec3 textureCoords, int lod){
    for(int i = 0; i < 2; i++){
        if(i == textureIndex){
            vec3 v = textureCoords;
            return texture(uCubeTextures[i], vec3(v.x, -v.z, v.y), lod);
        }
    }
    return vec4(0, 0, 0, 1);
}
