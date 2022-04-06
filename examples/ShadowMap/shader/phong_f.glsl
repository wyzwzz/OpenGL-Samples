#version 430 core
in vec3 position;
in vec3 normal;
in vec2 tex_coord;
in vec4 pos_from_light;
out vec4 frag_color;

uniform sampler2D AlbedoMap;
uniform sampler2D ShadowMap;
uniform vec3 light_dir;
uniform vec3 camera_pos;
uniform int shadow_mode;

float unpack(vec4 rgbaDepth) {
    const vec4 bitShift = vec4(1.0, 1.0/256.0, 1.0/(256.0*256.0), 1.0/(256.0*256.0*256.0));
    return dot(rgbaDepth, bitShift);
}
float Bias(){
    //解决shadow bias 因为shadow map的精度有限，当要渲染的fragment在light space中距Light很远的时候，
//就会有多个附近的fragement会samper shadow map中同一个texel,
//但是即使这些fragment在camera view space中的深度值z随xy变化是值变化是很大的，
    //但他们在light space 中的z值(shadow map中的值)却没变或变化很小，这是因为shadow map分辨率低，采样率低导致精度低，不能准确的记录这些细微的变化

    // calculate bias (based on depth map resolution and slope)  vec3 lightDir = normalize(uLightPos);
    vec3 lightDir = -normalize(light_dir);
    vec3 n = normalize(normal);
    float bias = max(0.005 * (1.0 - dot(n, lightDir)), 0.001);
    return  bias;
}

float useShadowMap(sampler2D shadow_map,vec4 shadow_coord){
    float bias = Bias();
    vec4 depth_pack = texture(shadow_map,shadow_coord.xy);
//    float depth_unpack = unpack(depth_pack);
    float depth_unpack = depth_pack.x;
    if(depth_unpack > shadow_coord.z -bias){
        return 1.f;
    }
    return 0.f;
}
vec3 blinnPhong(){
    vec3 color = texture(AlbedoMap,tex_coord).rgb;
    if(color == vec3(0.f)){
        color = vec3(0.7f);
        return color;
    }
    color = pow(color,vec3(2.2));

    vec3 ambient = 0.05*color;

    vec3 n = normalize(normal);
    float diff = max(dot(-light_dir,n),0.f);
    vec3 light_atten_coff = 0.4 * vec3(2.f,2.f,2.f);
    vec3 diffuse = diff * color * light_atten_coff;

    vec3 view_dir = normalize(camera_pos- position);
    vec3 half_dir = normalize(view_dir + -light_dir);
    float spec = pow(max(dot(half_dir,n),0.f),32.f);
    vec3 specular =  0.2*spec * light_atten_coff;

    vec3 radiance = ambient + diffuse + specular;

    vec3 phong_color = pow(radiance,vec3(1.f/2.2f));
    return phong_color;

}
float textureProj(vec4 shadowCoord, vec2 off)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
        float bias = Bias();
		float dist = texture( ShadowMap, shadowCoord.st + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z - bias) 
		{
			shadow = 0.05;
		}
	}
	return shadow;
}
float textureProjDepth(vec4 shadowCoord, vec2 off)
{
    float depth = 0.0;
    if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 )
    {
        float bias = Bias();
        float dist = texture( ShadowMap, shadowCoord.st + off ).r;
        if ( shadowCoord.w > 0.0 && dist < shadowCoord.z - bias)
        {
            depth = dist;
        }
    }
    return depth;
}

//使用pcf会导致凹陷的三角面片会产生额外的阴影
//此处使用了简单的均匀采样 使用一些随机采样效果会更好 比如poission disk采样

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

float filterPCSS(vec4 sc){
    ivec2 texDim = textureSize(ShadowMap, 0);
    float scale = 1.0;
    float dx = scale * 1.0 / float(texDim.x);
    float dy = scale * 1.0 / float(texDim.y);

    float depth = 0.0;
    int count = 0;
    int range = 1;

    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            depth += textureProjDepth(sc, vec2(dx*x, dy*y));
            count++;
        }
    }
    depth /= count;
    if(depth < 0.001f) return 1.f;

    scale = (sc.z - depth) / depth;
    dx = scale * 1.0 / float(texDim.x);
    dy = scale * 1.0 / float(texDim.y);

    float shadowFactor = 0.0;
    count = 0;
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
void main() {

    float visibility;
    vec3 proj_coord = pos_from_light.xyz / pos_from_light.w;
    vec3 shadowCoord = proj_coord*0.5f +0.5f;
    if(shadow_mode == 1)
        visibility = filterPCF(vec4(shadowCoord,1.f));
    else if(shadow_mode == 2)
        visibility = filterPCSS(vec4(shadowCoord,1.f));
    else if(shadow_mode == 0)
        visibility = useShadowMap(ShadowMap,vec4(shadowCoord,1.f));
    else
        visibility = 1.f;
    vec3 phongColor = blinnPhong();

    frag_color = vec4(phongColor * visibility,1.f);
}
