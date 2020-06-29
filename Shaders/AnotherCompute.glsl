#version 430 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

struct Triangle {
    uint vertIndices[3];
    uint texIndices[3];
    uint normalsIndices[3];
};

layout(std430, binding = 3) buffer Normals {
    vec3 normalsSSBO[];
};

layout(std430, binding = 5) buffer ID {
    int idSSBO[];
};

layout(std430, binding = 7) buffer Faces {
    Triangle facesSSBO[];
};

uniform mat4 modelMatrix;
uniform vec3 cameraPos;

layout(local_size_variable) in;
layout(binding = 0, offset = 0) uniform atomic_uint idCount;

void main() {
    vec3 cameraDirection = normalize(cameraPos - modelMatrix[3].xyz);
    vec3 vertexNormal = normalize(normalsSSBO[facesSSBO[gl_WorkGroupID.x].normalsIndices[gl_LocalInvocationID.x]]);

    if (dot(cameraDirection, vertexNormal) < 0.0) {
        idSSBO[gl_LocalInvocationID.x] = int(facesSSBO[gl_WorkGroupID.x].vertIndices[gl_LocalInvocationID.x]);
        atomicCounterIncrement(idCount);
    }

    memoryBarrierShared();
}