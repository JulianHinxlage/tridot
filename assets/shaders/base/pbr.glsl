const float PI = 3.14159265359;

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

vec3 pbrLighting(vec3 albedo, vec3 normal, vec3 viewDirection, vec3 lightDirection, float metallic, float roughness) {
    vec3 n = normal;
    vec3 v = viewDirection;
    vec3 l = lightDirection;
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
