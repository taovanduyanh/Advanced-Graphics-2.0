#version 460 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

// further testing 4..
layout(std430, binding = 20) buffer Temp2 {
    float temp2[][2];
};

layout(std430, binding = 15) buffer PlaneDs {
    float dSSBO[39][2];
};

layout(local_size_variable) in;

const uint numPlaneNormals = 39;
uniform uint numGroups;

void main() {
    for (int i = 0; i < numGroups; ++i) {
        uint id = numPlaneNormals * i + gl_GlobalInvocationID.y;
        dSSBO[gl_GlobalInvocationID.y][0] = min(temp2[id][0], dSSBO[gl_GlobalInvocationID.y][0]);
        dSSBO[gl_GlobalInvocationID.y][1] = max(temp2[id][1], dSSBO[gl_GlobalInvocationID.y][1]);
    }

    memoryBarrierBuffer();
    barrier();
}