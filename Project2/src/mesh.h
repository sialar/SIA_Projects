#ifndef _MESH_H_
#define _MESH_H_

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Mesh {
public :
	std::vector<glm::vec4> _points;						// vertices
	std::vector<glm::vec3> _normals;					// normals per point
	std::vector<glm::vec4> _colors;						// colors per point
	std::vector<glm::vec4> _colors0;						// colors per point
	std::vector<std::vector<unsigned int>> _faces;	// faces[i] = list of vertex indices of face i
	std::vector<unsigned int> _triangles;			// list of vertex indices : triangles[3*i] triangles[3*i+1] triangles[3*i+2] define a face

	glm::vec4 _color;								// if colors empty : color of all vertices

	int _nbEdges; // mesh of triangles or quads?
	bool _keepDrawing;

public:
	Mesh() {
		_color = glm::vec4(1.0, 0.6, 0.6, 0.9);
		_keepDrawing = true;
	}

	~Mesh() {
		_points.clear();
		_normals.clear();
		_faces.clear();
		_triangles.clear();
	}

	void initColor() {
		for (int i = 0; i < int(_colors.size()); i++)
			_colors[i] = _color;
	}
	void load(const char* fileName);

	void setColor(glm::vec4 col) { _color = col; };
	void draw();
};

#endif