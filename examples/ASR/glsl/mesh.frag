#version 460 core

layout(location = 0) in vec3 iFragPos;
layout(location = 1) in vec3 iFragNormal;
layout(location = 2) in vec2 iFragTexCoord;
layout(location = 3) in vec4 iScreenCoord;

layout(location = 0) out vec4 oFragColor;
layout(std140,binding = 0) uniform AtmosphereProperties{
    vec3 rayleigh_scattering;
    float rayleigh_density_h;

    float mie_scattering;
    float mie_asymmetry_g;
    float mie_absorption;
    float mie_density_h;

    vec3 ozone_absorption;
    float ozone_center_h;
    float ozone_width;

    float ground_radius;
    float top_atmosphere_radius;
};
layout(std140,binding = 1) uniform MeshParams{
    vec3 sun_dir;
    float sun_theta;
    vec3 sun_intensity;
    float max_aeraial_distance;
    vec3 view_pos;
    float world_scale;
    mat4 shadow_vp;

};

uniform sampler2D Transmittance;
uniform sampler3D AerialPerspective;
uniform sampler2D ShadowMap;

uniform sampler2D AlbedoMap;//optional
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
vec3 getTransmittance(float h,float theta){
    float u = h / (top_atmosphere_radius - ground_radius);
    float v = 0.5 + 0.5 * sin(theta);
    return texture(Transmittance, vec2(u, v)).rgb;
}

void main() {
//    oFragColor = vec4(iFragNormal,1.f);return;
    vec3 world_pos = iFragPos;
    vec3 normal = normalize(iFragNormal);
    vec2 screen_coord = iScreenCoord.xy / iScreenCoord.w;
    screen_coord = 0.5 + screen_coord * vec2(0.5,-0.5);//ndc differ with image which start at left-up corner

    float z = 0.9;//world_scale * distance(world_pos,view_pos) / max_aeraial_distance;
    vec4 aerial_res = texture(AerialPerspective,vec3(screen_coord,z));
//    oFragColor = vec4(screen_coord,z,1.0);return;
    vec3 in_scattering = aerial_res.rgb;
//    oFragColor = vec4(in_scattering,1.0);return;
    float view_transmittance = aerial_res.w;
    vec3 sun_transmittance = getTransmittance(world_pos.y * world_scale,sun_theta);
    vec3 albedo = vec3(0.03);//texture(AlbedoMap,iFragTexCoord).rgb;

    vec4 shadow_clip = shadow_vp * vec4(world_pos + 0.03 * normal,1);
    vec2 shadow_ndc = shadow_clip.xy / shadow_clip.w;
    vec2 shadow_uv = 0.5 + 0.5 * shadow_ndc;

    float shadow_factor = 1;
    if(all(equal(clamp(shadow_uv,vec2(0),vec2(1)), shadow_uv))){
        float view_z = shadow_clip.z * 0.5 + 0.5;
        float shadow_z = texture(ShadowMap,shadow_uv).r;
        shadow_factor = float(view_z <= shadow_z);
    }

    vec3 res = sun_intensity * (
    in_scattering +
    shadow_factor * sun_transmittance * albedo
    * max(0,dot(normal,-sun_dir)) * view_transmittance);

    res = postProcessColor(screen_coord,res);
    oFragColor = vec4(res,1.0);
}
