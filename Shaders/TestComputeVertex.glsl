#version 430 core

uniform mat4 modelMatrix;
uniform mat4 projMatrix;
uniform mat4 viewMatrix;

in vec3 position;
in vec4 colour;

out Vertex {
    vec4 colour;
}   OUT;

void main() {
    mat4 mvp = projMatrix * viewMatrix * modelMatrix;
    gl_Position = mvp * vec4(position, 1.0);
    OUT.colour = colour;
}