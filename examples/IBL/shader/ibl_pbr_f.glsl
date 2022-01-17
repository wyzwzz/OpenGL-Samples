#version 460 core
layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 outFragColor;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

#define LightNum 4

uniform vec3 lightPos[LightNum];
uniform vec3 lightColor[LightNum];

uniform vec3 cameraPos;

const float PI = 3.14159265359;
const float MaxReflectionLod = 4.f;

vec3 getNormalFromMap(){
    vec3 tangent_normal = texture(normalMap, inTexCoord).xyz * 2.f - 1.f;

    vec3 Q1 = dFdx(inWorldPos);
    vec3 Q2 = dFdy(inWorldPos);
    vec2 st1 = dFdx(inTexCoord);
    vec2 st2 = dFdy(inTexCoord);

    vec3 N = normalize(inNormal);
    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangent_normal);
}
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness){
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}
void main(){
    vec3 albedo = pow(texture(albedoMap, inTexCoord).rgb, vec3(2.2f));
    float metallic = texture(metallicMap, inTexCoord).r;
    float roughness = texture(roughnessMap, inTexCoord).r;
    float ao = texture(aoMap, inTexCoord).r;

    vec3 N = getNormalFromMap();
    vec3 V = normalize(cameraPos - inWorldPos);
    vec3 R = reflect(-V, N);

    vec3 F0 = vec3(0.04f);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.f);
    for(int i = 0; i < LightNum; i++){

    }
    vec3 F = vec3(0.5f);//fresnelSchlickRoughness(max(dot(N,V), 0.f), F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.f - kS;
    //金属度越高 漫反射的能量越少
    kD *= 1.f - metallic;

    //根据物体表面的法向量来获取半球的漫反射
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * albedo;
    
    //根据反射方向来以及粗糙度来采样镜面反射lobe的辐射度
    vec3 prefiltered_color = textureLod(prefilterMap, R, roughness * MaxReflectionLod).rgb;
    
    vec2 brdf = texture(brdfLUT, vec2(max(dot(N,V), 0.f), roughness)).rg;
    
    vec3 specular = prefiltered_color * (F * brdf.x + brdf.y);
    
    vec3 ambient = (kD * diffuse + specular) * ao;

    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.f));
    color = pow(color, vec3(1.f/2.2f));

    outFragColor = vec4(color, 1.f);
}