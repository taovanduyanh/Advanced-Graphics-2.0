#version 150 core

uniform sampler2D diffuseTex;
uniform sampler2D bumpTex;

uniform vec3 lightPos;
uniform vec4 lightColour;
uniform float lightRadius;
uniform vec3 lightDirection;
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
    mat3 TBN = mat3(IN.tangent, IN.binormal, IN.normal);
    vec3 normal = normalize(TBN * (texture(bumpTex, IN.texCoord).rgb * 2.0 - 1.0));

    // Diffuse
    vec4 diffuse = texture(diffuseTex, IN.texCoord);
    //vec3 incident = normalize(lightPos - IN.worldPos);
    vec3 lightDir = normalize(-lightDirection);
    float lambert = max(0.0f, dot(normal, lightDir));

    // Attenuation
    //float distance = length(lightPos - IN.worldPos);
    //float atten = 1.0f - clamp(distance / lightRadius, 0.0f, 1.0f);

    // Specular
    vec3 viewDir = normalize(cameraPos - IN.worldPos);
    vec3 halfDir = normalize(viewDir + lightDir);
    float rFactor = max(0.0f, dot(halfDir, normal));
    float sFactor = pow(rFactor, 33.0f);

    vec3 colour = diffuse.rgb * lightColour.rgb;
    colour += lightColour.rgb * sFactor * 0.33;
    fragColour = vec4(colour * lambert, diffuse.a);
    fragColour.rgb += diffuse.rgb * lightColour.rgb * 0.1f;
}