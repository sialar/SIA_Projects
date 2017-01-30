#include "skinning.h"

using namespace std;

void Skinning::init() {
	if (_skin==NULL) return;
	if (_skel==NULL) return;

	// Compute number of joints :
	getJoints(_skel);
	_nbJoints = _joints.size();

	// Get mesh info :
	_nbVtx = _skin->_points.size();
	_weights.resize(_nbVtx);
	_pointsInit.resize(_nbVtx);
	for (int iv = 0 ; iv < _nbVtx ; iv++) {
		_weights[iv].resize(_nbJoints, 0);
		_pointsInit[iv] = _skin->_points[iv];
		_pointsInit[iv][3] = 1.0;
	}

	// Get transfo joint info :
	_transfoInit.resize(_nbJoints);
	_transfoInitInv.resize(_nbJoints);
	_transfoCurr.resize(_nbJoints);
	int idx = 0;
	glPushMatrix();
	glLoadIdentity();
	computeTransfo(_skel, &idx);
	glPopMatrix();
	for (unsigned int i = 0 ; i < _transfoCurr.size() ; i++) {
		_transfoInit[i] = _transfoCurr[i];
		_transfoInitInv[i] = glm::inverse(_transfoInit[i]);
	}

	// Get bones pose info :
	idx = 0;
	_posBonesInit.resize(_nbJoints);
	getBonesPos(_skel, &idx);

	recomputeWeights();
}

void Skinning::getJoints(Skeleton *skel) {
	_joints.push_back(skel);
	for (unsigned int ichild = 0 ; ichild < skel->_children.size() ; ichild++) {
		getJoints(skel->_children[ichild]);
	}
}
void Skinning::getBonesPos(Skeleton *skel, int *idx) {
	int i0 = (*idx);
	qglviewer::Vec pos(_transfoInit[i0][3][0], _transfoInit[i0][3][1], _transfoInit[i0][3][2]);
	for (unsigned int ichild = 0 ; ichild < skel->_children.size() ; ichild++) {
		(*idx)++;
		pos+=qglviewer::Vec(_transfoInit[(*idx)][3][0], _transfoInit[(*idx)][3][1], _transfoInit[(*idx)][3][2]);
		getBonesPos(skel->_children[ichild], idx);
	}
	pos/=(float)(skel->_children.size()+1);
	_posBonesInit[i0] = glm::vec4(pos.x, pos.y, pos.z, 1.0);
}

void Skinning::recomputeWeights() {
	if (_skin == NULL) return;
	if (_skel == NULL) return;

	// Compute weights :
	if (_meth == 0) {
		cout << "loading weights\n";
		loadWeights("data/skinning.txt");
	}
	else if (_meth == 1) {
		cout << "computing weights (skinning rigid)\n";
		computeRigidWeights();
	}
	else if (_meth == 2) {
		cout << "computing weights (cylindric distance)\n";
		computeCylindricWeights();
	}

	// Test skinning :
	animate();
}

void Skinning::computeTransfo(Skeleton *skel, int *idx) {
	int i0 = (*idx);
	glPushMatrix();
	{
		glTranslatef(skel->_offX, skel->_offY, skel->_offZ);
		glTranslatef(skel->_curTx, skel->_curTy, skel->_curTz);
		skel->rotateSkeleton();

		float ptr[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, ptr);
		int i = 0;
		for (int j = 0; j < 4; j++) {
			for (int k = 0; k < 4; k++) {
				_transfoCurr[(*idx)][k][j] = ptr[i];
				i++;
			}
		}
		for (unsigned int ichild = 0; ichild < skel->_children.size(); ichild++) {
			(*idx)++;
			computeTransfo(skel->_children[ichild], idx);
		}
	}
	glPopMatrix();
	_transfoCurr[i0] = glm::transpose(_transfoCurr[i0]);
}


void Skinning::computeRigidWeights() {
	if (_skin == NULL) return;
	if (_skel == NULL) return;
	// weights[i][j] = influence of j-th joint on i-th vertex

	_weights.resize(_nbVtx);
	for (int i = 0; i < _nbVtx; i++)
		_weights[i].resize(_nbJoints);

	for (int i = 0; i < _nbVtx; i++)
	{
		int min_index = 0;
		float min_dist = glm::distance(_posBonesInit[0], _pointsInit[0]);
		for (int j = 1; j < _nbJoints; j++)
		{
			_weights[i][j] = 0;
			if (glm::distance(_posBonesInit[j], _pointsInit[i]) < min_dist)
			{
				min_index = j;
				min_dist = glm::distance(_posBonesInit[j], _pointsInit[i]);
			}
		}
		_weights[i][min_index] = 1;
	}
}

glm::vec3 toVec3(glm::vec4 v) {
	return glm::vec3(v.x, v.y, v.z);
}

double cylindricDistance(glm::vec4 c, glm::vec3 ab, glm::vec4 v) {
	// offset = vector ab
	// center = center of ab
	glm::vec3 center = toVec3(c);
	glm::vec3 vertex = toVec3(v);
	glm::vec3 halfAB(ab);
	halfAB /= 2;
	glm::vec3 a, b, i; // points
	a = center + ab;
	b = center - ab;
	glm::vec3 u1, u2; // vectors
	u1 = vertex - a;
	u2 = ab;

	double sqrU2Norm = pow(glm::length(u2), 2);
	if (sqrU2Norm) {
		double p = glm::dot(u1, u2) / sqrU2Norm;
		u2 *= p;
		i = (p < 0) ? a : ((p > 1) ? b : a + u2);
	}
	else {
		i = b;
	}
	return glm::length(vertex - i);
}

void Skinning::computeCylindricWeights() {
	if (_skin == NULL) return;
	if (_skel == NULL) return;
	// weights[i][j] = influence of j-th joint on i-th vertex

	_weights.resize(_nbVtx);
	for (int i = 0; i < _nbVtx; i++)
		_weights[i].resize(_nbJoints);

	for (int i = 0; i < _nbVtx; i++)
	{
		int min_index = 0;
		double new_dist;
		glm::vec3 offset(_joints[1]->_offX, _joints[1]->_offY, _joints[1]->_offZ);
		float min_dist = cylindricDistance(_posBonesInit[0], offset, _pointsInit[0]);
		//float min_dist1 = glm::distance(_posBonesInit[0], _pointsInit[0]);
		for (int j = 1; j < _nbJoints + 1; j++)
		{
			_weights[i][j] = 0;
			offset = glm::vec3(_joints[j - 1]->_offX, _joints[j - 1]->_offY, _joints[j - 1]->_offZ);
			new_dist = cylindricDistance(_posBonesInit[j], offset, _pointsInit[i]);
			//min_dist1 = glm::distance(_posBonesInit[j], _pointsInit[i]);
			//cout << new_dist << " " << min_dist1 << endl;
			if (new_dist < min_dist)
			{
				min_index = j;
				min_dist = new_dist;
			}
		}
		_weights[i][min_index] = 1;
	}
}

void Skinning::loadWeights(std::string filename) {

	_weights.resize(_nbVtx);
	for (int i = 0; i < _nbVtx; i++)
		_weights[i].resize(_nbJoints);

	std::vector<float> bone_indexA;
	std::vector<float> bone_weightA;
	FILE *file; fopen_s(&file, filename.data(), "r");
	if (!file) return;
	char * pch, *next_token;
	const int line_size = 600;
	char line[line_size];
	int iV = 0, iJt = 0;
	
	while (!feof(file)) {
		// for each line i.e. for each vertex :
		if (fgets(line, line_size, file)) {
			iJt = 0;
			float x;
			pch = strtok_s(line," ", &next_token);
			while (pch != NULL) {
				// for each number i.e. for each joint :
				if (pch[0]=='\n'){
				} else {
					x = (float)atof(pch);
					_weights[iV][iJt] = x;
				}
				pch = strtok_s(NULL, " ", &next_token);
				iJt++;
			}
			iV++;
		}		
	}
	fclose(file);
}

void Skinning::paintWeights(std::string jointName) {
	if (_skin==NULL) return;
	if (_skel==NULL) return;

	int jointIndex = 0;
	while (jointIndex < _nbJoints && _joints[jointIndex]->_name.compare(jointName))
		jointIndex++;
	//cout << jointIndex << ": " << jointName << endl;
	int nb = 0;
	_skin->_colors.resize(_skin->_points.size());

	for (int i = 0; i < _nbVtx; i++)
		_skin->_colors[i].x = _weights[i][jointIndex];
}

void Skinning::animate() {
	if (_skin==NULL) return;
	if (_skel==NULL) return;

	// Animate bones :
	int idx = 0;
	glPushMatrix();
	glLoadIdentity();
	computeTransfo(_skel, &idx);
	glPopMatrix();

	// Animate skin :
#if _SKINNING_GPU
#else
	applySkinning();
#endif
}

void Skinning::applySkinning() {
	if (!_keepAppling) return;
	for (int i = 0; i < _nbVtx; i++) {
		glm::vec4 newpos(0);
		for (int j = 0; j < _nbJoints; j++)
			newpos += _weights[i][j] * _transfoCurr[j] * _transfoInitInv[j] * _pointsInit[i];
		_skin->_points[i] = newpos;
	}
}