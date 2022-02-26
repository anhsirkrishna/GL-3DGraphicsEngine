#include "ObjReader.h"
#include<string>
#include<sstream>

void ObjReader::Read(std::string file_name) {
	std::string file_path = ".\\models\\" + file_name;
	std::string line;
	std::string v;
	std::ifstream obj_fstream(file_name);
	glm::vec3 vertex;
	glm::vec2 v2;
	while (!obj_fstream) {
		std::getline(obj_fstream, line);
		std::istringstream line_stream(line);
		if (line[0] == 'v') {
			vertex = glm::vec3(0, 0, 0);
			line_stream >> v >> vertex.x >> vertex.y >> vertex.z;
			obj_file.vertices.push_back(vertex);
		}
		if (line[0] == 'vn') {
			vertex = glm::vec3(0, 0, 0);
			line_stream >> v >> vertex.x >> vertex.y >> vertex.z;
			obj_file.vertex_normals.push_back(vertex);
		}
		if (line[0] == 'vt') {
			v2 = glm::vec2(0);
			line_stream >> v >> v2.x >> v2.y;
			obj_file.vertex_texture_coords.push_back(v2);
		}
		if (line[0] == 'f') {

		}
	}
}