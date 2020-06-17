#version 150 core

uniform sampler2D diffuseTex;
uniform sampler2D bumpTex;
uniform samplerCube cubeTex;

uniform vec4 lightColour;
uniform vec3 lightPos;
uniform float lightRadius;
uniform vec3 cameraPos;

in Vertex {
    vec4 colour;
    vec2 texCoord;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    vec3 worldPos;
}   IN;

out vec4 fragColour;

void main() {
    // we're using directional light here..
    // no attenuation since there is no light position..
    vec4 diffuse = texture(diffuseTex, IN.texCoord) * IN.colour;
    vec3 incident = normalize(IN.worldPos - cameraPos);

    // atten
    //float distance = length(lightPos - IN.worldPos);
    //float atten = 1.0 - clamp(distance / lightRadius, 0.2f, 1.0f);
    mat3 TBN = mat3(IN.tangent, IN.binormal, IN.normal);
    vec3 normal = normalize(TBN * (texture(bumpTex, IN.texCoord).rgb * 2.0 - 1.0));

    vec4 reflection = texture(cubeTex, reflect(incident, normalize(IN.normal)));
    //vec4 reflection = texture(cubeTex, reflect(incident, normalize(normal)));

    //fragColour =  (lightColour * diffuse * atten) * (diffuse + reflection);
    fragColour =  (lightColour * diffuse) * (diffuse + reflection);
    fragColour.a = 0.5f;
}