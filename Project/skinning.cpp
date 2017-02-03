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
void Skinning::computeTransfo(Skeleton *skel, int *idx) {
	int i0 = (*idx);
	glPushMatrix();
	{
		glTranslatef(skel->_offX, skel->_offY, skel->_offZ);
		glTranslatef(skel->_curTx, skel->_curTy, skel->_curTz);
		skel->rotateSkeleton();

		float ptr[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, ptr);
		for (int i = 0; i < 16; i++)
			if (ptr[i] != ptr[i]) {
				cout << "\nError: nan when calling glGetFloatv" << endl;
				cout << "Rerun the program" << endl;
				exit(1);
			}
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
std::vector<int> Skinning::getChildrenIndex(int parentIndex) {
	Skeleton* parent = _joints[parentIndex];
	std::vector<int> childrenIndex;
	for (Skeleton* child : parent->_children)
		childrenIndex.push_back(child->_index);
	return childrenIndex;
}

glm::vec3 Skinning::meanChildrenPos(int parentIndex) {
	Skeleton* parent = _joints[parentIndex];
	glm::vec3 res(0.0);
	for (Skeleton* child : parent->_children)
		res += glm::vec3(_posBonesInit[child->_index]);
	res /= parent->_children.size();
	return res;
}

int Skinning::closetChildIndex(int parentIndex) {
	Skeleton* parent = _joints[parentIndex];
	if (parent->_children.size()==0)
		return -1;

	Skeleton* closestChild = parent->_children[0];
	glm::vec3 offset = glm::vec3(closestChild->_offX, closestChild->_offY, closestChild->_offZ);
	double temp_offset, min_offset = glm::length(offset);
	int closestChildIndex = closestChild->_index;
	for (Skeleton* child : parent->_children) {
		offset = glm::vec3(child->_offX, child->_offY, child->_offZ);
		temp_offset = glm::length(offset);
		if (temp_offset < min_offset) {
			closestChild = child;
			min_offset = temp_offset;
			closestChildIndex = closestChild->_index;
		}
	}
	return closestChildIndex;
}
int Skinning::closestBoneIndex(int vertex_index) {
	int min_index = 0;
	float min_dist = glm::distance(_posBonesInit[0], _pointsInit[vertex_index]);
	for (int j = 1; j < _nbJoints; j++)
	{
		if (glm::distance(_posBonesInit[j], _pointsInit[vertex_index]) < min_dist)
		{
			min_index = j;
			min_dist = glm::distance(_posBonesInit[j], _pointsInit[vertex_index]);
		}
	}
	return min_index;
}
void Skinning::recomputeWeights() {
	if (_skin == NULL) return;
	if (_skel == NULL) return;

	// Compute weights :
	switch (_meth)
	{
	case 0:
		cout << "loading weights\n";
		loadWeights("data/skinning.txt");
		break;
	case 1:
		cout << "computing weights (skinning rigid)\n";
		computeRigidWeights();
		break;
	case 2:
		cout << "computing weights (cylindric distance)\n";
		computeCylindricWeights();
		break;
	default:
		break;
	}

	// Test skinning :
	animate();
}
void Skinning::computeRigidWeights() {
	if (_skin == NULL) return;
	if (_skel == NULL) return;

	_weights.clear();
	_weights.resize(_nbVtx);
	for (int i = 0; i < _nbVtx; i++)
		_weights[i].resize(_nbJoints);

	int jointIndex = 0;
	for (int i = 0; i < _nbVtx; i++)
	{
		jointIndex = closestBoneIndex(i);
		for (int j = 1; j < _nbJoints; j++)
			_weights[i][j] = 0;
		_weights[i][jointIndex] = 1;
	}
}

void Skinning::computeCylindricWeights() {
	if (_skin == NULL) return;
	if (_skel == NULL) return;

	_weights.resize(_nbVtx);
	for (int i = 0; i < _nbVtx; i++)
		_weights[i].resize(_nbJoints);

	double p, d1, d2, wParent, wChild;
	int jointIndex, childIndex;
	glm::vec3 xA, xB, xI, x, u1, u2, offset;
	Skeleton *child, * parent;
	for (int i = 0; i < _nbVtx; i++)
	{
		// get closest joint
		jointIndex = closestBoneIndex(i);
		parent = _joints[jointIndex];

		// initialisation
		for (int j = 1; j < _nbJoints; j++)
			_weights[i][j] = 0;


		if (!_joints[jointIndex]->_name.compare("Site")) {
			//cout << "is a site" << endl;
			_weights[i][jointIndex] = 1;
		} 
		else {
			// get nearest child
			childIndex = closetChildIndex(jointIndex);
			child = _joints[childIndex];
			
			x = glm::vec3(_pointsInit[i]);
			offset = glm::vec3(child->_offX, child->_offY, child->_offZ);
			if (glm::length(offset) == 0) {
				_weights[i][jointIndex] = 1;
			}
			else {
				xA = getPosition(jointIndex);
				xB = meanChildrenPos(jointIndex);
				u2 = xB - xA;
				u1 = x - xA;
				p = glm::dot(u1, u2) / pow(glm::length(u2), 2);
				if (p < 0)
					xI = xA;
				else if (p > 1)
					xI = xB;
				else
					xI = xA + float(p) * xB;
				
				d1 = glm::length(xA - xI);
				d2 = glm::length(xB - xI);

				wParent = d2 / (d1 + d2);
				wChild = d1 / (d1 + d2);

				_weights[i][jointIndex] = wParent;
				_weights[i][childIndex] = wChild;
				//vector<int> childrenIndex = getChildrenIndex(jointIndex);
				//for (int k = 0; k < childrenIndex.size(); k++)
				//	_weights[i][k] = wChild / childrenIndex.size();
			}
		}
	}
}
/*
void Skinning::computeCylindricWeights() {
	if (_skin == NULL) return;
	if (_skel == NULL) return;

	_weights.resize(_nbVtx);
	for (int i = 0; i < _nbVtx; i++)
		_weights[i].resize(_nbJoints);

	for (int i = 0; i < _nbVtx; i++)
	{
		//get the nearest joint
		int index1 = 0;
		float min_dist = glm::distance(_posBonesInit[0], _pointsInit[i]);
		for (int j = 1; j < _nbJoints; j++)
		{
			_weights[i][j] = 0;
			if ((*_joints[j])._children.size() > 0) {
				glm::vec3 boneLength = glm::vec3((*(*_joints[j])._children[0])._offX, (*(*_joints[j])._children[0])._offY, (*(*_joints[j])._children[0])._offZ);
				if (glm::length(boneLength) > 0.0001) {
					if (glm::distance(_posBonesInit[j], _pointsInit[i]) < min_dist)
					{
						index1 = j;
						min_dist = glm::distance(_posBonesInit[j], _pointsInit[i]);
					}
				}
			}
		}
		//get children of the joint
		glm::vec3 position1 = glm::vec3(_joints[index1]->_offX, _joints[index1]->_offY, _joints[index1]->_offZ);
		vector<Skeleton*> children_min = _joints[index1]->_children;
		//get their indices
		bool firstIteration = true;
		double tempdist;
		int index2;
		glm::vec3 u;
		glm::vec3 position2;
		glm::vec3 position2temp;
		glm::vec3 utemp;
		for (vector<Skeleton*>::iterator s = children_min.begin(); s != children_min.end(); ++s) {
			if (firstIteration) {
				index2 = (**s)._index;
				u = glm::vec3((**s)._offX, (**s)._offY, (**s)._offZ);
				position2 = position1 + u;
				min_dist = glm::distance(position2, glm::vec3(_pointsInit[i]));
				firstIteration = false;
			}
			else {
				utemp = glm::vec3((**s)._offX, (**s)._offY, (**s)._offZ);
				position2temp = position1 + utemp;
				tempdist = glm::distance(position2temp, glm::vec3(_pointsInit[i]));
				if (tempdist < min_dist) {
					min_dist = tempdist;
					index2 = (**s)._index;
					u = glm::vec3((**s)._offX, (**s)._offY, (**s)._offZ);
				}
			}
		}
		// calculer les poids de skinning
		float projPoint = -glm::dot(glm::vec3(_pointsInit[i]), u) / glm::length(u);
		float projFils = -glm::dot(position2, u) / glm::length(u);
		float projPere = -glm::dot(position1, u) / glm::length(u);
		float d1 = 1 / (projPoint - projFils);
		float d2 = 1 / (projPere - projPoint);
		float n = d1 + d2;
		_weights[i][index2] = d1 / n;
		_weights[i][index1] = d2 / n;
		if (_weights[i][index2] + _weights[i][index1] - 1 > 0.0001)
			cout << "probleme dans les coordonnees cyl " << _weights[i][index2] + _weights[i][index1] << endl;

	}
}
*/
void Skinning::loadWeights(std::string filename) {
	_weights.clear();
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
	int iV = 0;
	while (!feof(file)) {
		// for each line i.e. for each vertex :
		if (fgets(line, line_size, file)) {
			int iJt = 0;
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

glm::vec3 Skinning::getPosition(int index) {
	Skeleton* skel = _joints[index];
	glm::vec3 result(skel->_offX, skel->_offY, skel->_offZ);
	while (skel->_parent) {
		skel = skel->_parent;
		result += glm::vec3(skel->_offX, skel->_offY, skel->_offZ);
	}
	return result;
}


