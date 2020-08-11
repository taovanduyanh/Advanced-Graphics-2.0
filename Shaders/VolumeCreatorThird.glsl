#version 460 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

// further testing 4..
layout(std430, binding = 9) buffer TempDsSecond {
    float tempSecond[][2];
};

layout(std430, binding = 7) buffer PlaneDs {
    float dSSBO[][2];
};

layout(local_size_variable) in;

const uint numPlaneNormals = 43;
uniform uint numGroups;

void main() {
    dSSBO[gl_GlobalInvocationID.y][0] = 1.0 / 0.0;
    dSSBO[gl_GlobalInvocationID.y][1] = -1.0 / 0.0;
    
    for (int i = 0; i < numGroups; ++i) {
        uint id = numPlaneNormals * i + gl_GlobalInvocationID.y;
        dSSBO[gl_GlobalInvocationID.y][0] = min(tempSecond[id][0], dSSBO[gl_GlobalInvocationID.y][0]);
        dSSBO[gl_GlobalInvocationID.y][1] = max(tempSecond[id][1], dSSBO[gl_GlobalInvocationID.y][1]);
    }

    memoryBarrierBuffer();
    barrier();
}