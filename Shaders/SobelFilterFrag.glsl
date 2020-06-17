#version 150 core

uniform sampler2D diffuseTex;
uniform vec2 pixelSize;
uniform float time;
uniform float duration;
uniform bool switchColour;

in Vertex {
    vec2 texCoord;
}   IN;

out vec4 fragColour;

const float weights[5] = float[](0.12, 0.22, 0.32, 0.22, 0.12);
const vec4 pink = vec4(1.0, 0.4, 0.6, 1.0); // can change to colour1 for convenience
const vec4 purple = vec4(0.6, 0.2, 1.0, 1.0);   // same thing - change to colour2 if wanted

const mat3 kernelX = mat3(
    1, 0, -1,
    2, 0, -2,
    1, 0, -1
);

const mat3 kernelY = mat3(
    1, 2, 1,
    0, 0, 0,
    -1, -2, -1
);

void main() {
    // Sobel Filter
    mat3 A; // the matrix for the image
    float height = -pixelSize.y; // for the loop below

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            float width = -pixelSize.x;
            vec3 sample = texture(diffuseTex, IN.texCoord + vec2(width, height)).rgb;
            A[i][j] = length(sample);
            width += pixelSize.x;
        }
        height += pixelSize.y;
    }

    // doesn't matter the swizzling order?
    float gradientX = dot(kernelX[0], A[2].zyx) + dot(kernelX[1], A[1].zyx) + dot(kernelX[2], A[0].zyx);
    float gradientY = dot(kernelY[0], A[2].zyx) + dot(kernelY[1], A[1].zyx) + dot(kernelY[2], A[0].zyx);

    float g = sqrt(pow(gradientX, 2.0) + pow(gradientY, 2.0));
    
    float value = 0.0;
    float secondsHavePassed = mod(time, duration);

    if (secondsHavePassed != 0.0) {
        value = secondsHavePassed / duration;
    }
    
    // there are four variations:
    // texture(diffuseTex, IN.texCoord).rgb - vec3(g)
    // 1.0f - vec3(g) 
    // vec3(g) 
    // vec3(g) - 1.0f
    if (!switchColour) {
        fragColour = vec4(vec3(g), 1.0f) * mix(pink, purple, value);
    }
    else {
        fragColour = vec4(vec3(g), 1.0f) * mix(purple, pink, value);
    }
    
    //fragColour = vec4(vec3(g), 1.0f);
}