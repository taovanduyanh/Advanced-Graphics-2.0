#version 430 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

layout(std430, binding = 0) buffer Colours {
    vec4 colourSSBO[];
};

layout(local_size_variable) in;

uniform sampler2D sceneTex;
layout(rgba32f) uniform image2D image;
uniform vec2 pixelSize;

void main() {
    vec2 texCoord = gl_GlobalInvocationID.xy * pixelSize;
    vec4 colour = texture(sceneTex, texCoord);
    uint index = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * gl_NumWorkGroups.x;
    colourSSBO[index] = colour * vec4(1.0, 0.0, 0.5, 1.0);

    // coordinate in pixels
    ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    imageStore(image, pix, colourSSBO[index]);
    memoryBarrierShared();
}

