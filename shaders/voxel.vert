#version 450

layout(location=0) in vec3 inPos;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inUV;
layout(location=3) in vec4 inTile;
layout(location=4) in vec3 inColor;

layout(location=0) out vec3 vWorld;
layout(location=1) out vec3 vN;
layout(location=2) out vec4 vTile;
layout(location=3) out float vAO;

layout(push_constant) uniform Push {
    mat4 uMVP;
    vec4 uLight;
    vec4 uCamPos;
} pc;

void main(){
    vWorld = inPos;
    vN     = inNormal;
    vTile  = inTile;
    vAO    = inColor.r;

    gl_Position = pc.uMVP * vec4(inPos, 1.0);
}
