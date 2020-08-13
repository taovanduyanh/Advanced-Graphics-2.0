#pragma once
#include "OGLRenderer.h"

#ifdef USE_RAY_TRACING

// this is for SSBOs, while the above is for vertex buffers
enum VertexInfo {
	POSITION, COLOUR, TEX_COORD, NORMAL, TANGENT, MAX
};

struct Triangle {
	GLuint verticesIndices[3];
	GLuint texCoordsIndices[3];
	GLuint normalsIndices[3];
};

struct BoundingBox {
	Vector4 boundVectors[2];
};

#endif // USE_RAY_TRACING

enum MeshVertexBuffer {
	VERTEX_BUFFER, COLOUR_BUFFER, TEXTURE_BUFFER, NORMAL_BUFFER, TANGENT_BUFFER, INDEX_BUFFER, MAX_BUFFER
};

class Mesh {
public:
	Mesh(void);
	~Mesh(void);

	virtual void Draw();
	static Mesh* GenerateTriangle();
	static Mesh* GenerateQuad();

	void SetTexutre(GLuint tex) { texture = tex; }
	GLuint GetTexture() { return texture; }

	void SetBumpMap(GLuint tex) { bumpTexture = tex; }
	GLuint GetBumpMap() { return bumpTexture; }

#ifdef USE_RAY_TRACING
	static Mesh* GenerateTriangleRayTracing();
	static Mesh* GenerateQuadRayTracing();

	// Mainly for compute shaders..
	GLuint GetNumFaces() const { return numFaces; }	// Get the number of triangles/faces the mesh has.. 

	void UpdateCollectedID();	// Update the id list of triangles/faces for visible ones..

	GLuint GetNumVisibleFaces() const { return numVisibleFaces; } // Get the number of visible triangles/faces the mesh has after back culling..

	void BindSSBOs();	// Bind the SSBOs when initializing data and stuffs..

	// 'Boogy' virtual function.. ()
	virtual std::vector<Mesh*> GetChildren() const { return std::vector<Mesh*>(); };

	// remove this later..
	void PrintDistances();

#endif // USE_RAY_TRACING

protected:
	void BufferData();
	void GenerateNormals();
	Vector3 GenerateTangent(const Vector3& a, const Vector3& b, const Vector3& c, const Vector2& ta, const Vector2& tb, const Vector2& tc);
	void GenerateTangents();

	// vertex buffers..
	GLuint arrayObject;
	GLuint bufferObject[MAX_BUFFER];

	// no. of vertices, indices faces, etc.
	// maybe numTangents?
	GLuint numVertices;
	GLuint numTexCoords;
	GLuint numNormals;
	GLuint numIndices;

	// textures..
	GLuint texture;
	GLuint bumpTexture;

	GLuint type;

	// vertex data..
	std::vector<Vector3>* vertices;
	std::vector<Vector4>* colours;
	std::vector<Vector2>* textureCoords;
	std::vector<Vector3>* normals;
	std::vector<Vector3>* tangents;
	std::vector<GLuint>* indices;

#ifdef USE_RAY_TRACING

	// For SSBOs..
	void GenerateVerticesSSBOs();
	void GenerateFacesSSBOs();
	void GenerateSSBOs();

	GLuint numFaces;

	GLuint verticesInfoSSBO[MAX];
	GLuint facesInfoSSBO;
	GLuint visibleFacesIDSSBO;

	GLuint planeDsSSBO;
	GLuint numVisibleFaces;

	// A triangle/face always has three vertices.. 
	// Note that some OBJ meshes only have positions 
	// And all of them have number of faces (assumed)
	std::vector<Triangle>* facesList;

#endif // USE_RAY_TRACING
};