#version 430 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

layout(std430, binding = 3) buffer Normals {
    vec3 normalsSSBO[];
};

layout(std430, binding = 6) buffer ID {
    int idSSBO[];
};

uniform vec3 cameraDirection;

layout(local_size_variable) in;

void main() {
    if (dot(normalize(cameraDirection), normalsSSBO[gl_LocalInvocationID.x]) <= 0.0) {
        idSSBO[gl_LocalInvocationID.x] = int(gl_LocalInvocationID.x);
    }
    memoryBarrierShared();
}