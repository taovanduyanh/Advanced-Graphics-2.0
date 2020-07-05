#version 460 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

struct Sphere {
    vec4 center;
    uint numFaces;
    float radius;
    int facesID[2];
};

layout(std430, binding = 0) buffer Positions {
    vec3 posSSBO[];
};

layout(std430, binding = 5) buffer ID {
    int idSSBO[];
};

layout(std430, binding = 7) buffer MiddlePoints {
    vec3 middlePointsSSBO[];
};

layout(std430, binding = 8) buffer Spheres {
    Sphere spheresSSBO[];
};

layout(local_size_variable) in;

uniform mat4 modelMatrix;

void main() {
    uint firstIndex = gl_GlobalInvocationID.x * 2;
    spheresSSBO[gl_GlobalInvocationID.x].numFaces = 0;
    vec3 temp = (middlePointsSSBO[firstIndex + 1] - middlePointsSSBO[firstIndex]) * 0.5;

    for (uint i = 0; i < 2; ++i) {
        int id = idSSBO[firstIndex + i];
        if (id != -1) {
            spheresSSBO[gl_GlobalInvocationID.x].numFaces += 1;
        }
        spheresSSBO[gl_GlobalInvocationID.x].facesID[i] = id;
    }

    spheresSSBO[gl_GlobalInvocationID.x].center = modelMatrix * vec4(middlePointsSSBO[firstIndex] + temp, 1.0);
    spheresSSBO[gl_GlobalInvocationID.x].radius = length((modelMatrix * vec4(temp, 1.0)).xyz) * 0.3;
}
