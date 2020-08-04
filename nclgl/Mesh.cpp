#include "Mesh.h"

Mesh::Mesh(void) {
#ifdef USE_RAY_TRACING
	facesList = NULL;
	numFaces = 0;
#endif

	for (int i = 0; i < MAX_BUFFER; ++i) {
		bufferObject[i] = 0;
	}
	glGenVertexArrays(1, &arrayObject);

	vertices = NULL;
	colours = NULL;
	textureCoords = NULL;
	normals = NULL;
	tangents = NULL;
	indices = NULL;
	
	numVertices = 0;
	numNormals = 0;
	numTexCoords = 0;
	numIndices = 0;

	texture = 0;
	bumpTexture = 0;

	numVisibleFaces = 0;

	type = GL_TRIANGLES;
}

Mesh::~Mesh(void) {
	glDeleteVertexArrays(1, &arrayObject);
	glDeleteTextures(1, &texture);
	glDeleteTextures(1, &bumpTexture);

	// data..
	delete vertices;
#ifdef USE_RAY_TRACING
	delete facesList;
#endif // USE_RAY_TRACING
	delete colours;
	delete textureCoords;
	delete normals;
	delete tangents;
	delete indices;

	// Buffers..
	glDeleteBuffers(MAX_BUFFER, bufferObject);
#ifdef USE_RAY_TRACING
	glDeleteBuffers(MAX, verticesInfoSSBO);
	glDeleteBuffers(1, &facesInfoSSBO);
	glDeleteBuffers(1, &visibleFacesIDSSBO);
	glDeleteBuffers(1, &planeDsSSBO);
#endif
}

Mesh* Mesh::GenerateTriangle() {
	Mesh * m = new Mesh();
	
	m->numVertices = m->numTexCoords = m->numNormals = 3;

	m->vertices = new std::vector<Vector3>(m->numVertices);
	m->vertices->at(0) = Vector3(0.0f, 0.5f, 0.0f);
	m->vertices->at(1) = Vector3(0.5f, -0.5f, 0.0f);
	m->vertices->at(2) = Vector3(-0.5f, -0.5f, 0.0f);

	m->textureCoords = new std::vector<Vector2>(m->numTexCoords);
	m->textureCoords->at(0) = Vector2(0.5f, 0.0f);
	m->textureCoords->at(1) = Vector2(1.0f, 1.0f);
	m->textureCoords->at(2) = Vector2(0.0f, 1.0f);
	
	m->colours = new std::vector<Vector4>(m->numVertices);
	m->colours->at(0) = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
	m->colours->at(1) = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
	m->colours->at(2) = Vector4(0.0f, 0.0f, 1.0f, 1.0f);

	m->BufferData();

	return m;
}

Mesh* Mesh::GenerateQuad() {
	Mesh* m = new Mesh();
	
	m->numVertices = m->numTexCoords = m->numNormals = 4;
	m->type = GL_TRIANGLE_STRIP;
	
	m->vertices = new std::vector<Vector3>(m->numVertices);
	m->textureCoords = new std::vector<Vector2>(m->numTexCoords);
	m->colours = new std::vector<Vector4>(m->numVertices);
	m->normals = new std::vector<Vector3>(m->numNormals);
	m->tangents = new std::vector<Vector3>(m->numVertices);

	m->vertices->at(0) = Vector3(-1.0f, -1.0f, 0.0f);
	m->vertices->at(1) = Vector3(-1.0f, 1.0f, 0.0f);
	m->vertices->at(2) = Vector3(1.0f, -1.0f, 0.0f);
	m->vertices->at(3) = Vector3(1.0f, 1.0f, 0.0f);

	m->textureCoords->at(0) = Vector2(0.0f, 1.0f);
	m->textureCoords->at(1) = Vector2(0.0f, 0.0f);
	m->textureCoords->at(2) = Vector2(1.0f, 1.0f);
	m->textureCoords->at(3) = Vector2(1.0f, 0.0f);

	for (int i = 0; i < 4; ++i) {
		m->colours->at(i) = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		m->normals->at(i) = Vector3(0.0f, 0.0f, -1.0f);
		m->tangents->at(i) = Vector3(1.0f, 0.0f, 0.0f);
	}

	m->BufferData();
	return m;
}

void Mesh::BufferData() {
	glBindVertexArray(arrayObject);
	glGenBuffers(1, &bufferObject[VERTEX_BUFFER]);
	glBindBuffer(GL_ARRAY_BUFFER, bufferObject[VERTEX_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vector3), vertices->data(), GL_STATIC_DRAW);
	glVertexAttribPointer(VERTEX_BUFFER, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(VERTEX_BUFFER);

	if(colours) { // Just in case the data has no colour attribute ...
		glGenBuffers(1, &bufferObject[COLOUR_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, bufferObject[COLOUR_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vector4), colours->data(), GL_STATIC_DRAW);
		glVertexAttribPointer(COLOUR_BUFFER, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(COLOUR_BUFFER);
	}

	if (textureCoords) {
		glGenBuffers(1, &bufferObject[TEXTURE_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, bufferObject[TEXTURE_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vector2), textureCoords->data(), GL_STATIC_DRAW);
		glVertexAttribPointer(TEXTURE_BUFFER, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(TEXTURE_BUFFER);
	}

	if (normals) {
		glGenBuffers(1, &bufferObject[NORMAL_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, bufferObject[NORMAL_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vector3), normals->data(), GL_STATIC_DRAW);
		glVertexAttribPointer(NORMAL_BUFFER, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(NORMAL_BUFFER);
	}

	if (tangents) {
		glGenBuffers(1, &bufferObject[TANGENT_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, bufferObject[TANGENT_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vector3), tangents->data(), GL_STATIC_DRAW);
		glVertexAttribPointer(TANGENT_BUFFER, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(TANGENT_BUFFER);
	}

	if (indices) {
		glGenBuffers(1, &bufferObject[INDEX_BUFFER]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObject[INDEX_BUFFER]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(GLuint), indices->data(), GL_STATIC_DRAW);
	}

	glBindVertexArray(0);
}

#ifdef USE_RAY_TRACING

Mesh* Mesh::GenerateTriangleRayTracing() {
	Mesh* m = new Mesh();

	m->numVertices = m->numTexCoords = m->numNormals = 3;

	m->vertices = new std::vector<Vector3>(m->numVertices);
	m->vertices->at(0) = Vector3(0.0f, 0.5f, 0.0f);
	m->vertices->at(1) = Vector3(0.5f, -0.5f, 0.0f);
	m->vertices->at(2) = Vector3(-0.5f, -0.5f, 0.0f);

	m->textureCoords = new std::vector<Vector2>(m->numTexCoords);
	m->textureCoords->at(0) = Vector2(0.5f, 0.0f);
	m->textureCoords->at(1) = Vector2(1.0f, 1.0f);
	m->textureCoords->at(2) = Vector2(0.0f, 1.0f);

	m->colours = new std::vector<Vector4>(m->numVertices);
	m->colours->at(0) = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
	m->colours->at(1) = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
	m->colours->at(2) = Vector4(0.0f, 0.0f, 1.0f, 1.0f);

	m->numFaces = 1;
	// Settings the normals and face here..
	// NOTE: this is just simply for a single triangle..
	m->normals = new std::vector<Vector3>(m->numNormals);
	m->facesList = new std::vector<Triangle>(m->numFaces);
	m->facesList->at(0) = Triangle();
	// this allows the triangle's normal to be correctly calculated due to the whole negative z thingy
	// normal way can be used as well, but then it would requires to flip the camera direction calculation..
	for (int i = 0; i < m->numVertices; ++i) {
		m->normals->at(i) = Vector3(0.0f, 0.0f, -1.0f);
		m->facesList->at(0).verticesIndices[i] = 2 - i;
		m->facesList->at(0).texCoordsIndices[i] = 2 - i;
		m->facesList->at(0).normalsIndices[i] = 2 - i;
	}

	m->GenerateSSBOs();

	return m;
}

Mesh* Mesh::GenerateQuadRayTracing() {
	Mesh* m = new Mesh();

	m->numVertices = m->numTexCoords = m->numNormals = 4;
	m->type = GL_TRIANGLE_STRIP;

	m->vertices = new std::vector<Vector3>(m->numVertices);
	m->textureCoords = new std::vector<Vector2>(m->numTexCoords);
	m->colours = new std::vector<Vector4>(m->numVertices);
	m->normals = new std::vector<Vector3>(m->numNormals);
	m->tangents = new std::vector<Vector3>(m->numVertices);

	m->vertices->at(0) = Vector3(-1.0f, -1.0f, 0.0f);
	m->vertices->at(1) = Vector3(-1.0f, 1.0f, 0.0f);
	m->vertices->at(2) = Vector3(1.0f, -1.0f, 0.0f);
	m->vertices->at(3) = Vector3(1.0f, 1.0f, 0.0f);

	m->textureCoords->at(0) = Vector2(0.0f, 1.0f);
	m->textureCoords->at(1) = Vector2(0.0f, 0.0f);
	m->textureCoords->at(2) = Vector2(1.0f, 1.0f);
	m->textureCoords->at(3) = Vector2(1.0f, 0.0f);

	for (int i = 0; i < 4; ++i) {
		m->colours->at(i) = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		m->normals->at(i) = Vector3(0.0f, 0.0f, -1.0f);
		m->tangents->at(i) = Vector3(1.0f, 0.0f, 0.0f);
	}

	// Faces..
	m->numFaces = 2;
	m->facesList = new std::vector<Triangle>(m->numFaces);
	for (int i = 0; i < m->numFaces; ++i) {
		m->facesList->at(i) = Triangle();
	}

	// 1st triangle
	for (int i = 0; i < 3; ++i) {
		m->facesList->at(0).verticesIndices[2 - i] = i;
		m->facesList->at(0).texCoordsIndices[2 - i] = i;
		m->facesList->at(0).normalsIndices[2 - i] = i;
	}

	// 2nd triangle
	for (int i = 0, j = 1; i < 3; ++i, ++j) {
		m->facesList->at(1).verticesIndices[i] = j;
		m->facesList->at(1).texCoordsIndices[i] = j;
		m->facesList->at(1).normalsIndices[i] = j;
	}

	m->GenerateSSBOs();

	return m;
}

void Mesh::GenerateVerticesSSBOs() {
	std::vector<Vector4> temp;	// need this to convert vec3 to vec4, otherwise it's not gonna be fun..

	// Positions
	for (unsigned int i = 0; i < numVertices; ++i) {
		temp.push_back(Vector4(vertices->at(i).x, vertices->at(i).y, vertices->at(i).z, 1.0));
	}
	glGenBuffers(1, &verticesInfoSSBO[POSITION]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesInfoSSBO[POSITION]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numVertices * sizeof(Vector4), temp.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, POSITION, verticesInfoSSBO[POSITION]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	temp.clear();

	// Perhaps we don't need this..
	if (colours) {
		glGenBuffers(1, &verticesInfoSSBO[COLOUR]);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesInfoSSBO[COLOUR]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, numVertices * sizeof(Vector4), colours->data(), GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, COLOUR, verticesInfoSSBO[COLOUR]);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	// Texture coordinates 
	if (textureCoords) {
		glGenBuffers(1, &verticesInfoSSBO[TEX_COORD]);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesInfoSSBO[TEX_COORD]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, numTexCoords * sizeof(Vector2), textureCoords->data(), GL_STATIC_DRAW);	// change this later..
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, TEX_COORD, verticesInfoSSBO[TEX_COORD]);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	// Normals
	for (int i = 0; i < numNormals; ++i) {
		temp.push_back(Vector4(normals->at(i).x, normals->at(i).y, normals->at(i).z, 1.0f));
	}
	glGenBuffers(1, &verticesInfoSSBO[NORMAL]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesInfoSSBO[NORMAL]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numNormals * sizeof(Vector4), temp.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NORMAL, verticesInfoSSBO[NORMAL]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	temp.clear();
}

void Mesh::GenerateFacesSSBOs() {
	// Faces
	glGenBuffers(1, &facesInfoSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, facesInfoSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numFaces * sizeof(Triangle), facesList->data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MAX + 1, facesInfoSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Face ID 
	glGenBuffers(1, &visibleFacesIDSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleFacesIDSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numFaces * sizeof(GLint), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MAX, visibleFacesIDSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Planes 
	GLuint numDs = 38;
	glGenBuffers(1, &planeDsSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, planeDsSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numDs * sizeof(float), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MAX + 2, planeDsSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Mesh::GenerateSSBOs() {
	GenerateVerticesSSBOs();
	GenerateFacesSSBOs();
}

void Mesh::UpdateCollectedID() {
	std::vector<GLint> collectedID;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleFacesIDSSBO);
	GLint* ptr = (GLint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	for (int i = 0; i < numFaces; ++i) {
		if (ptr[i] != -1) {
			collectedID.push_back(ptr[i]);
		}
	}

	numVisibleFaces = static_cast<GLuint>(collectedID.size());

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, collectedID.size() * sizeof(GLint), collectedID.data());
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

#endif // USE_RAY_TRACING

void Mesh::GenerateNormals() {
	if (!normals) {
		normals = new std::vector<Vector3>(numVertices);
	}

	for (int i = 0; i < numVertices; ++i) {
		normals->at(i) = Vector3();
	}

	if (indices) {
		for (GLuint i = 0; i < numIndices; i+=3) {
			GLuint a = indices->at(i);
			GLuint b = indices->at(i + 1);
			GLuint c = indices->at(i + 2);

			Vector3 normal = Vector3::Cross(vertices->at(b) - vertices->at(a), vertices->at(c) - vertices->at(a));

			normals->at(a) += normal;
			normals->at(b) += normal;
			normals->at(c) += normal;
		}
	}
	else {
		for (GLuint i = 0; i < numVertices; i += 3) {
			Vector3& a = vertices->at(i);
			Vector3& b = vertices->at(i + 1);
			Vector3& c = vertices->at(i + 2);

			Vector3 normal = Vector3::Cross(b - a, c - a);

			normals->at(i) = normal;
			normals->at(i + 1) = normal;
			normals->at(i + 2) = normal;
		}
	}

	for (GLuint i = 0; i < numVertices; ++i) {
		normals->at(i).Normalise();
	}
}

void Mesh::GenerateTangents() {
	if (!tangents) {
		tangents = new std::vector<Vector3>(numVertices);
	}

	if (!textureCoords) {
		return;
	}

	for (GLuint i = 0; i < numVertices; ++i) {
		tangents->at(i) = Vector3();
	}

	if (indices) {
		for (GLuint i = 0; i < numIndices; i+=3) {
			int a = indices->at(i);
			int b = indices->at(i + 1);
			int c = indices->at(i + 2);

			Vector3 tangent = GenerateTangent(vertices->at(a), vertices->at(b), vertices->at(c), textureCoords->at(a), textureCoords->at(b), textureCoords->at(c));

			tangents->at(a) += tangent;
			tangents->at(b) += tangent;
			tangents->at(c) += tangent;
		}
	}
	else {
		for (GLuint i = 0; i < numVertices; i+=3) {
			Vector3 tangent = GenerateTangent(vertices->at(i), vertices->at(i + 1), vertices->at(i + 2), textureCoords->at(i), textureCoords->at(i + 1), textureCoords->at(i + 2));

			tangents->at(i) += tangent;
			tangents->at(i + 1) += tangent;
			tangents->at(i + 2) += tangent;
		}
	}

	for (GLuint i = 0; i < numVertices; ++i) {
		tangents->at(i).Normalise();
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