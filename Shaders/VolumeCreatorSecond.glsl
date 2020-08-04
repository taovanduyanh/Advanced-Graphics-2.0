#version 460 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

// further testing 3..
layout(std430, binding = 8) buffer TempDsFirst {
    float tempFirst[][2];
};

layout(std430, binding = 9) buffer TempDsSecond {
    float tempSecond[][2];
};

layout(local_size_variable) in;

const uint numPlaneNormals = 19;
uniform uint numFacesPerGroup;
uniform uint maxIndex;

void main() {
    // further testing 4..
    tempSecond[gl_GlobalInvocationID.x * numPlaneNormals + gl_GlobalInvocationID.y][0] = 1.0 / 0.0;
    tempSecond[gl_GlobalInvocationID.x * numPlaneNormals + gl_GlobalInvocationID.y][1] = -1.0 / 0.0;

    for (int i = 0; i < numFacesPerGroup; ++i) {
        uint id = (gl_GlobalInvocationID.x * numFacesPerGroup + i) * numPlaneNormals + gl_GlobalInvocationID.y;
        if (id >= maxIndex) {
            break;
        }
        float dMin = tempFirst[id][0];
        float dMax = tempFirst[id][1];

        tempSecond[gl_GlobalInvocationID.x * numPlaneNormals + gl_GlobalInvocationID.y][0] 
        = min(dMin, tempSecond[gl_GlobalInvocationID.x * numPlaneNormals + gl_GlobalInvocationID.y][0]);
        tempSecond[gl_GlobalInvocationID.x * numPlaneNormals + gl_GlobalInvocationID.y][1]
        = max(dMax, tempSecond[gl_GlobalInvocationID.x * numPlaneNormals + gl_GlobalInvocationID.y][1]);
    }
    
    memoryBarrierBuffer();
    barrier();
}