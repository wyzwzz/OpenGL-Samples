#version 460 core
layout(location = 0) in vec3 inWorldPos;
layout(location = 0) out vec4 outFragColor;

uniform sampler2D equirectangularMap;

// 1.f/(2*PI),1.f/PI
const vec2 invAtan = vec2(0.1591,0.3183);
vec2 SampleSphericalMap(vec3 v){
    //v是单位方向向量 也就是球上的一点 根据该坐标计算raw和pitch角度 再转换到0-1就是uv坐标
    //atan:(-PI,PI) asin(-PI/2,PI/2)
    vec2 uv = vec2(atan(v.z,v.x),asin(v.y));
    uv *= invAtan;//(-0.5,0.5)
    uv += 0.5f;//(0,1)
    return uv;
}

void main(){
    //投影转换: 立方体-->球-->全景图
    vec2 uv = SampleSphericalMap(normalize(inWorldPos));
    vec3 color = texture(equirectangularMap,uv).rgb;
    outFragColor = vec4(color,1.f);
}
