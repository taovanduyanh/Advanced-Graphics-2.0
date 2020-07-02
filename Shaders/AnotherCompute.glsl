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

layout(std430, binding = 7) buffer Faces {
    Triangle facesSSBO[];
};

uniform vec3 cameraDirection;

layout(local_size_variable) in;

void main() {
    /**
    uint numPassed = 0;
    for (int i = 0; i < 3; ++i) {
        vec3 vertexNormal = normalize(normalsSSBO[facesSSBO[gl_GlobalInvocationID.x].normalsIndices[i]]);
    
        if (dot(normalize(cameraDirection), vertexNormal) > 0.0) {
            ++numPassed; 
        }
    }
    */

    vec3 a = posSSBO[facesSSBO[gl_GlobalInvocationID.x].vertIndices[1]] - posSSBO[facesSSBO[gl_GlobalInvocationID.x].vertIndices[0]];
    vec3 b = posSSBO[facesSSBO[gl_GlobalInvocationID.x].vertIndices[2]] - posSSBO[facesSSBO[gl_GlobalInvocationID.x].vertIndices[0]];
    vec3 normal = cross(b, a);
    if (dot(normalize(cameraDirection), normalize(normal)) <= 0.0) {
        idSSBO[gl_GlobalInvocationID.x] = int(gl_GlobalInvocationID.x); 
    }
    /**
    if (numPassed == 3) {
       
    }
    */
}