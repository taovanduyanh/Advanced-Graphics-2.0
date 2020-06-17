#version 150 core

in Vertex {
    vec3 normal;
}   IN;

uniform samplerCube cubeTex;

out vec4 fragColour;

void main() {
    fragColour = texture(cubeTex, normalize(IN.normal));
}