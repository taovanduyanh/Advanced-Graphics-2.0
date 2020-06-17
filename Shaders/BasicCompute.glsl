#version 430 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

layout(std430, binding = 0) buffer Colours {
    vec4 colourSSBO[];
};

layout(local_size_variable) in;

shared vec4 temp;

void main() {
    temp = colourSSBO[0];
    memoryBarrierShared();
    if (temp.y < 1.0) {
        temp.y += 0.0005;
    }
    else if (temp.z < 1.0) {
       temp.z += 0.0005;
    }
    colourSSBO[0] = temp;
    //posSSBO[1] = vec3(0.5, -0.5, 0.0);
    //posSSBO[2] = vec3(-0.5, -0.5, 0.0);
}