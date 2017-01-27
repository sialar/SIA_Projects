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

	// Compute weights :
	if (_meth)
		computeWeights();
	else 
		loadWeights("data/skinning.txt");

	// Test skinning :
	animate();
}

void Skinning::recomputeWeights() {
	if (_skin==NULL) return;
	if (_skel==NULL) return;

	// Compute weights :
	if (_meth) {
		cout << "computing weights\n";
		computeWeights();
	} else {
		cout << "loading weights\n";
		loadWeights("data/skinning.txt");
	}

	// Test skinning :
	animate();
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
		for (int j = 0 ; j < 4 ; j++) {
			for (int k = 0 ; k < 4 ; k++) {
				_transfoCurr[(*idx)][k][j] = ptr[i];
				i++;
			}
		}
		for (unsigned int ichild = 0 ; ichild < skel->_children.size() ; ichild++) {
			(*idx)++;
			computeTransfo(skel->_children[ichild], idx);
		}
	}
	glPopMatrix();
	_transfoCurr[i0] = glm::transpose(_transfoCurr[i0]);
}


void Skinning::computeWeights() {
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
			while (pch != NULL && iJt<_nbJoints) {
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