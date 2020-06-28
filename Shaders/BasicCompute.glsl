#version 430 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

#define PI 3.1415926538
#define EPSILON 0.0000001

layout(std430, binding = 0) buffer Positions {
    vec3 posSSBO[];
};

layout(std430, binding = 1) buffer Colours {
    vec4 coloursSSBO[];
};

layout(std430, binding = 7) buffer FinalID {
    int finalIDSSBO[];
};

layout(local_size_variable) in;
layout(binding = 0, offset = 0) uniform atomic_uint idCount;

struct Ray {
    vec3 origin;
    vec3 direction;
    float t;
} ray, shadowRay;

struct Triangle {
    vec3 vertices[3];
    vec4 colours[3];
    vec3 normal;
} triangle[2];

struct BarycentricCoord {
    float u, v, w;
} barycentricCoord;

layout(rgba32f) uniform image2D image;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform vec2 pixelSize;
uniform float fov;

float toRadian(float angle);
bool rayIntersectsTriangle(Ray ray, Triangle triangle);
vec3 pixelMiddlePoint(ivec2 pixelCoords);

// NOTE: Remember to remove the unecessary comment later

void main() {
    vec4 finalColour = vec4(0.0);
    // coordinate in pixels
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);

    uint numVertices = atomicCounter(idCount);

    if (numVertices < 3) {
        finalColour = vec4(0.2, 0.2, 0.2, 1.0);
    }
    else {
        uint numTriangles = numVertices % 3 + 1;
        uint currentIndex = 2;

        for (int i = 0; i < numTriangles; ++i) {
            for (int j = 0, k = 2; j < 3 && k >= 0; ++j, --k) {
                triangle[i].vertices[k] = (modelMatrix * vec4(posSSBO[finalIDSSBO[currentIndex - j]], 1.0)).xyz;
                triangle[i].colours[k] = coloursSSBO[finalIDSSBO[currentIndex - j]];
            }
            ++currentIndex;
        }

        // the middle point of the pixel should be in world space now by multiplying with the inverse view matrix..
        // need to find the middle point of the pixel in world space..
        mat4 inverseViewMatrix = inverse(viewMatrix);
        vec3 middlePoint = pixelMiddlePoint(pixelCoords);
        ray.origin = (inverseViewMatrix * vec4(0.0, 0.0, 0.0, 1.0)).xyz;   
        ray.direction = normalize((inverseViewMatrix * vec4(middlePoint, 1.0)).xyz - ray.origin);

        // check for intersection between ray and objects
        for (int i = 0; i < triangle.length(); ++i) {
            if (rayIntersectsTriangle(ray, triangle[i])) {
                finalColour = barycentricCoord.w * triangle[i].colours[0] +  barycentricCoord.v * triangle[i].colours[2] + barycentricCoord.u * triangle[i].colours[1];   // this produces the correct colours..
                //finalColour = vec4(barycentricCoord.w, barycentricCoord.v, barycentricCoord.u, 1.0);
                break;
            } 
            else {
                finalColour = vec4(0.2, 0.2, 0.2, 1.0);
            }
        }
    }

    imageStore(image, pixelCoords, finalColour);
    memoryBarrierShared();
}

/**
Moller - Trumbore algorithm
Don't need to check for the determinator and t values since the Mesh reader already handles the cases behind + parallel
*/
bool rayIntersectsTriangle(Ray ray, Triangle triangle) {
    vec3 a = triangle.vertices[1] - triangle.vertices[0];
    vec3 b = triangle.vertices[2] - triangle.vertices[0];
    vec3 pVec = cross(ray.direction, b);
    float determinator = dot(a, pVec);

    // the nanimg convension is not nice.. 
    float invDet = 1 / determinator;

    vec3 tVec = ray.origin - triangle.vertices[0];
    barycentricCoord.u = invDet * dot(tVec, pVec);
    if (barycentricCoord.u < 0 || barycentricCoord.u > 1) {
        return false;
    }

    vec3 qVec = cross(tVec, a);
    barycentricCoord.v = invDet * dot(ray.direction, qVec);
    if (barycentricCoord.v < 0 || barycentricCoord.u + barycentricCoord.v > 1) {
        return false;
    }
    
    barycentricCoord.w = 1 - barycentricCoord.u - barycentricCoord.v;

    ray.t = invDet * dot(b, qVec);

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