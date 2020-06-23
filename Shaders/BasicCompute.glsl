#version 430 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

#define PI 3.1415926538
#define EPSILON 0.0000001

layout(std430, binding = 0) buffer Colours {
    vec4 colourSSBO[];
};

layout(local_size_variable) in;

struct Ray {
    vec3 origin;
    vec3 direction;
    float t;
} ray, shadowRay;

struct Triangle {
    vec3 vertices[3];
    vec4 colour[3];
    vec3 normal;
} triangle;

struct BarycentricCoord {
    float u, v, w;
} barycentricCoord;

layout(rgba32f) uniform image2D image;
uniform mat4 viewMatrix;
uniform vec2 pixelSize;
uniform float fov;
uniform vec3 cameraPos;

float toRadian(float angle);
bool newIntersect(Ray ray, Triangle triangle);
bool intersect(Ray ray, Triangle triangle);
bool pointIsInTrianglePlane(vec3 point, Triangle triangle);
void calculateBerycentricCoord(float tempU, float tempV, Triangle triangle);

// NOTE: Remember to remove the unecessary comment later

void main() {
    /**
    vec2 texCoord = gl_GlobalInvocationID.xy * pixelSize;
    vec4 colour = texture(depthTex, texCoord);
    uint index = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * gl_NumWorkGroups.x;
    colourSSBO[index] = colour * vec4(1.0, 0.0, 0.5, 1.0);
    */

    // setting a triangle for testing..
    triangle.vertices[0] = vec3(0, 0.5, -1.0);
    triangle.vertices[1] = vec3(0.5, -0.5, -1.0);
    triangle.vertices[2] = vec3(-0.5,  -0.5, -1.0);
    triangle.colour[0] = vec4(1.0, 0.0, 0.0, 1.0);
    triangle.colour[1] = vec4(0.0, 1.0, 0.0, 1.0);
    triangle.colour[2] = vec4(0.0, 0.0, 1.0, 1.0);
    triangle.normal = cross(triangle.vertices[1] - triangle.vertices[0], triangle.vertices[2] - triangle.vertices[0]);   // note that it is not normalized yet..

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
    inverseViewMatrix[3].z = -inverseViewMatrix[3].z; // negate the translation -z to fix the camera's movement bug.. (it's probably due to the negative z values stuff)
    ray.origin = (inverseViewMatrix * vec4(0.0, 0.0, 0.0, 1.0)).xyz;   
    ray.direction = normalize((inverseViewMatrix * vec4(middlePoint, 1.0)).xyz - ray.origin);

    vec4 finalColour = vec4(0.0);

    // check for intersection between ray and objects
    if (!intersect(ray, triangle)) {
        finalColour = vec4(0.2, 0.2, 0.2, 1.0);
    } 
    else {
        finalColour = barycentricCoord.u * triangle.colour[2] + barycentricCoord.v * triangle.colour[0] + barycentricCoord.w * triangle.colour[1];
    }

    imageStore(image, pixelCoords, finalColour);
    memoryBarrierShared();
}

// it requires you to go clockwise..
bool newIntersect(Ray ray, Triangle triangle) {
    vec3 a = triangle.vertices[1] - triangle.vertices[0];
    vec3 b = triangle.vertices[2] - triangle.vertices[0];
    vec3 pVec = cross(ray.direction, b);
    float determinator = dot(a, pVec);

    // checking for parallel and backfacing case
    // the dot product between two vectors equals to zero means that they are parallel -> no intersection
    // if the dot product is below zero, then the triangle is backfacing
    if (determinator > 1.0) {
        return false;
    }

    // the nanimg convension is not nice.. 
    float denominator = 1 / determinator;

    vec3 tVec = ray.origin - triangle.vertices[0];
    barycentricCoord.u = denominator * dot(tVec, pVec);
    if (barycentricCoord.u < 0 || barycentricCoord.u > 1) {
        return false;
    }

    vec3 qVec = cross(tVec, a);
    barycentricCoord.v = denominator * dot(ray.direction, qVec);
    if (barycentricCoord.v < 0 || barycentricCoord.u + barycentricCoord.v > 1) {
        return false;
    }
    
    barycentricCoord.w = 1 - barycentricCoord.u - barycentricCoord.v;

    ray.t = denominator * dot(b, qVec);

    return true;
}

/**
Check for intersection between a ray and a triangle
*/
bool intersect(Ray ray, Triangle triangle) {
    float denominator = dot(triangle.normal, ray.direction);
    if (denominator <= 0) {     
        return false;
    }

    denominator = 1 / denominator;
    float d = dot(triangle.normal, triangle.vertices[0]);
    float t = (dot(triangle.normal, ray.origin) + d) * denominator;   // distance between the ray origin and the intersection point

    // if t is negative then it means the ray is moving backwards - there won't be no intersection in front
    if (t < 0) {
        return false;
    }
    
    vec3 hitPoint = ray.origin + t * ray.direction;
    if (!pointIsInTrianglePlane(hitPoint, triangle)) {  // only if the intersection point is in/on the triangular plane then it's true that the ray intersects a triangle
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

/**
Check if the retrieved intersection point is in/on the plane or not
*/
bool pointIsInTrianglePlane(vec3 point, Triangle triangle) {
    vec3 a = triangle.vertices[1] - triangle.vertices[0];
    vec3 b = triangle.vertices[2] - triangle.vertices[1];
    vec3 c = triangle.vertices[0] - triangle.vertices[2];

    vec3 va = point - triangle.vertices[0];
    vec3 vb = point - triangle.vertices[1];
    vec3 vc = point - triangle.vertices[2];

    // for the barycentric coordinates..
    float tempU = dot(triangle.normal, cross(a, va));
    float tempV = dot(triangle.normal, cross(b, vb));
    bool condition0 = tempU > 0;
    bool condition1 = tempV > 0;
    bool condition2 = dot(triangle.normal, cross(c, vc)) > 0;

    if (condition0 && condition1 && condition2) {
        calculateBerycentricCoord(tempU, tempV, triangle);
        return true;
    }

    return false;
}

/**
Calculate the Barycentric Coordinates based on the intersection point and triangle normal
This looks trivial cause most of the values we need are calculated in the function above
*/
void calculateBerycentricCoord(float tempU, float tempV, Triangle triangle) {
    float denominator = 1 / dot(triangle.normal, triangle.normal);
    barycentricCoord.u = tempU * denominator;
    barycentricCoord.v = tempV * denominator;
    barycentricCoord.w = 1 - barycentricCoord.u - barycentricCoord.v;   // u + v + w = 1..
}
