#version 460 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

#define EPSILON 0.00001

struct Triangle {
    uint vertIndices[3];
    uint texIndices[3];
    uint normalsIndices[3];
};

layout(std430, binding = 0) buffer Positions {
    vec4 posSSBO[];
};

layout(std430, binding = 5) buffer ID {
    int idSSBO[];
};

layout(std430, binding = 6) buffer Faces {
    Triangle facesSSBO[];
};

uniform mat4 modelMatrix;
uniform vec3 cameraPosition;
uniform uint numTriangles;

layout(local_size_variable) in;
layout(binding = 0, offset = 0) uniform atomic_uint counter;

void main() {
    if (gl_GlobalInvocationID.x >= numTriangles) {
        return;
    }

    vec4 v0 = modelMatrix * posSSBO[facesSSBO[gl_GlobalInvocationID.x].vertIndices[0]];
    vec4 v1 = modelMatrix * posSSBO[facesSSBO[gl_GlobalInvocationID.x].vertIndices[1]];
    vec4 v2 = modelMatrix * posSSBO[facesSSBO[gl_GlobalInvocationID.x].vertIndices[2]];

    vec4 a = v1 - v0;
    vec4 b = v2 - v0;
    vec3 normal = normalize(cross(a.xyz, b.xyz));

    // minusing the first vertex here instead of the camera's position due to the negative z thingy..
    if (dot(normalize(cameraPosition - v0.xyz), normal) >= EPSILON) {
        idSSBO[gl_GlobalInvocationID.x] = int(gl_GlobalInvocationID.x); 
        atomicCounterIncrement(counter);
    }

    memoryBarrierBuffer();
    barrier();
}