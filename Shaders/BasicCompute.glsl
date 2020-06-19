#version 430 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

layout(std430, binding = 0) buffer Colours {
    vec4 colourSSBO[];
};

layout(local_size_variable) in;

//uniform float offset;
uniform sampler2D sceneTex;
uniform vec2 pixelSize;

//shared vec4 temp;

void main() {
    /**
    temp = colourSSBO[0];

    if (temp.y < 1.0) {
        temp.y += offset;
    }
    else if (temp.z < 1.0) {
       temp.z += offset;
    }

    memoryBarrierShared();
    colourSSBO[0] = temp;
    //posSSBO[1] = vec3(0.5, -0.5, 0.0);
    //posSSBO[2] = vec3(-0.5, -0.5, 0.0);
    */

    vec2 texCoord = gl_GlobalInvocationID.xy * pixelSize;
    vec4 colour = texture(sceneTex, texCoord);
    if (gl_GlobalInvocationID.x == 392 && gl_GlobalInvocationID.y == 280) {
        colourSSBO[0] = colour;
        colourSSBO[1] = vec4(gl_GlobalInvocationID, 1.0);
    }

    //memoryBarrierShared();
}