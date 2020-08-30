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

layout(std430, binding = 7) buffer RootNode {
    float rootNodeValues[][2];
};

layout(std430, binding = 9) buffer ParentNodes {
    float parentNodesValues[][2];
};

layout(std430, binding = 8) buffer LeafNodes {
    float leafNodesValues[][2];
};

layout(local_size_variable) in;

// Default plane normals..
const vec3 defaultNormals[3] = vec3[3] 
(
    vec3(1, 0, 0),
    vec3(0, 1, 0),
    vec3(0, 0, 1)
);

// Kay and Kajiya selected plane normals..
const vec3 kSNormals[7] = vec3[7]
(
    vec3(1, 0, 0),
    vec3(0, 1, 0),
    vec3(0, 0, 1),

    vec3(sqrt(3) / 3),
    vec3(-sqrt(3) / 3, sqrt(3) / 3, sqrt(3) / 3),
    vec3(-sqrt(3) / 3, -sqrt(3) / 3, sqrt(3) / 3),
    vec3(sqrt(3) / 3, -sqrt(3) / 3, sqrt(3) / 3)
);

// Default plane normals plus icosphere normals.. 
const vec3 icoNormals[43] = vec3[43]
(
vec3(1, 0, 0),
vec3(0, 1, 0),
vec3(0, 0, 1),

vec3(0.471318, -0.661688, 0.583119),
vec3(-0.471317, 0.661688, 0.583121),

vec3(0.187593, -0.794659, 0.577344),
vec3(-0.187594, 0.794658, 0.577345),

vec3(0.0385468, 0.661687, 0.748788),
vec3(0.0385481, 0.661688, -0.748788),

vec3(0.102381, -0.943523, 0.315091),
vec3(-0.102381, 0.943523, 0.315091),

vec3(0.700227, -0.661689, 0.268049),
vec3(-0.700228, 0.661688, 0.26805),

vec3(0.607059, -0.794656, 0),

vec3(0.331305, -0.943524, 0),

vec3(0.408938, 0.661687, 0.628442),
vec3(0.408939, 0.661687, -0.628442),

vec3(0.491118, 0.794657, 0.356823),
vec3(0.491119, 0.794657, -0.356822),

vec3(0.724044, 0.661693, 0.194736),
vec3(0.724044, 0.661693, -0.194736),

vec3(0.268034, 0.943523, 0.194738),
vec3(0.268034, 0.943523, -0.194738),

vec3(0.904981, -0.330393, 0.268049),
vec3(-0.904981, 0.330393, 0.268049),

vec3(-0.982246, 0.187599, 0),
vec3(-0.992077, -0.125631, 0),

vec3(0.0247248, -0.330396, 0.943518),
vec3(-0.0247264, 0.330397, 0.943518),

vec3(0.303532, -0.187597, 0.934171),
vec3(-0.303531, 0.187599, 0.934171),

vec3(0.306569, 0.125652, 0.943518),
vec3(0.306568, 0.125651, -0.943519),

vec3(0.534591, -0.330394, 0.777851),
vec3(-0.534589, 0.330396, 0.777851),

vec3(0.889697, 0.330386, 0.315093),
vec3(0.889697, 0.330387, -0.315092),

vec3(0.794655, 0.187596, -0.577349),
vec3(0.794655, 0.187595, 0.577349),

vec3(-0.802607, 0.125649, 0.583125),
vec3(0.802606, -0.125648, 0.583126),

vec3(0.574583, 0.330396, 0.748794),
vec3(0.574584, 0.330397, -0.748793) 
);

const uint depthIndices[3] = uint[3] 
(
    0, 1, 2
);

layout(rgba32f) uniform image2D image;  // this should be in binding 0..
uniform sampler2D diffuse;  // this should be in binding 1
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
bool rayIntersectsRootNode(Ray ray);
bool rayIntersectsLeafNodes(Ray ray, uint index);
bool rayIntersectsTriangle(inout Ray ray, Triangle triangle);   // the 'inout' keyword allows the ray's values will be modified..
vec3 pixelMiddlePoint(ivec2 pixelCoords);
vec4 getFinalColour(ivec2 pixelCoords);

// NOTE: Remember to remove the unecessary comment later
void main() {
    // check if the id are beyond the image's pixel first..
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

/*
Intersection between ray/bounding volume (Root Node)
*/
bool rayIntersectsRootNode(Ray ray) {
    float tNear = -1.0 / 0.0;
    float tFar = 1.0 / 0.0;

    for (uint i = 0; i < icoNormals.length(); ++i) {
        float planeNormalsDotOrigin = dot(icoNormals[i], ray.origin);
        float planeNormalsDotDirection = dot(icoNormals[i], ray.direction);

        float invDenominator = 1 / planeNormalsDotDirection;

        float tempTN = (rootNodeValues[i][0] - planeNormalsDotOrigin) * invDenominator;
        float tempTF = (rootNodeValues[i][1] - planeNormalsDotOrigin) * invDenominator;
        
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

/*
Intersection between ray/bounding volume (Leaf Nodes)
*/
bool rayIntersectsLeafNodes(Ray ray, uint index) {
    float tNear = -1.0 / 0.0;
    float tFar = 1.0 / 0.0;

    for (uint i = 0; i < icoNormals.length(); ++i) {
        float planeNormalsDotOrigin = dot(icoNormals[i], ray.origin);
        float planeNormalsDotDirection = dot(icoNormals[i], ray.direction);

        float invDenominator = 1 / planeNormalsDotDirection;

        float tempTN = (leafNodesValues[index * icoNormals.length() + i][0] - planeNormalsDotOrigin) * invDenominator;
        float tempTF = (leafNodesValues[index * icoNormals.length() + i][1] - planeNormalsDotOrigin) * invDenominator;
        
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
Don't need to check for the determinator since the Mesh reader already handles it.. (assmumingly if the concept is understood correctly)
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

/*
Calculating the middle point of the pixel
*/
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

/*
Convert degree to radian
*/
float toRadian(float angle) {
    return angle * PI / 180.0;
}

vec4 getFinalColour(ivec2 pixelCoords) {
    bool isInShadow = false;    // flag to check if the 
    int closestID = -1; // perhaps this could be infinity as well.. (although it might affect the performance)
    float closestTVal = 1.0 / 0.0; // infinity..
    vec3 closestBaryCoords = vec3(0);   // bary coords from the closest intersection point..

    // further testing..
    int parentNodeID = -1;
    float tNearClosest = 1.0 / 0.0;

    float testNear = -1.0 / 0.0;
    float testFar = 1.0 / 0.0;

    vec4 finalColour;   // the final calculated colour in the end..
    vec4 imageColour = imageLoad(image, pixelCoords);
    // If the retrieved image colour at the pixel coords is dark (all values is 0) then it means the image is not yet drawn..
    // (you don't want to comment the code below if the OBJMesh has submeshes..)
    finalColour = (imageColour == vec4(0)) ? vec4(0.2, 0.2, 0.2, 1.0) : imageColour;
    
    // the middle point of the pixel should be in world space now by multiplying with the inverse view matrix..
    // need to find the middle point of the pixel in world space..
    mat4 inverseViewMatrix = inverse(viewMatrix);
    vec3 middlePoint = pixelMiddlePoint(pixelCoords);
    Ray primaryRay;
    primaryRay.origin = (inverseViewMatrix * vec4(0.0, 0.0, 0.0, 1.0)).xyz;   
    primaryRay.direction = (inverseViewMatrix * vec4(middlePoint, 1.0)).xyz - primaryRay.origin;

    // IT WORKS!!!
    // We're now using icosphere to check for bounding volume..
    if (rayIntersectsRootNode(primaryRay)) {
        //finalColour += vec4(0.0, 0.05, 0.0, 1.0);   
        for (int i = 0; i < numVisibleFaces; ++i) {  // num faces..
            if (rayIntersectsLeafNodes(primaryRay, i)) {
                //finalColour += vec4(0.0, 0.05, 0.0, 1.0);
                ///*
                int intersectedID = idSSBO[i];
                if (rayIntersectsTriangle(primaryRay, facesSSBO[intersectedID]) && (primaryRay.t < closestTVal)) {
                    closestTVal = primaryRay.t;
                    closestID = intersectedID;
                    closestBaryCoords = primaryRay.barycentricCoord;  
                }
            }
        }

        //*/
        if (closestID != -1) {
            /*
            Ray shadowRay;
            shadowRay.origin = primaryRay.origin + primaryRay.direction * closestTVal;
            shadowRay.direction = normalize(lightPos - shadowRay.origin);
                    
            for (int i = 0; i < facesSSBO.length(); ++i) {
                if (i != closestID && rayIntersectsTriangle(shadowRay, facesSSBO[i])) {
                    isInShadow = true;
                    break;
                }   
            }
            //*/ 

            if (useTexture > 0) {
                vec2 tc0 = texCoordsSSBO[facesSSBO[closestID].texIndices[0]];
                vec2 tc1 = texCoordsSSBO[facesSSBO[closestID].texIndices[1]];
                vec2 tc2 = texCoordsSSBO[facesSSBO[closestID].texIndices[2]];
                vec2 texCoord = closestBaryCoords.z * tc0 + closestBaryCoords.x * tc1 + closestBaryCoords.y * tc2;
                return isInShadow ? texture(diffuse, texCoord) * vec4(0.5, 0.5, 0.5, 1.0) : texture(diffuse, texCoord) * lightColour;
            }
            else {
                return isInShadow ? vec4(closestBaryCoords, 1.0) * vec4(0.5, 0.5, 0.5, 1.0) : vec4(closestBaryCoords, 1.0) * lightColour;   // Bary coords colours..
                //return isInShadow ? vec4(0.5, 0.5, 0.5, 1.0) * vec4(0.5, 0.5, 0.5, 1.0) : vec4(0.5, 0.5, 0.5, 1.0) * lightColour;   // Gray colour..
            }
        }
    }
  //*/
    return finalColour;
}