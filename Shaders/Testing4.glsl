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

layout(std430, binding = 15) buffer PlaneDs {
    float dSSBO[39][2];
};

layout(local_size_variable) in;

const vec3 kSNormals[7] = vec3[7](vec3(1, 0, 0),
                                  vec3(0, 1, 0),
                                  vec3(0, 0, 1),
                                  vec3(sqrt(3) / 3),
                                  vec3(-sqrt(3) / 3, sqrt(3) / 3, sqrt(3) / 3),
                                  vec3(-sqrt(3) / 3, -sqrt(3) / 3, sqrt(3) / 3),
                                  vec3(sqrt(3) / 3, -sqrt(3) / 3, sqrt(3) / 3));

const vec3 nearHalfIcoNormals[39] = vec3[39](
                                         vec3(1.0, 0.0, 0.0),
                                         vec3(0.0, 1.0, 0.0),
                                         vec3(0.0, 0.0, 1.0),

                                         vec3(0.471318, -0.661688, 0.583119),
                                         vec3(0.187593, -0.794659, 0.577344),
                                         vec3(-0.0385481, -0.661688, 0.748788),
                                         vec3(0.102381, -0.943523, 0.315091),

                                         vec3(0.700227, -0.661689, 0.268049),
                                         vec3(-0.408937, -0.661687, 0.628443),
                                         vec3(-0.491118, -0.794658, 0.356822),
                                         vec3(-0.724045, -0.661693, 0.194734),

                                         vec3(-0.268034, -0.943523, 0.194738),
                                         vec3(0.904981, -0.330393, 0.268049),
                                         vec3(0.0247248, -0.330396, 0.943518),
                                         vec3(0.303532, -0.187597, 0.934171),

                                         vec3(0.306569, 0.125652, 0.943518),
                                         vec3(0.534591, -0.330394, 0.777851),
                                         vec3(-0.889698, -0.330386, 0.315091),
                                         vec3(-0.794656, -0.187594, 0.577348),

                                         vec3(-0.802607, 0.125649, 0.583125),
                                         vec3(-0.574582, -0.330397, 0.748794),
                                         vec3(0.889697, 0.330386, 0.315093),
                                         vec3(0.794655, 0.187595, 0.577349),

                                         vec3(0.574583, 0.330396, 0.748794),
                                         vec3(0.802606, -0.125648, 0.583126),
                                         vec3(-0.0247264, 0.330397, 0.943518),
                                         vec3(-0.303531, 0.187599, 0.934171),

                                         vec3(-0.534589, 0.330396, 0.777851),
                                         vec3(-0.306569, -0.125652, 0.943519),
                                         vec3(-0.904981, 0.330393, 0.268049),
                                         vec3(0.408938, 0.661687, 0.628442),

                                         vec3(0.491118, 0.794657, 0.356823),
                                         vec3(0.268034, 0.943523, 0.194738),
                                         vec3(0.724044, 0.661693, 0.194736),
                                         vec3(-0.471317, 0.661688, 0.583121),
                                         
                                         vec3(-0.187594, 0.794658, 0.577345),
                                         vec3(-0.102381, 0.943523, 0.315091),
                                         vec3(0.0385468, 0.661687, 0.748788),
                                         vec3(-0.700228, 0.661688, 0.26805));

uniform uint numVisibleFaces;
uniform mat4 modelMatrix;

void main() {
    dSSBO[gl_GlobalInvocationID.y][0] = 1.0 / 0.0;
    dSSBO[gl_GlobalInvocationID.y][1] = -1.0 / 0.0;

    memoryBarrierBuffer();
    barrier();

    for (int i = 0; i < numVisibleFaces; ++i) {
        int id = idSSBO[i];
        for (int j = 0; j < 3; ++j) {
            vec3 currPos = (modelMatrix * posSSBO[facesSSBO[id].vertIndices[j]]).xyz;
            float d = dot(nearHalfIcoNormals[gl_GlobalInvocationID.y], currPos);

            dSSBO[gl_GlobalInvocationID.y][0] = min(d, dSSBO[gl_GlobalInvocationID.y][0]);
            dSSBO[gl_GlobalInvocationID.y][1] = max(d, dSSBO[gl_GlobalInvocationID.y][1]);
        }
    }
    
    memoryBarrierBuffer();
    barrier();
}