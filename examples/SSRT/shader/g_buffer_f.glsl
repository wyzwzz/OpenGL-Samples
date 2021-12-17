#version 430 core

layout(location = 0) in vec3 inFragPos;

layout(location = 1) in vec3 inFragNormal;

layout(location = 2) in vec2 inFragTexCoord;

layout(location = 3) in vec4 inPosFromLight;

uniform sampler2D ShadowMap;

uniform sampler2D DiffuseMap;

uniform sampler2D NormalMap;

layout(location = 0) out vec4 GDiffuse;

layout(location = 1) out vec4 GPos;

layout(location = 2) out vec4 GNormal;

layout(location = 3) out float GDepth;

layout(location = 4) out float GShadow;

float SimpleShadowMap(){
    vec3 proj_coord = inPosFromLight.xyz / inPosFromLight.w;
    vec3 shadow_coord = proj_coord*0.5f+0.5f;
    float depth = texture(ShadowMap,shadow_coord.xy).x;
    if(depth > shadow_coord.z - 0.001){
        return 1.f;
    }
    return 0.f;
}
void LocalBasis(vec3 n, out vec3 b1, out vec3 b2) {
    float sign_ = sign(n.z);
    if (n.z == 0.0) {
        sign_ = 1.0;
    }
    float a = -1.0 / (sign_ + n.z);
    float b = n.x * n.y * a;
    b1 = vec3(1.0 + sign_ * n.x * n.x * a, sign_ * b, -sign_ * n.x);
    b2 = vec3(b, sign_ + n.y * n.y * a, -n.y);
}
//todo ???
vec3 ApplyTangentNormalMap() {
    vec3 t, b;
    LocalBasis(inFragNormal, t, b);
    vec3 nt = texture(NormalMap, inFragTexCoord).xyz * 2.0 - 1.0;
    nt = normalize(nt.x * t + nt.y * b + nt.z * inFragNormal);
    return nt;
}
void main(){
    GDiffuse = vec4(texture(DiffuseMap,inFragTexCoord).rgb,1.f);

    GPos = vec4(inFragPos,1.f);

    GNormal = vec4(ApplyTangentNormalMap(),1.f);

    GDepth = gl_FragCoord.z;

    GShadow = SimpleShadowMap();
}