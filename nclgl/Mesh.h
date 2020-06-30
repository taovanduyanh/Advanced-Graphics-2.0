#pragma once
#include "OGLRenderer.h"

enum MeshBuffer {
	VERTEX_BUFFER, COLOUR_BUFFER, TEXTURE_BUFFER, NORMAL_BUFFER, TANGENT_BUFFER, INDEX_BUFFER, MAX_BUFFER
};



class Mesh {
public:
	struct Triangle {
		GLuint verticesIndices[3];
		GLuint texCoordsIndices[3];
		GLuint normalsIndices[3];
	};

	Mesh(void);
	~Mesh(void);

	virtual void Draw();
	static Mesh* GenerateTriangle();
	static Mesh* GenerateQuad();

	void SetTexutre(GLuint tex) { texture = tex; }
	GLuint GetTexture() { return texture; }

	void SetBumpMap(GLuint tex) { bumpTexture = tex; }
	GLuint GetBumpMap() { return bumpTexture; }

	// For sending info into SSBO for ray tracing
	GLuint GetNumVertices() const { return numVertices; }
	Vector3* GetPositions() const { return vertices; }
	Vector4* GetColours() const { return colours; }
	Vector2* GetTexCoords() const { return textureCoords; }
	Vector3* GetNormals() const { return normals; }


	GLuint GetNumFaces() const { return numFaces; }
	Triangle* GetMeshFaces() const { return facesList; }

protected:
	void BufferData();
	void GenerateNormals();
	Vector3 GenerateTangent(const Vector3& a, const Vector3& b, const Vector3& c, const Vector2& ta, const Vector2& tb, const Vector2& tc);
	void GenerateTangents();

	GLuint arrayObject;
	GLuint bufferObject[MAX_BUFFER];
	GLuint numVertices;
	GLuint numIndices;
	GLuint texture;
	GLuint bumpTexture;

	GLuint type;

	Vector3* vertices;
	Vector4* colours;
	Vector2* textureCoords;
	Vector3* normals;
	Vector3* tangents;
	GLuint* indices;

	// A triangle/face always has three vertices.. 
	// Note that some OBJ meshes only have positions 
	// But it should be fine..
	// And all of them have number of faces (assumed)
	GLuint numFaces;
	Triangle* facesList;
};