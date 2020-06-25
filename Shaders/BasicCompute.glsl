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

layout(std430, binding = 6) buffer ID {
    int idSSBO[];
};

layout(local_size_variable) in;

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
uniform mat4 viewMatrix;
uniform vec2 pixelSize;
uniform float fov;

float toRadian(float angle);
bool rayIntersectsTriangle(Ray ray, Triangle triangle);

// NOTE: Remember to remove the unecessary comment later

void main() {
    // an ID with value of -1 means that there is no vertices in front of the camera - we don't have to process any further 
    if (idSSBO[0] == -1) {
        return;
    }

    // setting a triangle for testing..
    for (int i = 0; i < 3; ++i) {
        triangle[0].vertices[idSSBO[i]] = posSSBO[idSSBO[i]];
        triangle[0].colours[idSSBO[i]] = coloursSSBO[idSSBO[i]];
    }
    triangle[0].normal = cross(triangle[0].vertices[1] - triangle[0].vertices[0], triangle[0].vertices[2] - triangle[0].vertices[0]);   // note that it is not normalized yet..

    triangle[1].vertices[0] = posSSBO[idSSBO[3]];
    triangle[1].colours[0] = coloursSSBO[idSSBO[3]];
    triangle[1].vertices[1] = posSSBO[idSSBO[2]];
    triangle[1].colours[1] = coloursSSBO[idSSBO[2]];
    triangle[1].vertices[2] = posSSBO[idSSBO[1]];
    triangle[1].colours[2] = coloursSSBO[idSSBO[1]];
    triangle[1].normal = cross(triangle[1].vertices[1] - triangle[1].vertices[0], triangle[1].vertices[2] - triangle[1].vertices[0]);   // note that it is not normalized yet..

    // coordinate in pixels
    // need to find the middle point of the pixel in world space..
    ivec2 imageSize = imageSize(image); // x = width; y = height
    float imageRatio = imageSize.x / imageSize.y;
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    vec3 middlePoint = vec3((pixelCoords.x + 0.5) * pixelSize.x, (pixelCoords.y + 0.5) * pixelSize.y, -1.0); 

    // comment this part later..
    mat4 inverseViewMatrix = inverse(viewMatrix);
    float angleInRadian = toRadian(fov) * 0.5;
    middlePoint.x = (middlePoint.x * 2.0 - 1.0) * imageRatio * tan(angleInRadian);
    middlePoint.y = (middlePoint.y * 2.0 - 1.0) * tan(angleInRadian);  // do the other way to flip it..

    // the middle point of the pixel should be in world space now by multiplying with the inverse view matrix..
    //inverseViewMatrix[3].z = -inverseViewMatrix[3].z; // negate the translation -z to fix the camera's movement bug.. (it's probably due to the negative z values stuff)
    ray.origin = (inverseViewMatrix * vec4(0.0, 0.0, 0.0, 1.0)).xyz;   
    ray.direction = normalize(ray.origin - (inverseViewMatrix * vec4(middlePoint, 1.0)).xyz);
    vec4 finalColour = vec4(0.0);

    // check for intersection between ray and objects
    if (!rayIntersectsTriangle(ray, triangle[gl_LocalInvocationID.x])) {
        finalColour = vec4(0.2, 0.2, 0.2, 1.0);
    } 
    else {
        finalColour = vec4(barycentricCoord.w, barycentricCoord.u, barycentricCoord.v, 1.0);    // this produces the correct colour..
    }

    imageStore(image, pixelCoords, finalColour);
    memoryBarrierShared();
}

/**
Moller - Trumbore algorithm
*/
bool rayIntersectsTriangle(Ray ray, Triangle triangle) {
    vec3 a = triangle.vertices[1] - triangle.vertices[0];
    vec3 b = triangle.vertices[2] - triangle.vertices[0];
    vec3 pVec = cross(ray.direction, b);
    float determinator = dot(a, pVec);

    // checking for parallel and backfacing case
    // the dot product between two vectors equals to zero means that they are parallel -> no intersection
    // if the dot product is below zero, then the triangle is backfacing
    if (determinator < EPSILON) {
        return false;
    }

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

    ray.t = -invDet * dot(b, qVec); // need to negate this otherwise it would result in something horrible..

    // need to check this, otherwise will end up with a flipped image when going through the triangle..
    if (ray.t < EPSILON) {
        return false;
    }

    return true;
}

/**
Convert degree to radian
*/
float toRadian(float angle) {
    return angle * PI / 180.0;
}
