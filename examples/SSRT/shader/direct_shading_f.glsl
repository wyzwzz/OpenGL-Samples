#version 430 core

layout(location = 0) in vec3 inFragNormal;

layout(location = 1) in vec3 inFragPos;

layout(binding = 0,rgba32f) uniform image2D GBufferDiffuse;

layout(binding = 1,rgba32f) uniform image2D GBufferPos;

layout(binding = 2,rgba32f) uniform image2D GBufferNormal;

layout(binding = 3, r32f) uniform image2D GBufferDepth;

layout(binding = 4, r32f) uniform image2D GBufferShadow;

layout(location = 0) out vec4 DirectColor;

uniform vec3 LightDir;
uniform vec3 CameraPos;
uniform vec3 LightRadiance;
uniform vec3 ks;

vec3 blinnPhong(){
    vec3 color = imageLoad(GBufferDiffuse,ivec2(gl_FragCoord)).rgb;
    color = pow(color,vec3(2.2f));

    vec3 ambient = 0.05f * color;

    vec3 light_dir = -normalize(LightDir);
    vec3 normal = normalize(inFragNormal);
//    vec3 normal = normalize(imageLoad(GBufferNormal,ivec2(gl_FragCoord)).rgb);
    float diff = max(dot(light_dir,normal),0.f);
    vec3 diffuse = diff * LightRadiance * color;
    vec3 frag_pos = inFragPos;//imageLoad(GBufferPos,ivec2(gl_FragCoord)).rgb;
    vec3 view_dir = normalize(CameraPos-frag_pos);
    vec3 half_dir = normalize(light_dir+view_dir);
    float spec = pow(max(dot(half_dir,normal),0.f),32.f);
    vec3 specular =  ks * LightRadiance * spec;

    vec3 radiance  = ambient + diffuse + specular;
    vec3 phong_color = pow(radiance,vec3(1.f/2.2f));
    return phong_color;
}

void main() {
    float visibility = imageLoad(GBufferShadow,ivec2(gl_FragCoord)).r;
    vec3 shading_color = blinnPhong();
    DirectColor.rgb = shading_color * visibility;
    DirectColor.a = 1.f;
}
