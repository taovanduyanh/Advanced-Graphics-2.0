#version 150 core

uniform sampler2D diffuseTex;

uniform vec3 cameraPos;
uniform vec3 lightPos;
uniform vec4 lightColour;
uniform float lightRadius;
uniform vec3 lightDirection;

in Vertex {
    vec4 colour;
    vec2 texCoord;
    vec3 normal;
    vec3 worldPos;
}   IN;

out vec4 fragColour;

void main() {
    // we're using directional light here..
    vec3 lightDir = normalize(-lightDirection);

    // Diffuse light
    vec4 diffuse = texture(diffuseTex, IN.texCoord);
    //vec3 incident = normalize(lightPos - IN.worldPos);
    float lambert = max(0.0, dot(lightDir, IN.normal)); // diffuse factor

    // Attenuation
    //float distance = length(lightPos - IN.worldPos);
    //float atten = 1.0f - clamp(distance / lightRadius, 0.0f, 1.0f);   // linear
    //float atten = 1 / pow(distance, 2);   // true atten

    // Specular light
    vec3 viewDir = normalize(cameraPos - IN.worldPos);
    vec3 halfDir = normalize(viewDir + lightDir);

    float n = 50.0f;
    float rFactor = max(0.0f, dot(halfDir, IN.normal)); // reflection factor
    float sFactor = pow(rFactor, n);    // specular factor

    // frag colour + ambient
    vec3 colour = diffuse.rgb * lightColour.rgb;
    colour += lightColour.rgb * sFactor * 0.33f;
    //fragColour = vec4(colour * atten * lambert, diffuse.a);
    fragColour = vec4(colour * lambert, diffuse.a);
    fragColour.rgb += (diffuse.rgb * lightColour.rgb) * 0.1f;
}