#version 430 core

in vec3 position;
in vec4 colour;

readonly restrict layout(std430, binding = 0) buffer Pos {
	vec4 posSSBO[];
};

out Vertex {
	vec4 colour;
}	OUT;

void main() {
	gl_Position = vec4(position, 1.0);
	OUT.colour = posSSBO[gl_VertexID];
}