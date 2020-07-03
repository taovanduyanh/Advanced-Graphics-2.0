#version 460 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

struct Triangle {
    uint vertIndices[3];
    uint texIndices[3];
    uint normalsIndices[3];
};

layout(std430, binding = 0) buffer Positions {
    vec3 posSSBO[];
};

layout(std430, binding = 5) buffer ID {
    int idSSBO[];
};

layout(std430, binding = 6) buffer Faces {
    Triangle facesSSBO[];
};

layout(std430, binding = 7) buffer MiddlePoints {
    vec3 middlePointsSSBO[];
};

uniform vec3 cameraDirection;

layout(local_size_variable) in;

void main() {
    vec3 v0 = posSSBO[facesSSBO[gl_GlobalInvocationID.x].vertIndices[0]];
    vec3 v1 = posSSBO[facesSSBO[gl_GlobalInvocationID.x].vertIndices[1]];
    vec3 v2 = posSSBO[facesSSBO[gl_GlobalInvocationID.x].vertIndices[2]];

    vec3 a = v1 - v0;
    vec3 b = v2 - v0;
    vec3 normal = normalize(cross(a, b));

    if (dot(normalize(cameraDirection), normal) < 0) {
        idSSBO[gl_GlobalInvocationID.x] = int(gl_GlobalInvocationID.x); 
    }

    middlePointsSSBO[gl_GlobalInvocationID.x] = vec3((v0.x + v1.x + v2.x) / 3,
                                                     (v0.y + v1.y + v2.y) / 3, 
                                                     (v0.z + v1.z + v2.z) / 3);

}