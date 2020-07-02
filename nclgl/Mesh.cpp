#include "Mesh.h"

Mesh::Mesh(void) {
	for (int i = 0; i < MAX_BUFFER; ++i) {
		bufferObject[i] = 0;
	}
	glGenVertexArrays(1, &arrayObject);

	numVertices = 0;
	numIndices = 0;
	texture = 0;
	bumpTexture = 0;
	vertices = NULL;
	colours = NULL;
	textureCoords = NULL;
	indices = NULL;
	normals = NULL;
	tangents = NULL;
	type = GL_TRIANGLES;

	numFaces = 0;
	facesList = NULL;
}

Mesh::~Mesh(void) {
glDeleteVertexArrays(1, &arrayObject);
	glDeleteBuffers(MAX_BUFFER, bufferObject);
	glDeleteTextures(1, &texture);
	glDeleteTextures(1, &bumpTexture);
	delete[] vertices;
	delete[] colours;
	delete[] textureCoords;
	delete[] normals;
	delete[] tangents;
	delete[] indices;
	delete[] facesList;
}

Mesh* Mesh::GenerateTriangle() {
	Mesh * m = new Mesh();
	m->numFaces = 1;
	m->numVertices = 3;

	m->vertices = new Vector3[m->numVertices];
	m->vertices[0] = Vector3(0.0f, 0.5f, 0.0f);
	m->vertices[1] = Vector3(0.5f, -0.5f, 0.0f);
	m->vertices[2] = Vector3(-0.5f, -0.5f, 0.0f);

	m->textureCoords = new Vector2[m->numVertices];
	m->textureCoords[0] = Vector2(0.5f, 0.0f);
	m->textureCoords[1] = Vector2(1.0f, 1.0f);
	m->textureCoords[2] = Vector2(0.0f, 1.0f);
	
	m->colours = new Vector4[m->numVertices];
	m->colours[0] = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
	m->colours[1] = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
	m->colours[2] = Vector4(0.0f, 0.0f, 1.0f, 1.0f);

	// Settings the normals and face here..
	// NOTE: this is just simply for a single triangle..
	m->normals = new Vector3[m->numVertices];
	m->facesList = new Triangle[m->numFaces];
	m->facesList[0] = Triangle();
	for (int i = 0; i < m->numVertices; ++i) {
		m->normals[i] = Vector3(0.0f, 0.0f, -1.0f);
		m->facesList[0].verticesIndices[i] = i;
		m->facesList[0].texCoordsIndices[i] = i;
		m->facesList[0].normalsIndices[i] = i;
	}

	m->BufferData();
	return m;
}

Mesh* Mesh::GenerateQuad() {
	Mesh* m = new Mesh();
	m->numFaces = 2;
	m->numVertices = 4;
	m->type = GL_TRIANGLE_STRIP;
	
	m->vertices = new Vector3[m->numVertices];
	m->textureCoords = new Vector2[m->numVertices];
	m->colours = new Vector4[m->numVertices];
	m->normals = new Vector3[m->numVertices];
	m->tangents = new Vector3[m->numVertices];

	m->vertices[0] = Vector3(-1.0f, -1.0f, 0.0f);
	m->vertices[1] = Vector3(-1.0f, 1.0f, 0.0f);
	m->vertices[2] = Vector3(1.0f, -1.0f, 0.0f);
	m->vertices[3] = Vector3(1.0f, 1.0f, 0.0f);

	m->textureCoords[0] = Vector2(0.0f, 1.0f);
	m->textureCoords[1] = Vector2(0.0f, 0.0f);
	m->textureCoords[2] = Vector2(1.0f, 1.0f);
	m->textureCoords[3] = Vector2(1.0f, 0.0f);

	for (int i = 0; i < 4; ++i) {
		m->colours[i] = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		m->normals[i] = Vector3(0.0f, 0.0f, -1.0f);
		m->tangents[i] = Vector3(1.0f, 0.0f, 0.0f);
	}

	// Faces..
	m->facesList = new Triangle[m->numFaces];
	for (int i = 0; i < m->numFaces; ++i) {
		m->facesList[i] = Triangle();
	}

	// 1st triangle
	for (int i = 0; i < 3; ++i) {
		m->facesList[0].verticesIndices[i] = i;
		m->facesList[0].texCoordsIndices[i] = i;
		m->facesList[0].normalsIndices[i] = i;
	}

	// 2nd triangle
	for (int i = 0, j = 1; i < 3; ++i, ++j) {
		m->facesList[1].verticesIndices[i] = j;
		m->facesList[1].texCoordsIndices[i] = j;
		m->facesList[1].normalsIndices[i] = j;
	}

	m->BufferData();
	return m;
}

Mesh* Mesh::GenerateCube() {
	Mesh* m = new Mesh();
	m->numFaces = 12;
	m->numVertices = 8;
	m->type = GL_TRIANGLE_STRIP;

	m->vertices = new Vector3[m->numVertices];
	m->textureCoords = new Vector2[3];
	m->colours = new Vector4[m->numVertices];
	m->normals = new Vector3[6];
	m->tangents = new Vector3[6];

	m->vertices[0] = Vector3(-1.0f, -0.003248f, 1.0f);
	m->vertices[1] = Vector3(1.0f, -0.003248f, 1.0f);
	m->vertices[2] = Vector3(-1.0f, 1.996752f, 1.0f);
	m->vertices[3] = Vector3(1.0f, 1.996752f, 1.0f);
	m->vertices[4] = Vector3(1.0f, -0.003248f, -1.0f);
	m->vertices[5] = Vector3(-1.0f, -0.003248f, -1.0f);
	m->vertices[6] = Vector3(1.0f, 1.996752f, -1.0f);
	m->vertices[7] = Vector3(-1.0f, 1.996752f, -1.0f);

	m->textureCoords[0] = Vector2(1.0f, 0.0f);
	m->textureCoords[1] = Vector2(0.0f, 0.0f);
	m->textureCoords[2] = Vector2(0.0f, 1.0f);

	for (int i = 0; i < m->numVertices; ++i) {
		m->colours[i] = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	m->normals[0] = Vector3(0.0f, 0.0f, 1.0f);
	m->normals[1] = Vector3(0.0f, 0.0f, -1.0f);
	m->normals[2] = Vector3(-1.0f, 0.0f, 0.0f);
	m->normals[3] = Vector3(1.0f, 0.0f, 0.0f);
	m->normals[4] = Vector3(0.0f, 1.0f, 0.0f);
	m->normals[5] = Vector3(0.0f, -1.0f, 0.0f);

	// Faces..
	m->facesList = new Triangle[m->numFaces];
	
	int index = 0;
	for (int i = 0; i < m->numFaces; ++i) {
		for (int j = 0; j < 3; ++j) {
			m->facesList[i].normalsIndices[j] = index;
			m->facesList[i].texCoordsIndices[j] = j;
		}

		if ((i + 1) % 2 == 0) {
			++index;
		}
	}

	// Positions..
	m->facesList[0].verticesIndices[0] = 0;
	m->facesList[0].verticesIndices[1] = 1;
	m->facesList[0].verticesIndices[2] = 2;

	m->facesList[1].verticesIndices[0] = 1;
	m->facesList[1].verticesIndices[1] = 3;
	m->facesList[1].verticesIndices[2] = 2;

	m->facesList[2].verticesIndices[0] = 4;
	m->facesList[2].verticesIndices[1] = 5;
	m->facesList[2].verticesIndices[2] = 6;

	m->facesList[3].verticesIndices[0] = 5;
	m->facesList[3].verticesIndices[1] = 7;
	m->facesList[3].verticesIndices[2] = 6;

	m->facesList[4].verticesIndices[0] = 2;
	m->facesList[4].verticesIndices[1] = 5;
	m->facesList[4].verticesIndices[2] = 0;

	m->facesList[5].verticesIndices[0] = 2;
	m->facesList[5].verticesIndices[1] = 7;
	m->facesList[5].verticesIndices[2] = 5;

	m->facesList[6].verticesIndices[0] = 6;
	m->facesList[6].verticesIndices[1] = 1;
	m->facesList[6].verticesIndices[2] = 4;

	m->facesList[7].verticesIndices[0] = 6;
	m->facesList[7].verticesIndices[1] = 3;
	m->facesList[7].verticesIndices[2] = 1;

	m->facesList[8].verticesIndices[0] = 2;
	m->facesList[8].verticesIndices[1] = 6;
	m->facesList[8].verticesIndices[2] = 7;

	m->facesList[9].verticesIndices[0] = 2;
	m->facesList[9].verticesIndices[1] = 3;
	m->facesList[9].verticesIndices[2] = 6;

	m->facesList[10].verticesIndices[0] = 1;
	m->facesList[10].verticesIndices[1] = 5;
	m->facesList[10].verticesIndices[2] = 4;

	m->facesList[11].verticesIndices[0] = 1;
	m->facesList[11].verticesIndices[1] = 0;
	m->facesList[11].verticesIndices[2] = 5;

	//m->BufferData();
	return m;
}

void Mesh::BufferData() {
	glBindVertexArray(arrayObject);
	glGenBuffers(1, &bufferObject[VERTEX_BUFFER]);
	glBindBuffer(GL_ARRAY_BUFFER, bufferObject[VERTEX_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vector3), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(VERTEX_BUFFER, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(VERTEX_BUFFER);

	if(colours) { // Just in case the data has no colour attribute ...
		glGenBuffers(1, &bufferObject[COLOUR_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, bufferObject[COLOUR_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vector4), colours, GL_STATIC_DRAW);
		glVertexAttribPointer(COLOUR_BUFFER, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(COLOUR_BUFFER);
	}

	if (textureCoords) {
		glGenBuffers(1, &bufferObject[TEXTURE_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, bufferObject[TEXTURE_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vector2), textureCoords, GL_STATIC_DRAW);
		glVertexAttribPointer(TEXTURE_BUFFER, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(TEXTURE_BUFFER);
	}

	if (normals) {
		glGenBuffers(1, &bufferObject[NORMAL_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, bufferObject[NORMAL_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vector3), normals, GL_STATIC_DRAW);
		glVertexAttribPointer(NORMAL_BUFFER, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(NORMAL_BUFFER);
	}

	if (tangents) {
		glGenBuffers(1, &bufferObject[TANGENT_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, bufferObject[TANGENT_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vector3), tangents, GL_STATIC_DRAW);
		glVertexAttribPointer(TANGENT_BUFFER, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(TANGENT_BUFFER);
	}

	if (indices) {
		glGenBuffers(1, &bufferObject[INDEX_BUFFER]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObject[INDEX_BUFFER]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(GLuint), indices, GL_STATIC_DRAW);
	}

	glBindVertexArray(0);
}

void Mesh::GenerateNormals() {
	if (!normals) {
		normals = new Vector3[numVertices];
	}

	for (int i = 0; i < numVertices; ++i) {
		normals[i] = Vector3();
	}

	if (indices) {
		for (GLuint i = 0; i < numIndices; i+=3) {
			GLuint a = indices[i];
			GLuint b = indices[i + 1];
			GLuint c = indices[i + 2];

			Vector3 normal = Vector3::Cross(vertices[b] - vertices[a], vertices[c] - vertices[a]);

			normals[a] += normal;
			normals[b] += normal;
			normals[c] += normal;
		}
	}
	else {
		for (GLuint i = 0; i < numVertices; i += 3) {
			Vector3& a = vertices[i];
			Vector3& b = vertices[i + 1];
			Vector3& c = vertices[i + 2];

			Vector3 normal = Vector3::Cross(b - a, c - a);

			normals[i] = normal;
			normals[i + 1] = normal;
			normals[i + 2] = normal;
		}
	}

	for (GLuint i = 0; i < numVertices; ++i) {
		normals[i].Normalise();
	}
}

void Mesh::GenerateTangents() {
	if (!tangents) {
		tangents = new Vector3[numVertices];
	}

	if (!textureCoords) {
		return;
	}

	for (GLuint i = 0; i < numVertices; ++i) {
		tangents[i] = Vector3();
	}

	if (indices) {
		for (GLuint i = 0; i < numIndices; i+=3) {
			int a = indices[i];
			int b = indices[i + 1];
			int c = indices[i + 2];

			Vector3 tangent = GenerateTangent(vertices[a], vertices[b], vertices[c], textureCoords[a], textureCoords[b], textureCoords[c]);

			tangents[a] += tangent;
			tangents[b] += tangent;
			tangents[c] += tangent;
		}
	}
	else {
		for (GLuint i = 0; i < numVertices; i+=3) {
			Vector3 tangent = GenerateTangent(vertices[i], vertices[i + 1], vertices[i + 2], textureCoords[i], textureCoords[i + 1], textureCoords[i + 2]);

			tangents[i] += tangent;
			tangents[i + 1] += tangent;
			tangents[i + 2] += tangent;
		}
	}

	for (GLuint i = 0; i < numVertices; ++i) {
		tangents[i].Normalise();
	}
}

Vector3 Mesh::GenerateTangent(const Vector3& a, const Vector3& b, const Vector3& c, const Vector2& ta, const Vector2& tb, const Vector2& tc) {
	Vector2 coord1 = tb - ta;
	Vector2 coord2 = tc - ta;

	Vector3 vertex1 = b - a;
	Vector3 vertex2 = c - a;

	Vector3 axis = Vector3(vertex1 * coord2.y - vertex2 * coord1.y);

	float factor = 1.0f / (coord1.x * coord2.y - coord2.x * coord1.y);

	return axis * factor;
}

void Mesh::Draw() {
	glBindVertexArray(arrayObject);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bumpTexture);

	if (bufferObject[INDEX_BUFFER]) {
		glDrawElements(type, numIndices, GL_UNSIGNED_INT, 0);
	}
	else {
		glDrawArrays(type, 0, numVertices);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}