#version 460 core
layout(location = 0) in vec3 inWorldPos;
layout(location = 0) out vec4 outFragColor;

uniform samplerCube environmentMap;

void main(){
    vec3 env_color = textureLod(environmentMap,inWorldPos,0.0).rgb;

    env_color = env_color / (env_color + vec3(1.f));
    env_color = pow(env_color,vec3(1.f/2.2f));

    outFragColor = vec4(env_color,1.f);
}