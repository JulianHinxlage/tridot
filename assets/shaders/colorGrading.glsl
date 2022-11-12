#type vertex
#version 400 core

#include "base/vertexInput.glsl"

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
uniform float hueShift = 0.0f;
uniform float saturation = 1.0f;
uniform float temperature = 0.0f;
uniform float contrast = 1.0f;
uniform float brightness = 1.0f;
uniform float gamma = 1.0f;
uniform vec4 averageLuminance = vec4(0.5, 0.5, 0.5, 1.0);
uniform vec3 gain = vec3(0.0, 0.0, 0.0);

out vec4 oColor;

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

#define sat(x) clamp(x,0.,1.)
vec3 colorFromKelvin(float temperature) // photographic temperature values are between 15 to 150
{
    float r, g, b;
    if(temperature <= 66.0)
    {
        r = 1.0;
        g = sat((99.4708025861 * log(temperature) - 161.1195681661) / 255.0);
        if(temperature < 19.0)
            b = 0.0;
        else
            b = sat((138.5177312231 * log(temperature - 10.0) - 305.0447927307) / 255.0);
    }
    else
    {
        r = sat((329.698727446 / 255.0) * pow(temperature - 60.0, -0.1332047592));
        g = sat((288.1221695283  / 255.0) * pow(temperature - 60.0, -0.0755148492));
        b = 1.0;
    }
    return vec3(r, g, b);
}

void main(){
    vec4 color = texture(uTextures[0], fTexCoords);

    //saturation
    color.rgb = mix(vec3(dot(color.rgb, vec3(0.2126, 0.7152, 0.0722))), color.rgb, saturation);

    //hue shift
    vec3 hsv = rgb2hsv(color.rgb);
    hsv.x += hueShift;
    color.rgb = hsv2rgb(hsv);

    //temperature
    color.rgb *= vec3(1.0) / colorFromKelvin(20 + temperature / 4.0f * (200.0f - 20.0f));

    //gain
    color.rgb = max(vec3(0.0), color.rgb * (1.0 + gain));

    //contrast and brightness
    color = mix(color * brightness, mix(averageLuminance, color, contrast), 0.5f);

    //gamma
    color.rgb = pow(color.rgb, vec3(1.0 / gamma));

    oColor = color;
}
