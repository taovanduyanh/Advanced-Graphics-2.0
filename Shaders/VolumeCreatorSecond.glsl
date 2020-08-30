#version 460 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

// further testing 3..
layout(std430, binding = 5) buffer ID {
    int idSSBO[];
};

layout(std430, binding = 8) buffer LeafNodes {
    float leafNodesValues[][2];
};

layout(std430, binding = 9) buffer ParentNodes {
    float parentNodesValues[][2];
};

layout(local_size_variable) in;

const uint numPlaneNormals = 43;
uniform uint numVisibleFaces;
uniform uint numFacesPerGroup;
uniform uint maxIndex;

void main() {
    // the d values to calculate the bounding volume..
    parentNodesValues[gl_GlobalInvocationID.x * numPlaneNormals + gl_GlobalInvocationID.y][0] = 1.0 / 0.0;
    parentNodesValues[gl_GlobalInvocationID.x * numPlaneNormals + gl_GlobalInvocationID.y][1] = -1.0 / 0.0;

    for (int i = 0; i < numFacesPerGroup; ++i) {
        uint id = (gl_GlobalInvocationID.x * numFacesPerGroup + i) * numPlaneNormals + gl_GlobalInvocationID.y;
        if (id >= maxIndex) {
            break;
        }

        float dMin = leafNodesValues[id][0];
        float dMax = leafNodesValues[id][1];

        parentNodesValues[gl_GlobalInvocationID.x * numPlaneNormals + gl_GlobalInvocationID.y][0] 
        = min(dMin, parentNodesValues[gl_GlobalInvocationID.x * numPlaneNormals + gl_GlobalInvocationID.y][0]);
        parentNodesValues[gl_GlobalInvocationID.x * numPlaneNormals + gl_GlobalInvocationID.y][1]
        = max(dMax, parentNodesValues[gl_GlobalInvocationID.x * numPlaneNormals + gl_GlobalInvocationID.y][1]);
    }
    
    memoryBarrierBuffer();
    barrier();
}