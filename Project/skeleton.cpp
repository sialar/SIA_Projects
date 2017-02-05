#include "skeleton.h"
#include <skeletonIO.h>
#include <qglviewer.h>
#include <sstream>


using namespace std;

string tabulation(int level) {
	stringstream s;
	for (int i = 0; i < level; i++)
		s << "\t";
	return s.str();
}
void Skeleton::show(Skeleton* skel, int level) {
	cout << tabulation(level) << skel->_name << endl;
	cout << tabulation(level) << "_offset = (" << skel->_offX << ", " << skel->_offY << ", " << skel->_offZ << ")" << endl;
	cout << tabulation(level) << "_curT = (" << skel->_curTx << ", " << skel->_curTy << ", " << skel->_curTz << ")" << endl;
	cout << tabulation(level) << "_curR = (" << skel->_curRx << ", " << skel->_curRy << ", " << skel->_curRz << ")" << endl;
	for (Skeleton* s : skel->_children)
		show(s, level + 1);
}

void Skeleton::copy(Skeleton* s) {
	_name = s->_name;
	_offX = s->_offX;						
	_offY = s->_offY;						
	_offZ = s->_offZ;	
	_dofs = s->_dofs;	
	_curTx = s->_curTx;						
	_curTy = s->_curTy;					
	_curTz = s->_curTz;					
	_curRx = s->_curRx;					
	_curRy = s->_curRy;					
	_curRz = s->_curRz;					
	_rorder = s->_rorder;				
	_children.resize(s->_children.size());
	for (uint i = 0; i < s->_children.size(); i++) {
		_children[i] = new Skeleton();
		_children[i]->_parent = this;
		_children[i]->copy(s->_children[i]);
	}
	_index = s->_index;
}
Skeleton* Skeleton::createFromFile(const string fileName, bool debug) {
	bool                    is_load_success;
	string                  motion_name;
	vector<Skeleton*>       joints;
	map<string, Skeleton*>  joint_index;
	int                     num_frame;
	double                  interval;

	string					token;
	vector<Skeleton*>		joint_stack;
	Skeleton*				joint = NULL;
	Skeleton*				new_joint = NULL;

	if (debug)	
		cout << "Loading from " << fileName << endl;
	ifstream file(fileName.data());

	int iter = 0;
	if (file.good()) {
		while (!file.eof()) {
			iter++;
			file >> token;
			if (!token.compare("HIERARCHY")) {
				continue;
			}
			if (!token.compare("{")) {
				joint_stack.push_back(joint);
				joint = new_joint;
				continue;
			}
			if (!token.compare("}")) {
				joint = joint_stack.back();
				joint_stack.pop_back();
				continue;
			}
			if (!token.compare("ROOT") || !token.compare("JOINT") || !token.compare("End")) {
				new_joint = new Skeleton();
				new_joint->_index = joints.size();
				new_joint->_parent = joint;
				new_joint->_offX = 0.0;  new_joint->_offY = 0.0;  new_joint->_offZ = 0.0;
				joints.push_back(new_joint);
				if (joint)
					joint->_children.push_back(new_joint);

				file >> token;
				new_joint->_name = token;
				joint_index[new_joint->_name] = new_joint;
				continue;
			}
			if (!token.compare("OFFSET"))
			{
				file >> token;
				joint->_offX = stof(token);
				file >> token;
				joint->_offY = stof(token);
				file >> token;
				joint->_offZ = stof(token);
				continue;
			}
			if (!token.compare("CHANNELS"))
			{
				file >> token;
				int num_dofs = stoi(token);
				for (int i = 0; i < num_dofs; i++)
				{
					file >> token;
					AnimCurve dof;
					dof.name = token;
					joint->_dofs.push_back(dof);
				}
			}
			if (!token.compare("MOTION"))
				break;
		}
		file >> token;
		if (token.compare("Frames:"))  file.close();
		file >> token;
		num_frame = stoi(token);
		file >> token;
		if (token.compare("Frame"))  file.close();
		file >> token;
		if (token.compare("Time:"))  file.close();
		file >> token;
		interval = stof(token);

		int k = 0;
		for (int f = 0; f < num_frame; ++f)
		{
			for (Skeleton* s : joints)
			{
				for (uint i = 0; i<s->_dofs.size(); ++i)
				{
					file >> token;
					s->_dofs[i]._values.push_back(stof(token));
					k++;
				}
			}
		}

		file.close();
		is_load_success = true;
		if (debug)
			cout << "file loaded" << endl;
	}
	else {
		if (debug)
			cerr << "Failed to load the file " << fileName.data() << endl;
		fflush(stdout);
	}
	return joints[0];
}

void drawBone(Skeleton *child) 
{
	qglviewer::Vec v0(0,0,1);
	qglviewer::Vec v1(child->_offX, child->_offY, child->_offZ);
	qglviewer::Vec vRot = v0^v1; vRot.normalize();
	float angle = acosf((v0*v1)/(v0.norm()*v1.norm()))*180.0/M_PI;
	float height = (v1-v0).norm();
	float radius = 0.1f;
	glPushMatrix();
	{
		glRotatef(angle, vRot.x, vRot.y, vRot.z);
		gluCylinder(gluNewQuadric(), 0.1, 0.1, height, 5, 5);
	}
	glPopMatrix();
	
}

void Skeleton::rotateSkeleton() {
	switch (_rorder) {
		case roXYZ :
			glRotatef(_curRx, 1, 0, 0);
			glRotatef(_curRy, 0, 1, 0);
			glRotatef(_curRz, 0, 0, 1);
			break;
		case roYZX :
			glRotatef(_curRy, 0, 1, 0);
			glRotatef(_curRz, 0, 0, 1);
			glRotatef(_curRx, 1, 0, 0);
			break;
		case roZXY :
			glRotatef(_curRz, 0, 0, 1);
			glRotatef(_curRx, 1, 0, 0);
			glRotatef(_curRy, 0, 1, 0);
			break;
		case roXZY :
			glRotatef(_curRx, 1, 0, 0);
			glRotatef(_curRz, 0, 0, 1);
			glRotatef(_curRy, 0, 1, 0);
			break;
		case roYXZ :
			glRotatef(_curRy, 0, 1, 0);
			glRotatef(_curRx, 1, 0, 0);
			glRotatef(_curRz, 0, 0, 1);
			break;
		case roZYX :
			glRotatef(_curRz, 0, 0, 1);
			glRotatef(_curRy, 0, 1, 0);
			glRotatef(_curRx, 1, 0, 0);
			break;
	}
}
void Skeleton::draw() 
{
	glPushMatrix();
	{
		// Set good reference frame :
		glTranslatef(_offX, _offY, _offZ);
		// Use current value of dofs :
		glTranslatef(_curTx, _curTy, _curTz);
		rotateSkeleton();
		// Draw articulation :
		glColor3f(1,0,0),
		gluSphere(gluNewQuadric(), 0.25, 10, 10);
		// Draw bone and children :
		glColor3f(0,0,1);
		for (unsigned int ichild = 0 ; ichild < _children.size() ; ichild++) {
			drawBone(_children[ichild]);
			_children[ichild]->draw();
		}
	}
	glPopMatrix();
}
void Skeleton::init()
{
	// Update dofs :
	_curTx = 0; _curTy = 0; _curTz = 0;
	_curRx = 0; _curRy = 0; _curRz = 0;
	// Animate children :
	for (unsigned int ichild = 0; ichild < _children.size(); ichild++) {
		_children[ichild]->init();
	}
}
void Skeleton::animate(int iframe) 
{
	// Update dofs :
	_curTx = 0; _curTy = 0; _curTz = 0;
	_curRx = 0; _curRy = 0; _curRz = 0;
	for (unsigned int idof = 0 ; idof < _dofs.size() ; idof++) {
		if(!_dofs[idof].name.compare("Xposition")) _curTx = _dofs[idof]._values[iframe];
		if(!_dofs[idof].name.compare("Yposition")) _curTy = _dofs[idof]._values[iframe];
		if(!_dofs[idof].name.compare("Zposition")) _curTz = _dofs[idof]._values[iframe];
		if(!_dofs[idof].name.compare("Zrotation")) _curRz = _dofs[idof]._values[iframe];
		if(!_dofs[idof].name.compare("Yrotation")) _curRy = _dofs[idof]._values[iframe];
		if(!_dofs[idof].name.compare("Xrotation")) _curRx = _dofs[idof]._values[iframe];
	}	
	// Animate children :
	for (unsigned int ichild = 0 ; ichild < _children.size() ; ichild++) {
		_children[ichild]->animate(iframe);
	}
}

glm::mat3 toMat3(glm::mat4 m)
{
	glm::mat3 m3;
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			m3[i][j] = m[i][j];
	return m3;
}

void Skeleton::eulerToMatrix(double rx, double ry, double rz, int rorder, glm::mat3 *R)
{
	glm::mat3 mx, my, mz;

	mx = toMat3(glm::rotate(glm::mat4(1), (float)(rx * 180 / M_PI), glm::vec3(1, 0, 0)));
	my = toMat3(glm::rotate(glm::mat4(1), (float)(ry * 180 / M_PI), glm::vec3(0, 1, 0)));
	mz = toMat3(glm::rotate(glm::mat4(1), (float)(rz * 180 / M_PI), glm::vec3(0, 0, 1)));
	// Or
	/*
	double ch = cos(rx);
	double sh = sin(rx);
	mx = glm::mat3(1,  0,   0,
	0, ch, -sh,
	0, sh, ch );
	double ca = cos(ry);
	double sa = sin(ry);
	my = glm::mat3( ca, 0, -sa,
	0, 1,   0,
	sa, 0, ca );

	double cb = cos(rz);
	double sb = sin(rz);
	mz = glm::mat3( cb, -sb,  0,
	sb,  cb,  0,
	0,   0, 1 );
	*/

	switch (rorder) {
	case roXYZ: *R = mx * my * mz; break;
	case roYZX: *R = my * mz * mx; break;
	case roZXY: *R = mz * mx * my; break;
	case roXZY: *R = mx * mz * my; break;
	case roYXZ: *R = my * mx * mz; break;
	case roZYX: *R = mz * my * mx; break;
	default: break;
	}
}

void Skeleton::transfoToQuaternion(glm::mat3 R, glm::vec3 t, float dq[2][4]) {
	qglviewer::Quaternion *q;
	matrixToQuaternion(R, q);
	// non-dual part (just copy q):
	for (int i = 0; i<4; i++) dq[0][i] = (*q)[i];
	// dual part:
	dq[1][0] = -0.5*(t[0] * (*q)[1] + t[1] * (*q)[2] + t[2] * (*q)[3]);
	dq[1][1] = 0.5*(t[0] * (*q)[0] + t[1] * (*q)[3] - t[2] * (*q)[2]);
	dq[1][2] = 0.5*(-t[0] * (*q)[3] + t[1] * (*q)[0] + t[2] * (*q)[1]);
	dq[1][3] = 0.5*(t[0] * (*q)[2] - t[1] * (*q)[1] + t[2] * (*q)[0]);
}

glm::vec3 Skeleton::applyQuat(glm::vec3 pos, float dq[2][4]) {
	//conjugate
	float dqc[2][4];
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 4; j++)
			dqc[i][j] = dq[i][j];
	Skeleton::conjugateDualQuat(dqc);
	//position quaternion
	float dqp[2][4] = {1, 0, 0, 0, 0, pos[0], pos[1], pos[2]};
	//calculus
	float res1[2][4];
	float res2[2][4];
	Skeleton::multiplyDualQuat(dq, dqp, res1);
	Skeleton::multiplyDualQuat(res1, dqc, res2);
	glm::vec3 newPos(res2[1][1], res2[1][2], res2[1][3]);
	return newPos;
}

void Skeleton::multiplyDualQuat(float dq1[2][4], float dq2[2][4], float res[2][4]) {
	qglviewer::Quaternion q1, q2, qr;
	//partie non duale
	q1 = qglviewer::Quaternion(dq1[0][0], dq1[0][1], dq1[0][2], dq1[0][3]);
	q2 = qglviewer::Quaternion(dq2[0][0], dq2[0][1], dq2[0][2], dq2[0][3]);
	qr = q1 * q2;
	for (int i = 0; 1 < 4; i++) {
		res[0][i] = qr[i];
	}
	//partie duale
	q1 = qglviewer::Quaternion(dq1[0][0], dq1[0][1], dq1[0][2], dq1[0][3]);
	q2 = qglviewer::Quaternion(dq2[1][0], dq2[1][1], dq2[1][2], dq2[1][3]);
	qr = q1 * q2;
	for (int i = 0; 1 < 4; i++) {
		res[1][i] = qr[i];
	}
	q1 = qglviewer::Quaternion(dq1[1][0], dq1[1][1], dq1[1][2], dq1[1][3]);
	q2 = qglviewer::Quaternion(dq2[0][0], dq2[0][1], dq2[0][2], dq2[0][3]);
	qr = q1 * q2;
	for (int i = 0; 1 < 4; i++) {
		res[0][i] += qr[i];
	}
}

void Skeleton::conjugateDualQuat(float dq[2][4]){
	dq[0][1] = -dq[0][1];
	dq[0][2] = -dq[0][2];
	dq[0][3] = -dq[0][3];
	dq[1][1] = -dq[1][1];
}

void Skeleton::matrixToQuaternion(glm::mat3 R, qglviewer::Quaternion *q)
{
	// Output quaternion
	float w, x, y, z;
	// Determine which of w,x,y, or z has the largest absolute value
	float fourWSquaredMinus1 = R[0][0] + R[1][1] + R[2][2];
	float fourXSquaredMinus1 = R[0][0] - R[1][1] - R[2][2];
	float fourYSquaredMinus1 = R[1][1] - R[0][0] - R[2][2];
	float fourZSquaredMinus1 = R[2][2] - R[0][0] - R[1][1];

	int biggestIndex = 0;
	float fourBiggestSquaredMinus1 = fourWSquaredMinus1;

	if (fourXSquaredMinus1 > fourBiggestSquaredMinus1) {
		fourBiggestSquaredMinus1 = fourXSquaredMinus1;
		biggestIndex = 1;
	}
	if (fourYSquaredMinus1 > fourBiggestSquaredMinus1) {
		fourBiggestSquaredMinus1 = fourYSquaredMinus1;
		biggestIndex = 2;
	}
	if (fourZSquaredMinus1 > fourBiggestSquaredMinus1) {
		fourBiggestSquaredMinus1 = fourZSquaredMinus1;
		biggestIndex = 3;
	}
	// Per form square root and division
	float biggestVal = sqrt(fourBiggestSquaredMinus1 + 1.0f) * 0.5f;
	float mult = 0.25f / biggestVal;

	// Apply table to compute quaternion values
	switch (biggestIndex) {
	case 0:
		w = biggestVal;
		x = (R[1][2] - R[2][1]) * mult;
		y = (R[2][0] - R[0][2]) * mult;
		z = (R[0][1] - R[1][0]) * mult;
		break;
	case 1:
		x = biggestVal;
		w = (R[1][2] - R[2][1]) * mult;
		y = (R[0][1] + R[1][0]) * mult;
		z = (R[2][0] + R[0][2]) * mult;
		break;
	case 2:
		y = biggestVal;
		w = (R[2][0] - R[0][2]) * mult;
		x = (R[0][1] + R[1][0]) * mult;
		z = (R[1][2] + R[2][1]) * mult;
		break;
	case 3:
		z = biggestVal;
		w = (R[0][1] - R[1][0]) * mult;
		x = (R[2][0] + R[0][2]) * mult;
		y = (R[1][2] + R[2][1]) * mult;
		break;
	}
	*q = qglviewer::Quaternion(x, y, z, w);

}
void Skeleton::quaternionToAxisAngle(qglviewer::Quaternion q, qglviewer::Vec *vaa)
{
	if (q[3] > 1) q.normalize();
	double angle = 2 * acos(q[3]);
	double s = sqrt(1 - q[3] * q[3]);
	if (s >= 0.001) {
		q[0] /= s;
		q[1] /= s;
		q[2] /= s;
	}
	*vaa = qglviewer::Vec(q[0], q[1], q[2]);
}
void Skeleton::eulerToAxisAngle(double rx, double ry, double rz, int rorder, qglviewer::Vec *vaa)
{
	// Euler -> matrix :
	glm::mat3 R;
	// Using our functions
	eulerToMatrix(M_PI*rx / 180.0, M_PI*ry / 180.0, M_PI*rz / 180.0, rorder, &R);
	// matrix -> quaternion :
	qglviewer::Quaternion q;
	matrixToQuaternion(R, &q);
	// quaternion -> axis/angle :
	quaternionToAxisAngle(q, vaa);
	/*
	// Using qglviewer and glm functions
	R = toMat3(glm::eulerAngleYXZ((float)ry, (float)rx, (float)rz));
	
	glm::quat glmQuat = glm::toQuat(R);
	qglviewer::Quaternion qglviewerQuat;
	double mat[3][3];
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			mat[i][j] = R[i][j];
	qglviewerQuat.setFromRotationMatrix(mat);

	*vaa = qglviewerQuat.axis();*/	
}

void Skeleton::nbDofs() {
	if (_dofs.empty()) return;

	double tol = 0.01;
	int nbDofsR = -1;

	computeAxisAngles();

	nbDofsR = computeNbDofs(tol);
	//cout << _name << " : " << nbDofsR << endl;// " degree(s) of freedom in rotation\n";
	// Propagate to children :
	for (unsigned int ichild = 0; ichild < _children.size(); ichild++) {
		_children[ichild]->nbDofs();
	}
}
int Skeleton::computeNbDofs(double threshold)
{
	glm::vec3 eulerMaxDist = maxDistance(eulerAngles);
	// Si les angles d'euler sont constants au cours du temps alors nbDofs = 0
	// Exemples: hip, rhipjoint, lhipjoint
	if (glm::length(eulerMaxDist) < threshold)
		return 0;

	double crossProdMod = 0;
	glm::vec3 lastNVaa;
	glm::vec3 curNVaa = glm::normalize(rotationAxis[0]);
	uint i = 1;
	while (i < rotationAxis.size() && crossProdMod < threshold)
	{
		lastNVaa = curNVaa;
		curNVaa = glm::normalize(rotationAxis[i]);
		crossProdMod = glm::length(glm::cross(curNVaa, lastNVaa));
		i++;
	}

	// Si le produit vectoriel est résté assez faible pour pouvoir considerer les axes de rotaion
	// colinéaires ( ~ l'axe de rotaion est résté à peu près constant au cours du temps)
	if (i == rotationAxis.size())
		return 1;

	// Sinon nbDofs = 2 ou 3
	/*i = 1;
	glm::vec3 nextNVaa;
	curNVaa = glm::normalize(rotationAxis[0]);
	while (i < rotationAxis.size() - 1)
	{
		lastNVaa = curNVaa;
		curNVaa = glm::normalize(rotationAxis[i]);
		nextNVaa = glm::normalize(rotationAxis[i + 1]);
		// Si les axes de rotations ne restent pas dans un même plan alors nbDofs = 3 
		// A VERIFIER !!
		if (glm::dot(lastNVaa, glm::cross(nextNVaa, curNVaa)) > threshold)
			return 3;
		i++;
	}*/
	return 2;
}
void Skeleton::computeAxisAngles()
{
	double x = 0, y = 0, z = 0;
	qglviewer::Vec vaa;
	for (uint i = 0; i < _dofs[0]._values.size(); i++)
	{
		for (uint j = 0; j < _dofs.size(); j++)
		{
			if (!_dofs[j].name.compare("Xrotation"))
				x = _dofs[j]._values[i];
			if (!_dofs[j].name.compare("Yrotation"))
				y = _dofs[j]._values[i];
			if (!_dofs[j].name.compare("Zrotation"))
				z = _dofs[j]._values[i];
		}
		eulerAngles.push_back(glm::vec3(x, y, z));
		eulerToAxisAngle(x, y, z, _rorder, &vaa);
		rotationAxis.push_back(glm::vec3(vaa[0], vaa[1], vaa[2]));
	}
}
glm::vec3 Skeleton::maxDistance(std::vector<glm::vec3>& vector)
{
	std::vector<float> xVector, yVector, zVector;
	glm::vec3 dist;
	xVector.resize(vector.size());
	yVector.resize(vector.size());
	zVector.resize(vector.size());
	for (uint i = 0; i < vector.size(); i++)
	{
		xVector[i] = vector[i].x;
		yVector[i] = vector[i].y;
		zVector[i] = vector[i].z;
	}
	dist.x = *max_element(xVector.begin(), xVector.end()) - *min_element(xVector.begin(), xVector.end());
	dist.y = *max_element(yVector.begin(), yVector.end()) - *min_element(yVector.begin(), yVector.end());
	dist.z = *max_element(zVector.begin(), zVector.end()) - *min_element(zVector.begin(), zVector.end());
	return dist;
}

void Skeleton::resizeDofs(int size) {
	for (uint i = 0; i < _dofs.size(); i++) {
		_dofs[i]._values.clear();
		_dofs[i]._values.resize(size);
	}
	for (unsigned int ichild = 0; ichild < _children.size(); ichild++) {
		_children[ichild]->resizeDofs(size);
	}
}
	
void getJoints(Skeleton* skel, vector<Skeleton*>* joints) {
	joints->push_back(skel);
	for (unsigned int ichild = 0; ichild < skel->_children.size(); ichild++) {
		getJoints(skel->_children[ichild], joints);
	}
}

double compareDofs(Skeleton* s1, Skeleton* s2, int frame1, int frame2, double tol) {
	double mean = 0;
	for (uint i = 0; i < s1->_dofs.size(); i++) 
		mean += std::abs(s1->_dofs[i]._values[frame1] - s2->_dofs[i]._values[frame2]);
	
	mean /= s1->_dofs.size();
	return mean ;
}

void findAllignNumberI(vector<Skeleton*> joints1, vector<Skeleton*> joints2, int k, int* i1, int* i2, double tol) {
	int target = 0, n = 0, nbSite = joints1.size();
	double mean = 0;
	int nbFrames = std::min(joints1[0]->_dofs[0]._values.size(), joints2[0]->_dofs[0]._values.size());
	for (int t1=0; t1<nbFrames;t1++) {
		for (int t2 = 0; t2 < nbFrames; t2++) {
			n = 0;
			mean = 0;
			for (uint iskel = 0; iskel < joints1.size(); iskel++) {
				if (joints1[iskel]->_dofs.size()>0 && joints1[iskel]->_dofs.size()>0) {
					nbSite--;
					mean += compareDofs(joints1[iskel], joints2[iskel], t1, t2, tol);
				}
			}
			mean /= joints1.size() - nbSite;
			if (mean < tol) {
				*i1 = t1;
				*i2 = t2;
				target++;
				if (target == k)
					return;
			}
		}
	}
}

glm::vec3 Skeleton::quaternionToEulerAngle(const glm::quat& q)
{
	float pitch = glm::pitch(q);
	float yaw = glm::yaw(q);
	float roll = glm::roll(q);
	glm::vec3 eulerAngle = glm::vec3(pitch, yaw, roll);
	eulerAngle /= M_PI / 180.0;
	return eulerAngle;
}

glm::quat Skeleton::eulerToQuaternionOfDofs(int frame) {
	glm::vec3 eulerAngle = eulerAngles[frame];
	glm::mat3 R;
	//cout << eulerAngles[0].x << " " << eulerAngles[0].y << " " << eulerAngles[0].z << endl;
	eulerAngle *= M_PI / 180.0;
	return glm::quat(glm::vec3(eulerAngle.x, eulerAngle.y, eulerAngle.z));
}
void Skeleton::getEulerAnglesFromQuat(const glm::quat& q, int frame, Skeleton* s1, Skeleton* s2) {
	glm::vec3 euler = quaternionToEulerAngle(q);
	for (uint i = 0; i < _dofs.size(); i++) {
		//_dofs[i]._values[frame] = 0.5*s2->_dofs[i]._values[frame] + (1 - 0.5) * s1->_dofs[i]._values[frame];
		if (!_dofs[i].name.compare("Xrotation"))
			_dofs[i]._values[frame] = euler.x;
		if (!_dofs[i].name.compare("Yrotation"))
			_dofs[i]._values[frame] = euler.y;
		if (!_dofs[i].name.compare("Zrotation"))
			_dofs[i]._values[frame] = euler.z;
	}
}

void fill(Skeleton* s, Skeleton* s1, Skeleton* s2, int i1, int i2, float coef, int version) {
	int min;
	glm::quat quat1, quat2, quatRes;
	glm::vec3 euler;

	switch (version) {
		
		/************************ Version 0 ***************************/
		case 0:
			for (uint i = 0; i < s2->_dofs.size(); i++) {
				min = std::min(s2->_dofs[i]._values.size(), s1->_dofs[i]._values.size());
				for (int j = 0; j < min; j++)
					s->_dofs[i]._values[j] = (1-coef) *s2->_dofs[i]._values[j] + (coef) * s1->_dofs[i]._values[j];
			}
			for (unsigned int ichild = 0; ichild < s1->_children.size(); ichild++)
				fill(s->_children[ichild], s1->_children[ichild], s2->_children[ichild], i1, i2, coef, version);
			break;
		
		/************************ Version 1 ***************************/
		case 1:
			for (uint i = 0; i < s2->_dofs.size(); i++) {
				min = std::min(s2->_dofs[i]._values.size(), s1->_dofs[i]._values.size());
				for (int j = 0; j < i2; j++)
					s->_dofs[i]._values[j] = s1->_dofs[i]._values[j];
				for (int j = i2; j < min; j++)
					s->_dofs[i]._values[j] = (1 - coef) *s2->_dofs[i]._values[j + i1 - i2] + coef * s1->_dofs[i]._values[j];
			}
			for (unsigned int ichild = 0; ichild < s1->_children.size(); ichild++)
				fill(s->_children[ichild], s1->_children[ichild], s2->_children[ichild], i1, i2, coef, version);
			break;
		
		/************************ Version 2 ***************************/
		case 2:
			if (s1->_dofs.size() > 0 && s2->_dofs.size() > 0) {
				min = std::min(s2->_dofs[0]._values.size(), s1->_dofs[0]._values.size());
				for (uint i = 0; i < s->_dofs.size(); i++) {
					if (!s->_dofs[i].name.compare("Xposition") || !s->_dofs[i].name.compare("Yposition")
						|| !s->_dofs[i].name.compare("Zposition")) {
						for (int j = 0; j < min; j++)
							s->_dofs[i]._values[j] = s1->_dofs[i]._values[j];// +(1 - coef) * s1->_dofs[i]._values[j];
					}
				}
				s1->computeAxisAngles();
				s2->computeAxisAngles();
				for (int j = 0; j < min; j++) {
					quat1 = s1->eulerToQuaternionOfDofs(j);
					quat2 = s2->eulerToQuaternionOfDofs(j);
					quatRes = glm::mix(quat1, quat2, 1-coef);
					s->getEulerAnglesFromQuat(quatRes,j,s1,s2);
					if (j < 0) {
						cout << quat1[0] << " " << quat1[1] << " " << quat1[2] << " " << quat1[3] << endl;
						cout << quat2[0] << " " << quat2[1] << " " << quat2[2] << " " << quat2[3] << endl;
						cout << quatRes[0] << " " << quatRes[1] << " " << quatRes[2] << " " << quatRes[3] << endl;

						cout << s1->_dofs[2]._values[j] << " " << s1->_dofs[1]._values[j] << " " << s1->_dofs[0]._values[j] << endl;
						cout << s2->_dofs[2]._values[j] << " " << s2->_dofs[1]._values[j] << " " << s2->_dofs[0]._values[j] << endl;
						cout << s->_dofs[2]._values[j] << " " << s->_dofs[1]._values[j] << " " << s->_dofs[0]._values[j] << endl << endl;
					}
				}

				for (unsigned int ichild = 0; ichild < s1->_children.size(); ichild++)
					fill(s->_children[ichild], s1->_children[ichild], s2->_children[ichild], i1, i2, coef, version);
			}
			
	break;
	}

}

Skeleton* Skeleton::createNewAnimationVersion0() {
	float coef = -1;
	while (coef > 1 || coef < 0) {
		cout << "Entrer un coeficient entre 0 et 1.\n\t- 1 : walk\n\t- 0 : run" << endl;
		cin >> coef;
	}

	Skeleton* root1 = createFromFile("data/walk.bvh", false);
	Skeleton* root2 = createFromFile("data/run.bvh", false);

	int minDofsSize = min(root1->_dofs[0]._values.size(), root2->_dofs[0]._values.size());
	
	vector<Skeleton*> joints1, joints2;
	getJoints(root1, &joints1);
	getJoints(root2, &joints2);
	
	Skeleton* newRoot = new Skeleton();
	newRoot->copy(root1);
	newRoot->resizeDofs(minDofsSize);

	fill(newRoot, root1, root2, 0, 0, coef,0);
	return newRoot;
}

Skeleton* Skeleton::createNewAnimationVersion1() {
	Skeleton* root1 = createFromFile("data/walk.bvh", false);
	Skeleton* root2 = createFromFile("data/run.bvh", false);

	float coef = -1;
	while (coef > 1 || coef < 0) {
		cout << "Entrer un coeficient entre 0 et 1.\n\t- 1 : walk\n\t- 0 : run" << endl;
		cin >> coef;
	}
	int minDofsSize = min(root1->_dofs[0]._values.size(), root2->_dofs[0]._values.size());

	vector<Skeleton*> joints1, joints2;
	getJoints(root1, &joints1);
	getJoints(root2, &joints2);

	Skeleton* newRoot = new Skeleton();
	newRoot->copy(root1);
	newRoot->resizeDofs(minDofsSize);

	int i1 = -1, i2 = -1;
	findAllignNumberI(joints1, joints2, 100, &i1, &i2, 10e-4);

	fill(newRoot, root1, root2, i1, i2, coef, 1);
	return newRoot;
}

Skeleton* Skeleton::createNewAnimationVersion2() {
	Skeleton* root1 = createFromFile("data/walk.bvh", false);
	Skeleton* root2 = createFromFile("data/run.bvh", false);

	float coef = -1;
	while (coef > 1 || coef < 0) {
		cout << "Entrer un coeficient entre 0 et 1.\n\t- 1 : walk\n\t- 0 : run" << endl;
		cin >> coef;
	}
	int minDofsSize = min(root1->_dofs[0]._values.size(), root2->_dofs[0]._values.size());
	vector<Skeleton*> joints1, joints2;
	getJoints(root1, &joints1);


	getJoints(root2, &joints2);
	Skeleton* newRoot = new Skeleton();
	newRoot->copy(root1);
	newRoot->resizeDofs(minDofsSize);
	/*
	glm::vec3 eulerAngle(0.4f, 0.5f, 0.3f);
	float pitch = glm::pitch(glm::quat(glm::vec3(eulerAngle.x, eulerAngle.y, eulerAngle.z)));
	float yaw = glm::yaw(glm::quat(glm::vec3(eulerAngle.x, eulerAngle.y, eulerAngle.z)));
	float roll = glm::roll(glm::quat(glm::vec3(eulerAngle.x, eulerAngle.y, eulerAngle.z)));

	glm::vec3 v(pitch,yaw,roll);

	cout << eulerAngle.x << " " << eulerAngle.y << " " << eulerAngle.z << endl;
	cout << v.x << " " << v.y << " " << v.z << endl;
	*/
	fill(newRoot, root1, root2, 0, 0, coef, 2);
	return newRoot;
}
