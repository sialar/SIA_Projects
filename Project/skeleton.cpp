#include "skeleton.h"
#include <skeletonIO.h>
#include <qglviewer.h>

using namespace std;

/********************************   Test functions   ********************************/

void Skeleton::testSkeletonCreation(Skeleton* s)
{
	cout << "Testing the skeleton creation ...\n";
	cout << "Name = " << s->_name << endl;
	cout << "Offset = (" << s->_offX << ", " << s->_offY << ", " << s->_offZ << ")" << endl;
	cout << "The skeleton have (" << s->_dofs.size() << ") dofs:" << endl;
	for (AnimCurve dof : s->_dofs)
		cout << "\t- " << dof.name << " = " << dof._values.front() << ", ..., " << dof._values.back() << endl;
	cout << "The skeleton have (" << s->_children.size() << ") children:" << endl;
	for (Skeleton* sk : s->_children)
		cout << "\t- " << sk->_name << endl;
	if (s->_parent)
		cout << "The skeleton parent is " << s->_parent->_name << endl;
}
glm::mat3 Skeleton::testEulerToMatrixTransform(float rx, float ry, float rz)
{
	glm::mat3 res;
	glm::mat3 ea = toMat3(glm::eulerAngleYXZ(ry, rx, rz));
	cout << "\nTesting Euler-Matrix transformation with:" << endl;
	cout << "rx = " << rx << ", " << "ry = " << ry << ", " << "rz = " << rz << endl;
	displayMat3(ea);
	cout << "Using glm: " << endl;
	eulerToMatrix(rx, ry, rz, roYXZ, &res);
	displayMat3(res);
	return res;
}
qglviewer::Quaternion Skeleton::testMatrixToQuaternionTransform(glm::mat3 m)
{
	qglviewer::Quaternion q1, q3;
	cout << "\nTesting Matrix-Quat transformation:" << endl;
	matrixToQuaternion(m, &q1);
	cout << "Axis = (" << q1.axis().x << ", " << q1.axis().y << ", " << q1.axis().z << "). Angle = " << q1.angle() << endl;
	cout << "Using glm:\n";
	glm::quat q2 = glm::toQuat(m);
	cout << "Axis = (" << glm::axis(q2).x << ", " << glm::axis(q2).y << ", " << glm::axis(q2).z << "). Angle = " << glm::angle(q2) << endl;
	cout << "Using qglViewer:\n";
	float mat[3][3];
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			mat[i][j] = m[i][j];
	q3.setFromRotationMatrix(mat);
	cout << "Axis = (" << q3.axis().x << ", " << q3.axis().y << ", " << q3.axis().z << "). Angle = " << q3.angle() << endl;
	return q1;
}
qglviewer::Vec Skeleton::testQuaternionToAxisAngleTransform(qglviewer::Quaternion q)
{
	qglviewer::Vec v1, v2;
	cout << "\nTesting Quat-AxisAngle transformation:" << endl;
	quaternionToAxisAngle(q, &v1);
	cout << "Axis = (" << v1.x << ", " << v1.y << ", " << v1.z << endl;
	cout << "Using qglViewer:\n";
	v2 = q.axis();
	cout << "Axis = (" << v2.x << ", " << v2.y << ", " << v2.z << endl;
	return v1;
}
void Skeleton::testTransform()
{
	float rx, ry, rz;
	rx = M_PI / 2;
	ry = M_PI;
	rz = 0;

	glm::mat3 m = testEulerToMatrixTransform(rx, ry, rz);
	qglviewer::Quaternion q = testMatrixToQuaternionTransform(m);
	qglviewer::Vec v = testQuaternionToAxisAngleTransform(q);
}


/*******************************   Viewer methods   *********************************/
void drawBone(Skeleton *child)
{
	qglviewer::Vec v0(0, 0, 1);
	qglviewer::Vec v1(child->_offX, child->_offY, child->_offZ);
	qglviewer::Vec vRot = v0^v1; vRot.normalize();
	float angle = acosf((v0*v1) / (v0.norm()*v1.norm()))*180.0 / M_PI;
	float height = (v1 - v0).norm();
	float radius = 0.1f;
	glPushMatrix();
	{
		glRotatef(angle, vRot.x, vRot.y, vRot.z);
		gluCylinder(gluNewQuadric(), 0.1, 0.1, height, 5, 5);
	}
	glPopMatrix();
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
		glColor3f(1, 0, 0),
			gluSphere(gluNewQuadric(), 0.25, 10, 10);
		// Draw bone and children :
		glColor3f(0, 0, 1);
		for (unsigned int ichild = 0; ichild < _children.size(); ichild++) {
			drawBone(_children[ichild]);
			_children[ichild]->draw();
		}
	}
	glPopMatrix();
}
void Skeleton::rotateSkeleton() {
	switch (_rorder) {
	case roXYZ:
		glRotatef(_curRx, 1, 0, 0);
		glRotatef(_curRy, 0, 1, 0);
		glRotatef(_curRz, 0, 0, 1);
		break;
	case roYZX:
		glRotatef(_curRy, 0, 1, 0);
		glRotatef(_curRz, 0, 0, 1);
		glRotatef(_curRx, 1, 0, 0);
		break;
	case roZXY:
		glRotatef(_curRz, 0, 0, 1);
		glRotatef(_curRx, 1, 0, 0);
		glRotatef(_curRy, 0, 1, 0);
		break;
	case roXZY:
		glRotatef(_curRx, 1, 0, 0);
		glRotatef(_curRz, 0, 0, 1);
		glRotatef(_curRy, 0, 1, 0);
		break;
	case roYXZ:
		glRotatef(_curRy, 0, 1, 0);
		glRotatef(_curRx, 1, 0, 0);
		glRotatef(_curRz, 0, 0, 1);
		break;
	case roZYX:
		glRotatef(_curRz, 0, 0, 1);
		glRotatef(_curRy, 0, 1, 0);
		glRotatef(_curRx, 1, 0, 0);
		break;
	}
}
void Skeleton::animate(int iframe)
{
	// Update dofs :
	_curTx = 0; _curTy = 0; _curTz = 0;
	_curRx = 0; _curRy = 0; _curRz = 0;
	for (unsigned int idof = 0; idof < _dofs.size(); idof++) {
		if (!_dofs[idof].name.compare("Xposition")) _curTx = _dofs[idof]._values[iframe];
		if (!_dofs[idof].name.compare("Yposition")) _curTy = _dofs[idof]._values[iframe];
		if (!_dofs[idof].name.compare("Zposition")) _curTz = _dofs[idof]._values[iframe];
		if (!_dofs[idof].name.compare("Zrotation")) _curRz = _dofs[idof]._values[iframe];
		if (!_dofs[idof].name.compare("Yrotation")) _curRy = _dofs[idof]._values[iframe];
		if (!_dofs[idof].name.compare("Xrotation")) _curRx = _dofs[idof]._values[iframe];
	}
	// Animate children :
	for (unsigned int ichild = 0; ichild < _children.size(); ichild++) {
		_children[ichild]->animate(iframe);
	}
}

/****************************   Load from file (.bvh)   *****************************/

Skeleton* Skeleton::createFromFile(const string fileName) {
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
		cout << "file loaded" << endl;		
	}
	else {
		cerr << "Failed to load the file " << fileName.data() << endl;
		fflush(stdout);
	}
	return joints[0];
}

/***********************   Analysis of degrees of freedom   *************************/

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
	eulerToMatrix(M_PI*rx / 180.0, M_PI*ry / 180.0, M_PI*rz / 180.0, rorder, &R);
	// matrix -> quaternion :
	qglviewer::Quaternion q;
	matrixToQuaternion(R, &q);
	// quaternion -> axis/angle :
	quaternionToAxisAngle(q, vaa);
}
void Skeleton::nbDofs() {
	if (_dofs.empty()) return;

	double tol = 10e-4;
	int nbDofsR = -1;
	
	computeAxisAngles();
	/*
	glm::vec3 constRotations = rotationIsConstant(tol);
	nbDofsR = (int) glm::dot(constRotations, glm::vec3(1.0));
	*/
	nbDofsR = computeNbDofs(tol);
	cout << _name << " : " << nbDofsR << endl;// " degree(s) of freedom in rotation\n";

	// Propagate to children :
	for (unsigned int ichild = 0; ichild < _children.size(); ichild++) {
		_children[ichild]->nbDofs();
	}
}

/***********************   Intermediate Functions  **********************************/

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
			eulerAngles.push_back(glm::vec3(x, y, z));
		}
		eulerToAxisAngle(x, y, z, _rorder, &vaa);
		rotationAxis.push_back(glm::vec3(vaa[0], vaa[1], vaa[2]));
	}
}
glm::vec3 Skeleton::rotationIsConstant(double threshold)
{
	glm::vec3 boolRes(0, 0, 0);
	glm::vec3 dist = maxDistance(rotationAxis);
	for (int k = 0; k<3; k++)
		boolRes[k] = (dist[k] <= threshold) ? 1 : 0;
	return boolRes;
}
bool coplanaire(glm::vec3 lastNVaa, glm::vec3 curNVaa, glm::vec3 nextNVaa, double threshold) {
	return (glm::dot(lastNVaa, glm::cross(nextNVaa, curNVaa)) < 0.1);
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
	while ( i < rotationAxis.size() && crossProdMod < threshold)
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
	i = 1;
	glm::vec3 nextNVaa;
	curNVaa = glm::normalize(rotationAxis[0]);
	while (i < rotationAxis.size()-1)
	{
		lastNVaa = curNVaa;
		curNVaa = glm::normalize(rotationAxis[i]);
		nextNVaa = glm::normalize(rotationAxis[i+1]);
		// Si les axes de rotations ne restent pas dans un même plan alors nbDofs = 3 
		// A VERIFIER !!
		if (!coplanaire(lastNVaa, curNVaa, nextNVaa, threshold))
			return 3;
		i++;
	}
	return 2;
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
void Skeleton::displayMat3(glm::mat3 m)
{
	for (int i = 0; i<3; ++i)
	{
		for (int j = 0; j<3; ++j)
			cout << m[i][j] << " ";
		cout << "\n";
	}
}
glm::mat3 Skeleton::toMat3(glm::mat4 m)
{
	glm::mat3 m3;
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			m3[i][j] = m[i][j];
	return m3;
}
