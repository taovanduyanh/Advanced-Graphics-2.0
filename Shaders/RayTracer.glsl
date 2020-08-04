#version 460 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

#define PI 3.1415926538
#define EPSILON 0.00001

struct Triangle {
    uint vertIndices[3];
    uint texIndices[3];
    uint normalsIndices[3];
};

struct Ray {
    vec3 origin;
    vec3 direction;
    vec3 barycentricCoord; // xyz == uvw..
    float t;
};

layout(std430, binding = 0) buffer Positions {
    vec4 posSSBO[];
};

layout(std430, binding = 2) buffer TextureCoords {
    vec2 texCoordsSSBO[];
};

layout(std430, binding = 5) buffer ID {
    int idSSBO[];
};

layout(std430, binding = 6) buffer Faces {
    Triangle facesSSBO[];
};

layout(std430, binding = 7) buffer PlaneDs {
    float dSSBO[][2];
};

layout(local_size_variable) in;


const vec3 kSNormals[7] = vec3[7](vec3(1, 0, 0),
                                  vec3(0, 1, 0),
                                  vec3(0, 0, 1),
                                  vec3(sqrt(3) / 3),
                                  vec3(-sqrt(3) / 3, sqrt(3) / 3, sqrt(3) / 3),
                                  vec3(-sqrt(3) / 3, -sqrt(3) / 3, sqrt(3) / 3),
                                  vec3(sqrt(3) / 3, -sqrt(3) / 3, sqrt(3) / 3)
                            );


const vec3 nearHalfIcoNormals[19] = vec3[19]
(
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),

    vec3(0.700227, -0.661689, 0.268049),
    vec3(-0.700228, 0.661688, 0.26805),

    vec3(-0.724045, -0.661693, 0.194734),
    vec3(0.724044, 0.661693, 0.194736),

    vec3(0.904981, -0.330393, 0.268049),
    vec3(-0.904981, 0.330393, 0.268049),

    vec3(0.0247248, -0.330396, 0.943518),
    vec3(-0.0247264, 0.330397, 0.943518),

    vec3(0.534591, -0.330394, 0.777851),
    vec3(-0.534589, 0.330396, 0.777851),

    vec3(-0.889698, -0.330386, 0.315091),
    vec3(0.889697, 0.330386, 0.315093),

    vec3(0.794655, 0.187595, 0.577349),
    vec3(-0.794656, -0.187594, 0.577348),

    vec3(-0.802607, 0.125649, 0.583125),
    vec3(0.802606, -0.125648, 0.583126)                                
);

/*
const vec3 nearHalfIcoNormals[17] = vec3[17](
    vec3(-0.270598, 0.707107, -0.653281),
    vec3(0.382683, 0, -0.923879),
    vec3(0.31246, 0.57735, -0.754345),
    vec3(0.653282, 0.707107, -0.270598),
    vec3(0, 1, 0),
    vec3(1, 0, 0),
    vec3(0, 0, 1),
    vec3(-0.653282, 0.707107, 0.270598),
    vec3(-0.31246, 0.57735, 0.754344),
    vec3(-0.92388, 0, 0.382683),
    vec3(-0.754344, 0.57735, -0.31246),
    vec3(0.653281, -0.707107, -0.270598),
    vec3(0.92388, 0, 0.382683),
    vec3(0.270598, 0.707107, 0.653281),
    vec3(0.754344, 0.57735, 0.31246),
    vec3(-0.653281, -0.707107, 0.270598),
    vec3(0.382683, 0, 0.923879)
);
*/
layout(rgba32f) uniform image2D image;
uniform sampler2D diffuse;
uniform int useTexture;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform vec2 pixelSize;
uniform float fov;
uniform uint numVisibleFaces;

// testing..
uniform vec4 lightColour;
uniform vec3 lightPos;

float toRadian(float angle);
bool rayIntersectsVolume(Ray ray);
bool rayIntersectsTriangle(inout Ray ray, Triangle triangle);   // the 'inout' keyword allows the ray's values will be modified..
vec3 pixelMiddlePoint(ivec2 pixelCoords);
vec4 getFinalColour(ivec2 pixelCoords);

// NOTE: Remember to remove the unecessary comment later
void main() {
    // check first..
    ivec2 imageSize = imageSize(image);
    if ((gl_GlobalInvocationID.x >= imageSize.x) || (gl_GlobalInvocationID.y >= imageSize.y)) {
        return;
    }

    // coordinate in pixels
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    vec4 finalColour = getFinalColour(pixelCoords);
    imageStore(image, pixelCoords, finalColour);
    memoryBarrierImage();
    barrier();
}

bool rayIntersectsVolume(Ray ray) {
    float tNear = -1.0 / 0.0;
    float tFar = 1.0 / 0.0;

    for (uint i = 0; i < nearHalfIcoNormals.length(); ++i) {
        float planeNormalsDotOrigin = dot(nearHalfIcoNormals[i], ray.origin);
        float planeNormalsDotDirection = dot(nearHalfIcoNormals[i], ray.direction);

        float invDenominator = 1 / planeNormalsDotDirection;

        float tempTN = (dSSBO[i][0] - planeNormalsDotOrigin) * invDenominator;
        float tempTF = (dSSBO[i][1] - planeNormalsDotOrigin) * invDenominator;
        
        if (planeNormalsDotDirection < EPSILON) {
            float temp = tempTN;
            tempTN = tempTF;
            tempTF = temp;
        }

        if (tempTN > tNear) {
            tNear = tempTN;
        }
        if (tempTF < tFar) {
            tFar = tempTF;
        }

        // Check tFar against epsilon to prevent "mirror" volume (idk how to describe this lol) 
        if (tNear > tFar || tFar <= EPSILON) {
            return false;
        }
    }
    
    return true;
}

/**
Moller - Trumbore algorithm
Don't need to check for the determinatorsince the Mesh reader already handles the cases behind + parallel?
*/
bool rayIntersectsTriangle(inout Ray ray, Triangle triangle) {
    // three vertices of the current triangle
    vec3 v0 = (modelMatrix * posSSBO[triangle.vertIndices[0]]).xyz;
    vec3 v1 = (modelMatrix * posSSBO[triangle.vertIndices[1]]).xyz;
    vec3 v2 = (modelMatrix * posSSBO[triangle.vertIndices[2]]).xyz;

    vec3 a = v1 - v0;
    vec3 b = v2 - v0;
    vec3 pVec = cross(ray.direction, b);
    float determinator = dot(a, pVec);
    
    float invDet = 1 / determinator;

    vec3 tVec = ray.origin - v0;
    ray.barycentricCoord.x = invDet * dot(tVec, pVec);
    if (ray.barycentricCoord.x < 0 || ray.barycentricCoord.x > 1) {
        return false;
    }

    vec3 qVec = cross(tVec, a);
    ray.barycentricCoord.y = invDet * dot(ray.direction, qVec);
    if (ray.barycentricCoord.y < 0 || ray.barycentricCoord.x + ray.barycentricCoord.y > 1) {
        return false;
    }
    
    ray.barycentricCoord.z = 1 - ray.barycentricCoord.x - ray.barycentricCoord.y;

    ray.t = invDet * dot(b, qVec);
    
    // if t is lower than the epsilon value then the triangle is behind the ray
    // i.e. the ray should not be able to intersect with the triangle
    if (ray.t <= EPSILON) {
        return false;
    }

    return true;
}

vec3 pixelMiddlePoint(ivec2 pixelCoords) {
    ivec2 imageSize = imageSize(image); // x = width; y = height
    float imageRatio = imageSize.x / imageSize.y;
    vec3 middlePoint = vec3((pixelCoords.x + 0.5) * pixelSize.x, (pixelCoords.y + 0.5) * pixelSize.y, -1.0); 

    // comment this part later..
    float angleInRadian = toRadian(fov) * 0.5;
    middlePoint.x = (middlePoint.x * 2.0 - 1.0) * imageRatio * tan(angleInRadian);
    middlePoint.y = (middlePoint.y * 2.0 - 1.0) * tan(angleInRadian);  // do the other way to flip it..

    return middlePoint;
}

/**
Convert degree to radian
*/
float toRadian(float angle) {
    return angle * PI / 180.0;
}

vec4 getFinalColour(ivec2 pixelCoords) {
    bool isInShadow = false;
    vec4 finalColour = vec4(0.2, 0.2, 0.2, 1.0);
    // the middle point of the pixel should be in world space now by multiplying with the inverse view matrix..
    // need to find the middle point of the pixel in world space..
    mat4 inverseViewMatrix = inverse(viewMatrix);
    vec3 middlePoint = pixelMiddlePoint(pixelCoords);
    Ray primaryRay;
    primaryRay.origin = (inverseViewMatrix * vec4(0.0, 0.0, 0.0, 1.0)).xyz;   
    primaryRay.direction = (inverseViewMatrix * vec4(middlePoint, 1.0)).xyz - primaryRay.origin;

    // IT WORKS!!!
    // We're now using Ks' bounding volume..
    // but we are not using their plane normals, we use the normals gotten from icosphere instead..
    if (rayIntersectsVolume(primaryRay)) {
        finalColour += vec4(0.0, 0.05, 0.0, 1.0);
        for (int i = 0; i < numVisibleFaces; ++i) {
            int intersectedID = idSSBO[i];
            if (rayIntersectsTriangle(primaryRay, facesSSBO[intersectedID])) {
                
                /*
                Ray shadowRay;
                shadowRay.origin = primaryRay.origin + primaryRay.direction * primaryRay.t;
                shadowRay.direction = normalize(lightPos - shadowRay.origin);
                
                for (int j = 0; j < facesSSBO.length(); ++j) {
                    if (j != intersectedID && rayIntersectsTriangle(shadowRay, facesSSBO[j])) {
                        isInShadow = true;
                        break;
                    }
                }
                //*/           

                if (useTexture > 0) {
                    vec2 tc0 = texCoordsSSBO[facesSSBO[intersectedID].texIndices[0]];
                    vec2 tc1 = texCoordsSSBO[facesSSBO[intersectedID].texIndices[1]];
                    vec2 tc2 = texCoordsSSBO[facesSSBO[intersectedID].texIndices[2]];
                    vec2 texCoord = primaryRay.barycentricCoord.z * tc0 + primaryRay.barycentricCoord.x * tc1 + primaryRay.barycentricCoord.y * tc2;
                    //return isInShadow ? texture(diffuse, texCoord) * vec4(0.5, 0.5, 0.5, 1.0) : texture(diffuse, texCoord) * lightColour;
                    return texture(diffuse, texCoord) * lightColour;
                }
                else {
                    //return isInShadow ? vec4(primaryRay.barycentricCoord, 1.0) * vec4(0.5, 0.5, 0.5, 1.0) : vec4(primaryRay.barycentricCoord, 1.0) * lightColour;
                    return vec4(primaryRay.barycentricCoord, 1.0) * lightColour;
                }
            }
        }
    }

    return finalColour;
}