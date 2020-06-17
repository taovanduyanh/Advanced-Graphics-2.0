#version 330 core
out vec4 fragColour;

in Vertex {
    vec3 worldPos;
    vec2 texCoord;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
} IN;

uniform sampler2D diffuseTex;
uniform sampler2D bumpMap;
uniform sampler2D depthMap;
  
uniform float heightScale;
uniform vec3 lightPos;
uniform vec4 lightColour;
uniform float lightRadius;
uniform vec3 cameraPos;
  
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir, mat3 TBN) {
    const float numLayers = 10;
    float layerDepth = 1.0f / numLayers;
    float currentLayerDepth = 0.0f;
    vec3 temp = normalize(TBN * (viewDir * 2.0f - 1.0f));
    vec2 p = temp.xy * heightScale;
    vec2 deltaTexCoords = p / numLayers;
    
    vec2 currentTexCoord = texCoords;
    float currentDepthValue = texture(depthMap, currentTexCoord).r;

    while (currentLayerDepth < currentDepthValue) {
        currentTexCoord -= deltaTexCoords;
        currentDepthValue = texture(depthMap, currentTexCoord).r;
        currentLayerDepth += layerDepth;
    }

    return currentTexCoord;    
}
  
void main() {           
    vec3 viewDir   = normalize(cameraPos - IN.worldPos);
    mat3 TBN = mat3(IN.tangent, IN.binormal, IN.normal);
    mat3 test = mat3(
        IN.tangent.x, IN.tangent.y, IN.tangent.z,
        IN.binormal.x, IN.binormal.y, IN.binormal.z,
        IN.normal.x, IN.normal.y, IN.normal.z
    );
    
    vec2 newTextureCoord = ParallaxMapping(IN.texCoord,  viewDir, test);

    vec3 normal = normalize(TBN * (texture(bumpMap, newTextureCoord).rgb * 2.0f - 1.0f));

    // Diffuse
    vec4 diffuse = texture(diffuseTex, newTextureCoord);
    vec3 incident = normalize(lightPos - IN.worldPos);
    float lambert = max(0.0f, dot(incident, normal));

    // Attenuation
    float distance = length(lightPos - IN.worldPos);
    float atten = 1.0f - clamp(distance / lightRadius, 0.0f, 1.0f);

    // Specular
    vec3 halfDir = normalize(viewDir + incident);
    float rFactor = max(0.0f, dot(halfDir, normal));
    float sFactor = pow(rFactor, 50.0f);

    vec3 colour = diffuse.rgb * lightColour.rgb;
    colour += lightColour.rgb * sFactor * 0.33f;
    fragColour = vec4(colour * atten * lambert, diffuse.a);
    fragColour.rgb += diffuse.rgb * lightColour.rgb * 0.1f;
}