#version 460 core
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

uniform vec3 cameraDirection;

layout(local_size_variable) in;

void main() {
    uint numPassed = 0;
    for (int i = 0; i < 3; ++i) {
        vec3 vertexNormal = normalize(normalsSSBO[facesSSBO[gl_GlobalInvocationID.x].normalsIndices[i]]);
    
        if (dot(normalize(cameraDirection), vertexNormal) < 0.0) {
            ++numPassed; 
        }
    }

    if (numPassed == 3) {
        idSSBO[gl_GlobalInvocationID.x] = int(gl_GlobalInvocationID.x); 
    }
}