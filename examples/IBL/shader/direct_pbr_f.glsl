#version 460 core
layout(location = 0) in vec3 inFragPos;
layout(location = 1) in vec3 inFragNormal;
layout(location = 2) in vec2 inFragTexCoord;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

uniform vec3 cameraPos;

#define LightNum 4

uniform vec3 lightPos[LightNum];
uniform vec3 lightColor[LightNum];

const float PI = 3.14159265359;

layout(location = 0) out vec4 outFragColor;

// dFdx 对x的偏导 内部根据相邻的片段计算

vec3 getNormalFromMap(){
    vec3 tangentNormal = texture(normalMap,inFragTexCoord).xyz*2.f-1.f;

    vec3 Q1 = dFdx(inFragPos);
    vec3 Q2 = dFdy(inFragPos);
    vec2 st1 = dFdx(inFragTexCoord);
    vec2 st2 = dFdy(inFragTexCoord);

    vec3 N = normalize(inFragNormal);
    vec3 T = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B = -normalize(cross(N,T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness){
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N,H),0.f);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.f) + 1.f);
    denom = PI * denom * denom;

    return nom / denom;
}
float GeometrySchlickGGX(float NdotV, float roughness){
    float r = (roughness + 1.f);
    float k = (r * r) / 8.f;

    float nom = NdotV;
    float denom = NdotV * (1.f - k) + k;

    return nom / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness){
    float NdotV = max(dot(N,V),0.f);
    float NdotH = max(dot(N,L),0.f);
    float ggx2 = GeometrySchlickGGX(NdotV,roughness);
    float ggx1 = GeometrySchlickGGX(NdotH,roughness);

    return ggx1 * ggx2;
}
vec3 fresnelSchlick(float cosTheta, vec3 F0){
    return F0 + (1.f - F0) * pow(max(1.f - cosTheta, 0.f), 5.f);
}

void main(){
    vec3 albedo = pow(texture(albedoMap,inFragTexCoord).rgb,vec3(2.2f));
    float metallic = texture(metallicMap,inFragTexCoord).r;
    float roughness = texture(roughnessMap,inFragTexCoord).r;
    float ao = texture(aoMap,inFragTexCoord).r;

    vec3 N = getNormalFromMap();
    vec3 V = normalize(cameraPos - inFragPos);

    vec3 F0 = vec3(0.04f);
    //mix(x,y,a) = (1-a) * x + a * y
    F0 = mix(F0,albedo,metallic);

    vec3 Lo = vec3(0.f);
    for(int i = 0; i<LightNum; i++){
        vec3 L = normalize(lightPos[i] - inFragPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPos[i] - inFragPos);
        float attenuation = 1.f / (distance * distance);
        vec3 radiance = lightColor[i] * attenuation;

        float NDF = DistributionGGX(N,H,roughness);
        float G = GeometrySmith(N,V,L,roughness);
        vec3 F = fresnelSchlick(max(dot(H,V),0.f),F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4 * max(dot(N,V),0.f) * max(dot(N,L),0.f) + 0.001f;
        vec3 specular = numerator / denominator;

        vec3 kS = F;

        vec3 kD = vec3(1.f) - kS;

        kD *= 1.f - metallic;

        float NdotL = max(dot(N,L),0.f);

        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03f) * albedo * ao;

    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.f));

    color = pow(color, vec3(1.f/2.2f));

    outFragColor = vec4(color,1.f);
}