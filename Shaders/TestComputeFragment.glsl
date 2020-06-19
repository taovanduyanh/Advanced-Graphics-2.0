#version 430 core

layout(std430, binding = 0) buffer Colour {
    vec4 coloursSSBO[];
};

in Vertex {
    vec4 colour;
}   IN;

out vec4 fragColour;

void main() {
    float test = gl_SamplePosition.x * 784 + gl_SamplePosition.y * 561 * 784;
    fragColour = coloursSSBO[int(test)];
}
