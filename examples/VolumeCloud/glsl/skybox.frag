#version 460 core
layout(location = 0) in vec3 inWorldPos;
layout(location = 0) out vec4 oFragColor;

uniform samplerCube EnvironmentMap;

void main(){
    vec3 env_color = texture(environmentMap,inWorldPos,0.0).rgb;

    oFragColor = vec4(env_color,1.f);
}