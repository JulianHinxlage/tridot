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

struct Light{
    vec3 position;
    int align1;
    vec3 direction;
    int align2;
    vec3 color;
    int align3;
    int type;
    float intensity;
    float radius;
    int shadowMapIndex;
    mat4 projection;
};
layout(std140) uniform uLights {
    Light lights[32];
};

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

uniform sampler2D uTextures[30];
uniform samplerCube uCubeTextures[2];
uniform float bloomThreshold = 1.0;
const float PI = 3.14159265359;

out vec4 oColor;
out vec4 oEmissive;

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


float shadowMapping(int lightIndex, float ndotl, vec3 fPosition){
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

vec3 lighing(vec3 albedo, vec3 normal, float metallic, float roughness, float ao, vec3 fPosition){
    vec3 lightOutput = vec3(0.0);
    vec3 viewDirection = normalize(eyePosition - fPosition);

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
                shadow = shadowMapping(i, ndotl, fPosition);
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

void main() {
    //inputs
    vec4 albedo = texture(uTextures[0], fTexCoords);
    vec4 depth = texture(uTextures[5], fTexCoords);
    if(depth.rgb == vec3(1, 1, 1)){
        oColor = albedo;
        oEmissive = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 normal = texture(uTextures[1], fTexCoords).xyz;
    normal = normalize(normal * 2.0 - 1.0);

    vec3 position = texture(uTextures[2], fTexCoords).xyz;
    vec4 RME = texture(uTextures[3], fTexCoords);
    float roughness = RME.r;
    float metallic = RME.g;
    float emissive = RME.b;
    float ao = texture(uTextures[4], fTexCoords).r;

    //lighting
    vec3 lightColor = lighing(albedo.rgb, normal, metallic, roughness, ao, position);
    oColor = vec4(lightColor, albedo.a);
    oColor.rgb += emissive * albedo.rgb;

    //emissive
    float brightness = dot(lightColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > bloomThreshold){
        oEmissive = vec4(lightColor.rgb, 1.0);
    }else{
        oEmissive = vec4(0.0, 0.0, 0.0, emissive > 0.0 ? 1.0 : albedo.a);
    }
    oEmissive.rgb += emissive * albedo.rgb;
}

