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
