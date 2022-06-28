#version 460 core

layout(location = 0) in vec2 iFragTexCoord;

layout(location = 0) out vec4 oFragColor;

layout(std140,binding = 0) uniform SkyParams{
    vec3 frustum_a;
    vec3 frustum_b;
    vec3 frustum_c;
    vec3 frustum_d;
};

#define PI 3.14159265
#define POSTCOLOR_A 2.51
#define POSTCOLOR_B 0.03
#define POSTCOLOR_C 2.43
#define POSTCOLOR_D 0.59
#define POSTCOLOR_E 0.14

vec3 tonemap(vec3 color)
{
    return (color * (POSTCOLOR_A * color + POSTCOLOR_B))
    / (color * (POSTCOLOR_C * color + POSTCOLOR_D) + POSTCOLOR_E);
}

vec3 postProcessColor(vec2 seed, vec3 color)
{
    #define TONEMAP
#ifdef TONEMAP
    color = tonemap(color);

    float rand = fract(sin(dot(seed, vec2(12.9898, 78.233) * 2.0)) * 43758.5453);
    color = 255 * clamp(pow(color, vec3(1 / 2.2)),vec3(0),vec3(1));
    color = all(lessThan(rand.xxx,(color - floor(color)))) ? ceil(color) : floor(color);
    return color / 255.0;
#else
    vec3 white_point = vec3(1.08241, 0.96756, 0.95003);
    float exposure = 10.0;
    return pow(vec3(1.0) - exp(-color / white_point * exposure), vec3(1.0 / 2.2));
#endif
}
uniform sampler2D SkyView;

void main() {
//    oFragColor = vec4(iFragTexCoord,1, 1);return;
    vec3 view_dir = mix(mix(frustum_a,frustum_b,iFragTexCoord.x),
                    mix(frustum_c,frustum_d,iFragTexCoord.x),1 - iFragTexCoord.y);
    view_dir = normalize(view_dir);
    float phi = atan(view_dir.z,view_dir.x) + PI;
    float u = phi / (2 * PI);

    float theta = asin(view_dir.y);
    float v = 0.5 + 0.5 * sign(theta) * sqrt(abs(theta) / (PI / 2));

    vec3 sky_color = texture(SkyView,vec2(u,v)).rgb;

    sky_color = postProcessColor(iFragTexCoord,sky_color);

    oFragColor = vec4(sky_color,1.0);
}
