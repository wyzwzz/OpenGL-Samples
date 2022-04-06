#version 430 core
out vec4 frag_color;
vec4 pack(float depth){
    const vec4 bitShift = vec4(1.f,256.f,256.f*256.f,256.f*256.f*256.f);
    const vec4 bitMask = vec4(1.f/256.f,1.f/256.f,1.f/256.f,0.f);
    vec4 rgbaDepth = fract(depth*bitShift);
    rgbaDepth -= rgbaDepth.gbaa * bitMask;
    return rgbaDepth;
}

void main() {
//    frag_color = pack(gl_FragCoord.z);
    frag_color = vec4(gl_FragCoord.z);
//    frag_color = vec4(1.f,0.f,1.f,1.f);
}
