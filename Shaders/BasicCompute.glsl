#version 430 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_compute_variable_group_size : enable

layout(std430, binding = 0) buffer Colours {
    vec4 colourSSBO[];
};

layout(local_size_variable) in;

struct Ray {
    vec3 origin;
    vec3 direction;
    float t;
} ray;

struct Triangle {
    vec3 vertices[3];
    vec4 colour;
    vec3 normal;
} triangle;

layout(rgba32f) uniform image2D image;
uniform sampler2D depthTex;
uniform mat4 projMatrix;
uniform mat4 viewMatrix;
uniform vec2 pixelSize;
uniform vec3 cameraPos;

bool intersect(Ray ray, Triangle triangle);
bool pointIsInTrianglePlane(vec3 point, Triangle triangle);

void main() {
    /**
    vec2 texCoord = gl_GlobalInvocationID.xy * pixelSize;
    vec4 colour = texture(depthTex, texCoord);
    uint index = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * gl_NumWorkGroups.x;
    colourSSBO[index] = colour * vec4(1.0, 0.0, 0.5, 1.0);
    */

    // setting a triangle for testing..
    triangle.vertices[0] = vec3(0.0, 0.5, -1.0);
    triangle.vertices[1] = vec3(0.5, -0.5, -1.0);
    triangle.vertices[2] = vec3(-0.5, -0.5, -1.0);
    triangle.colour = vec4(1.0, 0.0, 0.0, 1.0);
    triangle.normal = normalize(cross(triangle.vertices[1] - triangle.vertices[0], triangle.vertices[2] - triangle.vertices[0]));

    // coordinate in pixels
    // 1st need to find the middle point of the pixel in world space..
    ivec2 imageSize = imageSize(image); // x = width; y = height
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    vec3 middlePoint = vec3((pixelCoords.x + 0.5) * pixelSize.x, (pixelCoords.y + 0.5) * pixelSize.y, 0.0); 
    middlePoint.z = texture(depthTex, pixelCoords.xy).r;

    mat4 inverseProjView = inverse(projMatrix * viewMatrix);
    middlePoint.x = middlePoint.x * 2.0 - 1.0;
    middlePoint.y = 1.0 - 2.0 * middlePoint.y;  // have to do this 
    vec4 clip = inverseProjView * vec4(middlePoint, 1.0);
    middlePoint = clip.xyz / clip.w;    // the middle point of the pixel should be in world space now..
    ray.origin = cameraPos;
    ray.direction = normalize(middlePoint - cameraPos);

    vec4 finalColour = vec4(0.0);

    // check for intersection between ray and objects
    if (!intersect(ray, triangle)) {
        finalColour = vec4(0.2, 0.2, 0.2, 1.0);
    } 
    else {
        finalColour = triangle.colour;
    }

    imageStore(image, pixelCoords, finalColour);
    memoryBarrierShared();
}

bool intersect(Ray ray, Triangle triangle) {
    // checking for parallel case
    // => if a triangle's normal and a ray are parallel, then there is no intersection
    // aka the dot product between these two equals to zero
    float denominator = dot(triangle.normal, ray.direction);

    if (denominator == 0) {
        return false;
    }

    denominator = 1 / denominator;
    float d = dot(triangle.normal, triangle.vertices[0]);
    ray.t = (dot(triangle.normal, ray.origin) + d) * denominator;

    if (ray.t <= 0) {
        return false;
    }
    
    vec3 hitPoint = ray.origin + ray.t * ray.direction;
    if (!pointIsInTrianglePlane(hitPoint, triangle)) {
        return false;
    }

    return true;
}

bool pointIsInTrianglePlane(vec3 point, Triangle triangle) {
    vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
    vec3 e1 = triangle.vertices[2] - triangle.vertices[1];
    vec3 e2 = triangle.vertices[0] - triangle.vertices[2];

    vec3 c0 = point - triangle.vertices[0];
    vec3 c1 = point - triangle.vertices[1];
    vec3 c2 = point - triangle.vertices[2];

    bool condition0 = dot(triangle.normal, cross(e0, c0)) > 0;
    bool condition1 = dot(triangle.normal, cross(e1, c1)) > 0;
    bool condition2 = dot(triangle.normal, cross(e2, c2)) > 0;

    if (condition0 && condition1 && condition2) {
        return true;
    }

    return false;
}
