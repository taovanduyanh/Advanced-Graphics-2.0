#version 460 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

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

layout(std430, binding = 7) buffer PlaneDs {
    float dSSBO[][2];
};

const vec3 kSNormals[7] = vec3[7](vec3(1, 0, 0),
                                  vec3(0, 1, 0),
                                  vec3(0, 0, 1),
                                  vec3(sqrt(3) / 3),
                                  vec3(-sqrt(3) / 3, sqrt(3) / 3, sqrt(3) / 3),
                                  vec3(-sqrt(3) / 3, -sqrt(3) / 3, sqrt(3) / 3),
                                  vec3(sqrt(3) / 3, -sqrt(3) / 3, sqrt(3) / 3));

const vec3 defaultNormals[3] = vec3[3]
(
    vec3(1, 0, 0),
    vec3(0, 1, 0),
    vec3(0, 0, 1)
);

uniform uint numVisibleFaces;
uniform mat4 modelMatrix;

layout(local_size_variable) in;

void main() {
    // further testing 6..
    if (gl_GlobalInvocationID.x >= numVisibleFaces) {
        return;
    }

    dSSBO[gl_GlobalInvocationID.x * kSNormals.length() + gl_GlobalInvocationID.y][0] = 1.0 / 0.0;
    dSSBO[gl_GlobalInvocationID.x * kSNormals.length() + gl_GlobalInvocationID.y][1] = -1.0 / 0.0;

    int triangleID = idSSBO[gl_GlobalInvocationID.x];

    for (uint i = 0; i < 3; ++i) {
        vec3 currVertex = (modelMatrix * posSSBO[facesSSBO[triangleID].vertIndices[i]]).xyz;
        float d = dot(currVertex, kSNormals[gl_GlobalInvocationID.y]);

        dSSBO[gl_GlobalInvocationID.x * kSNormals.length() + gl_GlobalInvocationID.y][0] = min(d, dSSBO[gl_GlobalInvocationID.x * kSNormals.length() + gl_GlobalInvocationID.y][0]);
        dSSBO[gl_GlobalInvocationID.x * kSNormals.length() + gl_GlobalInvocationID.y][1] = max(d, dSSBO[gl_GlobalInvocationID.x * kSNormals.length() + gl_GlobalInvocationID.y][1]);
    }
}