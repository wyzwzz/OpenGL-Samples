#version 460
layout(location = 0) in vec2 iUV;
layout(location = 0) out vec4 oFragColor;

uniform sampler2D GColor;

void main() {
    vec3 color = texture(GColor,iUV).rgb;

    oFragColor = vec4(pow(color,vec3(1.0/2.2)),1.0);

}
