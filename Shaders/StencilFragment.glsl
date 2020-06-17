#version 150 core

uniform sampler2D diffuseTex;

in Vertex {
    vec2 texCoord;
}   IN;

out vec4 fragColour;

void main() {
    vec4 value = texture(diffuseTex, IN.texCoord);

    if (value.a == 0) {
        discard;
    }

    fragColour = value;
}