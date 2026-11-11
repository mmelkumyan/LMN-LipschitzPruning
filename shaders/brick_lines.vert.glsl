#version 450

#include "extensions.glsl"
#include "../include/constants.h"
#include "common.glsl"

//a push constant is a small block of data that you write directly into the command buffer with vkCmdPushConstants
//vulkan glsl only allows uniforms inside blocks (uniform buffer boudn with a descriptor set, or a push constant block)
layout(push_constant) uniform PushConstant{
    vec4 color;
    FloatArrayRef AABB;
    Mat4Ref MVP;
    vec4 yParams; //slice min, slice max, color range low, color range high
};

ivec2 cornerTable[12] = ivec2[](
    ivec2(0, 1), ivec2(2, 3), ivec2(4, 5), ivec2(6, 7),
    ivec2(0, 2), ivec2(1, 3), ivec2(4, 6), ivec2(5, 7),
    ivec2(0, 4), ivec2(1, 5), ivec2(2, 6), ivec2(3, 7)
);

layout(location = 0) out vec3 vertColor;

void main(){
    //drawing wireframes

    int brick = gl_VertexIndex / 24;
    int localIndex = gl_VertexIndex % 24;

    int edge = localIndex/2; //which edge of the local box we'll pick
    int endpoint = localIndex%2; //which component of the local box

    ivec2 cornerIdxPair = cornerTable[edge];
    int cornerIdx = cornerIdxPair[endpoint];

    int base = brick * 6; //6 floats per brick min(xyz) max(xyz)
    vec3 bmin = vec3(AABB.tab[base + 0], AABB.tab[base+1], AABB.tab[base+2]);
    vec3 bmax = vec3(AABB.tab[base + 3], AABB.tab[base+4], AABB.tab[base+5]);
    
    float bcenterheight = (bmin.y + bmax.y)/2.0;

    //discard any brick wireframes that lie outside the slice range
    if(bcenterheight < yParams.x || bcenterheight > yParams.y){
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0); //setting it outside of the viewport so it gets culled
        return;
    }

    float centerNormalized = clamp((bcenterheight - yParams.z) / (yParams.w - yParams.z), 0.0, 1.0);

    if(centerNormalized < 0.5){
        vertColor = mix(vec3(0.0,0.0,1.0), vec3(0.0,1.0,0.0), centerNormalized*2);
    }
    else{
        vertColor = mix(vec3(0.0,1.0,0.0), vec3(1.0,0.0,0.0), centerNormalized*2 - 1);
    }


    //turn corner number into actual position
    //corner = 5: binary 101 = (bmax.x, bmin.y, bmax.z)
    vec3 cornerPos;
    cornerPos.x = ((cornerIdx >> 0) & 1) == 0 ? bmin.x : bmax.x;
    cornerPos.y = ((cornerIdx >> 1) & 1) == 0 ? bmin.y : bmax.y;
    cornerPos.z = ((cornerIdx >> 2) & 1) == 0 ? bmin.z : bmax.z;

    gl_Position = MVP.m * vec4(cornerPos, 1.0);

    //Vulkan's clip sapce has +Y pointing down, GLM was written for OpenGL, so glm::perspective assumes +Y is up
    gl_Position.y *= -1.0;
}