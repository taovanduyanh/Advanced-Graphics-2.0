#version 430 core
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
shared uint numNormalsPassed;

void main() {
    vec3 vertexNormal = normalize(normalsSSBO[facesSSBO[gl_WorkGroupID.x].normalsIndices[gl_LocalInvocationID.x]]);
    
    if (dot(normalize(cameraDirection), vertexNormal) < 0.0) {
        atomicAdd(numNormalsPassed, 1);
    }
    memoryBarrierShared();

    if (numNormalsPassed == 3) {
        idSSBO[gl_WorkGroupID.x] = int(gl_WorkGroupID.x); 
    }
}