#include "HeightMap.h"

HeightMap::HeightMap(string name) {
	ifstream file(name.c_str(), ios::binary);

	if (!file) {
		return;
	}

	numVertices = RAW_WIDTH * RAW_HEIGHT;
	numIndices = (RAW_WIDTH - 1) * (RAW_HEIGHT - 1) * 6;
	vertices = new std::vector<Vector3>(numVertices);
	textureCoords = new std::vector<Vector2>(numVertices);
	indices = new std::vector<GLuint>(numIndices);

	unsigned char* data = new unsigned char[numVertices];
	file.read((char*)data, numVertices * sizeof(unsigned char));
	file.close();

	for (int x = 0; x < RAW_WIDTH; ++x) {
		for (int z = 0; z < RAW_HEIGHT; ++z) {
			int offset = (x * RAW_WIDTH) + z;

			vertices->at(offset) = Vector3(x * HEIGHTMAP_X, data[offset] * HEIGHTMAP_Y, z * HEIGHTMAP_Z);
			textureCoords->at(offset) = Vector2(x * HEIGHTMAP_TEX_X, z * HEIGHTMAP_TEX_Z);
		}
	}

	delete[] data;

	numIndices = 0; 

	for (int x = 0; x < RAW_WIDTH - 1; ++x) {
		for (int z = 0; z < RAW_HEIGHT - 1; ++z) {
			int a = x * RAW_WIDTH + z;
			int b = (x + 1) * RAW_WIDTH + z;
			int c = (x + 1) * RAW_WIDTH + z + 1;
			int d = x * RAW_WIDTH + z + 1;

			indices->at(numIndices++) = c;
			indices->at(numIndices++) = b;
			indices->at(numIndices++) = a;

			indices->at(numIndices++) = a;
			indices->at(numIndices++) = d;
			indices->at(numIndices++) = c;
		} 
	}

	GenerateNormals();
	GenerateTangents();
	BufferData();
}