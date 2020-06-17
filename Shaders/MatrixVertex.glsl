#version 430 core

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

in vec3 position;
in vec4 colour;

out Vertex {
    vec4 colour;
}   OUT;

layout(std430, binding = 0) buffer Pos {
	vec4 posSSBO[];
};

void main() {
    mat4 mvp = projMatrix * viewMatrix * modelMatrix;
    gl_Position = mvp * posSSBO[gl_VertexID];
    OUT.colour = colour;
}