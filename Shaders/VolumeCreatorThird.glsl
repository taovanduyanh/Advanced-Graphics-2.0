#version 460 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

// further testing 4..
struct Node {
    uint depthIndex;
    uint nodeIndex;
};

layout(std430, binding = 9) buffer ParentNodes {
    float parentNodesValues[][2];
};

layout(std430, binding = 7) buffer RootNode {
    float rootNodeValues[][2];
};

layout(local_size_variable) in;

const uint numPlaneNormals = 43;
uniform uint numGroups;

void main() {
    // the d values to calculate the bounding volume..
    rootNodeValues[gl_GlobalInvocationID.y][0] = 1.0 / 0.0;
    rootNodeValues[gl_GlobalInvocationID.y][1] = -1.0 / 0.0;

    for (int i = 0; i < numGroups; ++i) {
        uint id = numPlaneNormals * i + gl_GlobalInvocationID.y;
        rootNodeValues[gl_GlobalInvocationID.y][0] = min(parentNodesValues[id][0], rootNodeValues[gl_GlobalInvocationID.y][0]);
        rootNodeValues[gl_GlobalInvocationID.y][1] = max(parentNodesValues[id][1], rootNodeValues[gl_GlobalInvocationID.y][1]);
    }

    memoryBarrierBuffer();
    barrier();
}