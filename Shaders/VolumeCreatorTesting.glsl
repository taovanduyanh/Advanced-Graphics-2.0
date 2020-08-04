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

layout(std430, binding = 20) buffer VisibleFacesNormals {
    vec4 visibleNormals[];
};

layout(std430, binding = 7) buffer PlaneDs {
    float dSSBO[][2];
};

uniform uint numVisibleFaces;
uniform mat4 modelMatrix;

layout(local_size_variable) in;

void main() {
    for (int i = 0; i < numVisibleFaces; ++i) {
        dSSBO[i][0] = 1.0 / 0.0;  // infinity
        dSSBO[i][1] = -1.0 / 0.0; // neg infinity
    }

    // further testing 5..
    for (int i = 0; i < numVisibleFaces; ++i) {
        int triangleID = idSSBO[i];
        for (int j = 0; j < 3; ++j) {
            vec3 currVertex = (modelMatrix * posSSBO[facesSSBO[triangleID].vertIndices[j]]).xyz;
            float d = dot(currVertex, visibleNormals[triangleID].xyz);

            dSSBO[i][0] = min(d, dSSBO[i][0]);
        }
    }

    
    for (int i = 0; i < numVisibleFaces; ++i) {
        int currNormalID = idSSBO[i];
        for (int j = 0; j < numVisibleFaces; ++j) {
            int triangleID = idSSBO[j];
            for (int k = 0; k < 3; ++k) {
                vec3 currVertex = (modelMatrix * posSSBO[facesSSBO[triangleID].vertIndices[k]]).xyz;
                float d = dot(currVertex, visibleNormals[currNormalID].xyz);

                //dSSBO[i][0] = min(d, dSSBO[i][0]);
                dSSBO[i][1] = max(d, dSSBO[i][1]);
            }
        }
    }
   
    memoryBarrierBuffer();
    barrier();
}