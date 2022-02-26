#pragma once
#include<vector>
#include<glm/glm.hpp>
#include <fstream>

struct objFile
{
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> vertex_normals;
	std::vector<glm::vec2> vertex_texture_coords;
	std::vector<glm::vec3> vertex_indeces;
};

class ObjReader {
public:
	void Read(std::string file_name);
	objFile obj_file;
};