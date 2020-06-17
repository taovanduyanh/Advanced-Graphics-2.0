#version 330 core
in vec3 position;
in vec3 normal;
in vec3 tangent;
in vec2 texCoord;


out Vertex {
    vec3 worldPos;
    vec2 texCoord;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
} OUT;

uniform mat4 projMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

uniform vec3 lightPos;
uniform vec3 cameraPos;

void main()
{
    gl_Position      = projMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);
    OUT.worldPos   = (modelMatrix * vec4(position, 1.0f)).xyz;   
    OUT.texCoord = texCoord;    
    
    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
    OUT.normal = normalize(normalMatrix * normalize(normal));
    OUT.tangent = normalize(normalMatrix * normalize(tangent));
    OUT.binormal = cross(normal, tangent);
}   

