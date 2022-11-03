#include "sampleTextureIndexed.glsl"

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
