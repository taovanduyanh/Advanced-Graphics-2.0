#version 150 core

uniform mat4 modelMatrix;
uniform mat4 projMatrix;
uniform mat4 viewMatrix;
uniform vec4 nodeColour;

in vec3 position;
in vec2 texCoord;

out Vertex {
    vec2 texCoord;
    vec4 colour;
}   OUT;

void main() {
    gl_Position = projMatrix * viewMatrix * modelMatrix * vec4(position, 1.0f);
    OUT.texCoord = texCoord;
    OUT.colour = nodeColour;
}