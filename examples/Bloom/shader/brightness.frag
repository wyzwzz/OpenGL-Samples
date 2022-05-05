#version 460 core
layout(location = 0) out vec4 oFragColor;
uniform sampler2D renderTexture;
uniform float threshold;
uniform vec2 resolution;
float luminance(vec3 color) {
    return dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
}
void main() {
    vec2 uv = gl_FragCoord.xy / resolution;
    vec4 color = texture(renderTexture,uv);
    float l = luminance(color.rgb);
    if(l > threshold){
        oFragColor = color;
    }
    else{
        oFragColor = vec4(0.0,0.0,0.0,1.0);
    }
}
