#version 150 core

uniform sampler2D diffuseTex;
uniform sampler2D bumpTex;
uniform sampler2DShadow shadowTex;

uniform vec3 lightPos;
uniform vec4 lightColour;
uniform float lightRadius;
uniform vec3 cameraPos;

in Vertex {
    vec4 colour;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    vec2 texCoord;
    vec3 worldPos;
    vec4 shadowProj;
}  IN;

out vec4 fragColour;

void main() {
    mat3 TBN = mat3(IN.tangent, IN.binormal, IN.normal);
    vec3 normal = normalize(TBN * (texture(bumpTex, IN.texCoord).rgb * 2.0f - 1.0f));

    vec4 diffuse = texture(diffuseTex, IN.texCoord);
    vec3 incident = normalize(lightPos - IN.worldPos);
    float lambert = max(0.0, dot(incident, normal));

    // atten
    float distance = length(lightPos - IN.worldPos);
    float atten = 1.0f - clamp(distance / lightRadius, 0.0, 1.0);

    // specular
    vec3 viewDir = normalize(cameraPos - IN.worldPos);
    vec3 halfDir = normalize(viewDir + incident);

    float rFactor = max(0.0, dot(halfDir, normal));
    float sFactor = pow(rFactor, 33.0);

    float shadow = 1.0;

    if (IN.shadowProj.w > 0.0) {
        shadow = textureProj(shadowTex, IN.shadowProj);
    }

    lambert *= shadow;

    vec3 colour = diffuse.rgb * lightColour.rgb;
    colour += lightColour.rgb * sFactor * 0.33;
    fragColour = vec4(colour * atten * lambert, diffuse.a);
    fragColour.rgb += diffuse.rgb * lightColour.rgb * 0.1;
}