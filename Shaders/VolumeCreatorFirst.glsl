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

layout(std430, binding = 8) buffer TempDsFirst {
    float tempFirst[][2];
};

layout(local_size_variable) in;

const vec3 kSNormals[7] = vec3[7](vec3(1, 0, 0),
                                  vec3(0, 1, 0),
                                  vec3(0, 0, 1),
                                  vec3(sqrt(3) / 3),
                                  vec3(-sqrt(3) / 3, sqrt(3) / 3, sqrt(3) / 3),
                                  vec3(-sqrt(3) / 3, -sqrt(3) / 3, sqrt(3) / 3),
                                  vec3(sqrt(3) / 3, -sqrt(3) / 3, sqrt(3) / 3));

const vec3 nearHalfIcoNormals[19] = vec3[19]
(
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),

    vec3(0.700227, -0.661689, 0.268049),
    vec3(-0.700228, 0.661688, 0.26805),

    vec3(-0.724045, -0.661693, 0.194734),
    vec3(0.724044, 0.661693, 0.194736),

    vec3(0.904981, -0.330393, 0.268049),
    vec3(-0.904981, 0.330393, 0.268049),

    vec3(0.0247248, -0.330396, 0.943518),
    vec3(-0.0247264, 0.330397, 0.943518),

    vec3(0.534591, -0.330394, 0.777851),
    vec3(-0.534589, 0.330396, 0.777851),

    vec3(-0.889698, -0.330386, 0.315091),
    vec3(0.889697, 0.330386, 0.315093),

    vec3(0.794655, 0.187595, 0.577349),
    vec3(-0.794656, -0.187594, 0.577348),

    vec3(-0.802607, 0.125649, 0.583125),
    vec3(0.802606, -0.125648, 0.583126)                                
);

                                         

/*
const vec3 nearHalfIcoNormals[17] = vec3[17](
    vec3(-0.270598, 0.707107, -0.653281),
    vec3(0.382683, 0, -0.923879),
    vec3(0.31246, 0.57735, -0.754345),
    vec3(0.653282, 0.707107, -0.270598),
    vec3(0, 1, 0),
    vec3(1, 0, 0),
    vec3(0, 0, 1),
    vec3(-0.653282, 0.707107, 0.270598),
    vec3(-0.31246, 0.57735, 0.754344),
    vec3(-0.92388, 0, 0.382683),
    vec3(-0.754344, 0.57735, -0.31246),
    vec3(0.653281, -0.707107, -0.270598),
    vec3(0.92388, 0, 0.382683),
    vec3(0.270598, 0.707107, 0.653281),
    vec3(0.754344, 0.57735, 0.31246),
    vec3(-0.653281, -0.707107, 0.270598),
    vec3(0.382683, 0, 0.923879)
);
*/

uniform uint numVisibleFaces;
uniform mat4 modelMatrix;

void main() {
    // further testing 4..
    if (gl_GlobalInvocationID.x >= numVisibleFaces) {
        return;
    }

    tempFirst[gl_GlobalInvocationID.x * nearHalfIcoNormals.length() + gl_GlobalInvocationID.y][0] = 1.0 / 0.0;
    tempFirst[gl_GlobalInvocationID.x * nearHalfIcoNormals.length() + gl_GlobalInvocationID.y][1] = -1.0 / 0.0;

    memoryBarrierBuffer();
    barrier();
    
    int id = idSSBO[gl_GlobalInvocationID.x];
    for (int i = 0; i < 3; ++i) {
        vec3 currVertex = (modelMatrix * posSSBO[facesSSBO[id].vertIndices[i]]).xyz;
        float d = dot(currVertex, nearHalfIcoNormals[gl_GlobalInvocationID.y]);

        tempFirst[gl_GlobalInvocationID.x * nearHalfIcoNormals.length() + gl_GlobalInvocationID.y][0] 
        = min(d, tempFirst[gl_GlobalInvocationID.x * nearHalfIcoNormals.length() + gl_GlobalInvocationID.y][0]);

        tempFirst[gl_GlobalInvocationID.x * nearHalfIcoNormals.length() + gl_GlobalInvocationID.y][1] 
        = max(d, tempFirst[gl_GlobalInvocationID.x * nearHalfIcoNormals.length() + gl_GlobalInvocationID.y][1]);
    }

    memoryBarrierBuffer();
    barrier();
}