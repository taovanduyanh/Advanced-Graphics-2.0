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

struct BBox {
    vec4 bounds[2];
};

struct Ray {
    vec3 origin;
    vec3 direction;
    float t;
} ray, shadowRay;

struct BarycentricCoord {
    float u, v, w;
} barycentricCoord;

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

layout(std430, binding = 12) buffer BoundingBox {
    BBox parentBoxSSBO;
};

layout(local_size_variable) in;
layout(binding = 0, offset = 0) uniform atomic_uint counter;

layout(rgba32f) uniform image2D image;
uniform sampler2D diffuse;
uniform int useTexture;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform vec2 pixelSize;
uniform vec3 scaleVector;
uniform vec3 cameraPos;
uniform float fov;

float toRadian(float angle);
bool rayIntersectsTriangle(Ray ray, Triangle triangle);
bool rayIntersectsBox(Ray ray, BBox box);
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

/**
Moller - Trumbore algorithm
Don't need to check for the determinatorsince the Mesh reader already handles the cases behind + parallel?
*/
bool rayIntersectsTriangle(Ray ray, Triangle triangle) {
    // three vertices of the current triangle
    vec3 v0 = (modelMatrix * posSSBO[triangle.vertIndices[0]]).xyz;
    vec3 v1 = (modelMatrix * posSSBO[triangle.vertIndices[1]]).xyz;
    vec3 v2 = (modelMatrix * posSSBO[triangle.vertIndices[2]]).xyz;

    vec3 a = v1 - v0;
    vec3 b = v2 - v0;
    vec3 pVec = cross(ray.direction, b);
    float determinator = dot(a, pVec);

    if (abs(determinator) < EPSILON) {
        return false;
    }

    float invDet = 1 / determinator;

    vec3 tVec = ray.origin - v0;
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
    
    // if t is lower than the epsilon value then the triangle is behind the ray
    // i.e. the ray should not be able to intersect with the triangle
    if (ray.t < EPSILON) {
        return false;
    }

    return true;
}

bool rayIntersectsBox(Ray ray, BBox box) {
    vec3 rayInvDir = 1 / ray.direction;

    int signs[3];   // for x, y, z..
    signs[0] = int(rayInvDir.x < 0);
    signs[1] = int(rayInvDir.y < 0);
    signs[2] = int(rayInvDir.z < 0);

    float tmin, tmax, tymin, tymax, tzmin, tzmax;
    
    tmin = (box.bounds[signs[0]].x - ray.origin.x) * rayInvDir.x;
    tmax = (box.bounds[1 - signs[0]].x - ray.origin.x) * rayInvDir.x;

    tymin = (box.bounds[signs[1]].y - ray.origin.y) * rayInvDir.y;
    tymax = (box.bounds[1 - signs[1]].y - ray.origin.y) * rayInvDir.y;

    if ((tmin > tymax) || (tymin > tmax)) {
        return false;
    }

    if (tymin > tmin) {
        tmin = tymin;
    }
    if (tymax < tmax) {
        tmax = tymax; 
    }

    tzmin = (box.bounds[signs[2]].z - ray.origin.z) * rayInvDir.z;
    tzmax = (box.bounds[1 - signs[2]].z - ray.origin.z) * rayInvDir.z;

    if ((tmin > tzmax) || (tzmin > tmax)) {
        return false;
    }
    if (tzmin > tmin) {
        tmin = tzmin;
    }
    if (tzmax < tmax) {
        tmax = tzmax;
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
    vec4 finalColour = vec4(0.2, 0.2, 0.2, 1.0);

    // the middle point of the pixel should be in world space now by multiplying with the inverse view matrix..
    // need to find the middle point of the pixel in world space..
    mat4 inverseViewMatrix = inverse(viewMatrix);
    vec3 middlePoint = pixelMiddlePoint(pixelCoords);
    ray.origin = (inverseViewMatrix * vec4(0.0, 0.0, 0.0, 1.0)).xyz;   
    ray.direction = normalize((inverseViewMatrix * vec4(middlePoint, 1.0)).xyz - ray.origin);
    
    // IT WORKS!!!
    // We're now using bounding box instead
    // Note: there currently a parent box..
    if (rayIntersectsBox(ray, parentBoxSSBO)) {
        for (int i = 0; i < atomicCounter(counter); ++i) {
            if (rayIntersectsTriangle(ray, facesSSBO[idSSBO[i]])) {
                if (useTexture > 0) {
                    vec2 tc0 = texCoordsSSBO[facesSSBO[idSSBO[i]].texIndices[0]];
                    vec2 tc1 = texCoordsSSBO[facesSSBO[idSSBO[i]].texIndices[1]];
                    vec2 tc2 = texCoordsSSBO[facesSSBO[idSSBO[i]].texIndices[2]];
                    vec2 texCoord = barycentricCoord.w * tc0 + barycentricCoord.u * tc1 + barycentricCoord.v * tc2;
                    return texture(diffuse, texCoord);
                }
                else {
                    return vec4(barycentricCoord.u, barycentricCoord.v, barycentricCoord.w, 1.0);
                }
            }
        }
    }

    return finalColour;
}