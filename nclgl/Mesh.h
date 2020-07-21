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

	// Clear the main SSBOs..
	void ResetSSBOs();

	// Mainly for compute shaders..
	GLuint GetNumFaces() const { return numFaces; }

	void UpdateCollectedID();

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
	Vector3* vertices;
	Vector4* colours;
	Vector2* textureCoords;
	Vector3* normals;
	Vector3* tangents;
	GLuint* indices;

#ifdef USE_RAY_TRACING

	// For SSBOs..
	void GenerateVerticesSSBOs();
	void GenerateFacesSSBOs();
	void GenerateSSBOs();

	GLuint numFaces;

	GLuint verticesInfoSSBO[MAX];
	GLuint facesInfoSSBO;
	GLuint selectedFacesIDSSBO;

	// further testing..
	GLuint idAtomicCounter;
	GLuint parentBoxSSBO;

	// A triangle/face always has three vertices.. 
	// Note that some OBJ meshes only have positions 
	// And all of them have number of faces (assumed)
	Triangle* facesList;

#endif // USE_RAY_TRACING

};