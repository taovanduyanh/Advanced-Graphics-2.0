#version 460 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

// further testing 3..
layout(std430, binding = 10) buffer Temp1 {
    float temp1[][2];
};

layout(std430, binding = 20) buffer Temp2 {
    float temp2[][2];
};

layout(local_size_variable) in;

const uint numPlaneNormals = 39;
uniform uint numFacesPerGroup;
uniform uint maxIndex;

void main() {
    // further testing 4..
    temp2[gl_GlobalInvocationID.x * numPlaneNormals + gl_GlobalInvocationID.y][0] = 1.0 / 0.0;
    temp2[gl_GlobalInvocationID.x * numPlaneNormals + gl_GlobalInvocationID.y][1] = -1.0 / 0.0;

    memoryBarrierBuffer();
    barrier();

    for (int i = 0; i < numFacesPerGroup; ++i) {
        uint id = (gl_GlobalInvocationID.x * numFacesPerGroup + i) * numPlaneNormals + gl_GlobalInvocationID.y;
        if (id >= maxIndex) {
            break;
        }
        float dMin = temp1[id][0];
        float dMax = temp1[id][1];

        temp2[gl_GlobalInvocationID.x * numPlaneNormals + gl_GlobalInvocationID.y][0] 
        = min(dMin, temp2[gl_GlobalInvocationID.x * numPlaneNormals + gl_GlobalInvocationID.y][0]);
        temp2[gl_GlobalInvocationID.x * numPlaneNormals + gl_GlobalInvocationID.y][1]
        = max(dMax, temp2[gl_GlobalInvocationID.x * numPlaneNormals + gl_GlobalInvocationID.y][1]);
    }
    
    memoryBarrierBuffer();
    barrier();
}