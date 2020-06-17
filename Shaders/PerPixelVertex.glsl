#version 150 core

uniform mat4 modelMatrix;
uniform mat4 projMatrix;
uniform mat4 viewMatrix;
uniform mat4 textureMatrix;

in vec3 position;
in vec4 colour;
in vec2 texCoord;
in vec3 normal;

out Vertex {
    vec4 colour;
    vec2 texCoord;
    vec3 normal;
    vec3 worldPos;
}   OUT;

void main() {
    mat4 mvp = projMatrix * viewMatrix * modelMatrix;
    gl_Position = mvp * vec4(position, 1.0f);

    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));

    OUT.normal = normalize(normalMatrix * normalize(normal));
    OUT.colour = colour;
    OUT.worldPos = (modelMatrix * vec4(position, 1.0f)).xyz;
    OUT.texCoord = (textureMatrix * vec4(texCoord, 0.0f, 1.0f)).xy;
}