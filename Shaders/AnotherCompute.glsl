#version 430 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

layout(std430, binding = 0) buffer Positions {
    vec3 posSSBO[];
};

layout(std430, binding = 3) buffer Normals {
    vec3 normalsSSBO[];
};

layout(std430, binding = 6) buffer ID {
    int idSSBO[];
};

uniform mat4 modelMatrix;
uniform vec3 cameraPos;

layout(local_size_variable) in;
layout(binding = 0, offset = 0) uniform atomic_uint idCount;

void main() {
    vec3 cameraDirection = normalize(cameraPos - (modelMatrix * vec4(posSSBO[gl_LocalInvocationID.x], 1.0)).xyz);
    if (dot(cameraDirection, normalize(normalsSSBO[gl_LocalInvocationID.x])) < 0.0) {
        idSSBO[gl_LocalInvocationID.x] = int(gl_LocalInvocationID.x);
        atomicCounterIncrement(idCount);
    }

    memoryBarrierShared();
}