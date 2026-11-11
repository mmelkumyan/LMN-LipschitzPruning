#version 450

#include "extensions.glsl"
#include "../include/constants.h"
#include "common.glsl"

//a push constant is a small block of data that you write directly into the command buffer with vkCmdPushConstants
layout(push_constant) uniform PushConstant{
    vec4 color;
};

layout(location = 0) in vec3 inColor;

layout(location = 0) out vec4 outColor;

void main(){
    //drawing wireframes
    outColor = vec4(inColor * color.rgb, 1.0);
}