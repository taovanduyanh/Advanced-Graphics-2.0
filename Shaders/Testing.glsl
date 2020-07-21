#version 430 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

struct Triangle {
    uint vertIndices[3];
    uint texIndices[3];
    uint normalsIndices[3];
};

struct BBox {
    vec4 bounds[2];
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

layout(std430, binding = 12) buffer BoundingBox {
    BBox parentBoxSSBO;
};

layout(local_size_variable) in;
layout(binding = 0, offset = 0) uniform atomic_uint counter;

uniform mat4 modelMatrix;

void main() {
    vec3 min = vec3(1.0 / 0.0);     // infinity
    vec3 max = vec3(-1.0 / 0.0);    // neg infinity

    for (int i = 0; i < atomicCounter(counter); ++i) {
        int id = idSSBO[i];
        for (int j = 0; j < 3; ++j) {
            vec3 currPos = (modelMatrix * posSSBO[facesSSBO[id].vertIndices[j]]).xyz;

            // look for min first..
            if (min.x > currPos.x) {
                min.x = currPos.x;
            }
            if (min.y > currPos.y) {
                min.y = currPos.y;
            }
            if (min.z > currPos.z) {
                min.z = currPos.z;
            }

            // look for max now..
            if (max.x < currPos.x) {
                max.x = currPos.x;
            }
            if (max.y < currPos.y) {
                max.y = currPos.y;
            }
            if (max.z < currPos.z) {
                max.z = currPos.z;
            }
        }
    }

    parentBoxSSBO.bounds[0] = vec4(min, 1.0);
    parentBoxSSBO.bounds[1] = vec4(max, 1.0);
}