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
float textureProj(vec4 shadowCoord, vec2 off)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
        float bias = 0.001;
		float dist = texture( ShadowMap, shadowCoord.st + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z - bias) 
		{
			shadow = 0.005;
		}
	}
	return shadow;
}
float filterPCF(vec4 sc)
{
	ivec2 texDim = textureSize(ShadowMap, 0);
	float scale = 1.0;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y));
			count++;
		}
	
	}
	return shadowFactor / count;
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
    vec3 albedo = texture(DiffuseMap,inFragTexCoord).rgb;
    GDiffuse = vec4(albedo,1.f);

    GPos = vec4(inFragPos,1.f);

    GNormal = vec4(ApplyTangentNormalMap(),1.f);

    GDepth = gl_FragCoord.z;

    vec3 proj_coord = inPosFromLight.xyz / inPosFromLight.w;
    vec3 shadow_coord = proj_coord*0.5f+0.5f;
    GShadow = filterPCF(vec4(shadow_coord,1.f));
}