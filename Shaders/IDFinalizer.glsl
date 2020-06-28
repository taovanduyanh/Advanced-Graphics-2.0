#version 430 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

layout(std430, binding = 6) buffer ID {
    int idSSBO[];
};

layout(std430, binding = 7) buffer FinalID {
    int finalIDSSBO[];
};

layout(local_size_variable) in;

void main() {
    int count = 0;

    for (int i = 0; i < idSSBO.length(); ++i) {
        if (idSSBO[i] != -1) {
            finalIDSSBO[count++] = idSSBO[i];
        }
    }
    memoryBarrierShared();
}