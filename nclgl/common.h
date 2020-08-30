/******************************************************************************
Author:Rich Davison
Description: Some random variables and functions, for lack of a better place 
to put them.

-_-_-_-_-_-_-_,------,   
_-_-_-_-_-_-_-|   /\_/\   NYANYANYAN
-_-_-_-_-_-_-~|__( ^ .^) /
_-_-_-_-_-_-_-""  ""   

*//////////////////////////////////////////////////////////////////////////////

#pragma once

#define WEEK_2_CODE
#define WEEK_3_CODE

/*
As this tutorial series progresses, you'll learn how to generate normals, tangents,
and how to use bumpmaps. In order for this class to compile before these features
are introduced, 'advanced' functionality has been disabled using the preprocessor.

If you want to play around with MD5Meshes in the first real time lighting tutorial,
uncomment the MD5_USE_NORMALS define. If you want to use and MD5Mesh in the second real
time lighting tutorial, uncomment both MD5_USE_NORMALS and MD5_USE_TANGENTS_BUMPMAPS
*/
#define USE_MD5MESH
#define MD5_USE_NORMALS
#define MD5_USE_TANGENTS_BUMPMAPS

// For dissertation (Van Duy Anh Tao)
// Comment this if you want to go back to rasterization, but don't do it for the diss obviously..
#define USE_RAY_TRACING

#ifdef USE_RAY_TRACING
// the number of planes necessary to create the bounding volume (3 of them are the default plane normals, the other 40 are from the icosphere)
constexpr auto NUM_PLANE_NORMALS = 43;
#endif

//It's pi(ish)...
static const float		PI = 3.14159265358979323846f;	

//It's pi...divided by 360.0f!
static const float		PI_OVER_360 = PI / 360.0f;

//Radians to degrees
static inline double RadToDeg(const double deg)	{
	return deg * 180.0 / PI;
};

//Degrees to radians
static inline double DegToRad(const double rad)	{
	return rad * PI / 180.0;
};

//I blame Microsoft...
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))

#define SHADERDIR	"../../Shaders/"
#define MESHDIR		"../../Meshes/"
#define TEXTUREDIR  "../../Textures/"
#define SOUNDSDIR	"../../Sounds/"