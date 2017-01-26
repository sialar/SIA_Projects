#ifndef  HEADER_H
#define  HEADER_H

#include <vector>
#include <map>
#include <string>

enum  ChannelEnum
{
	X_ROTATION, Y_ROTATION, Z_ROTATION,
	X_POSITION, Y_POSITION, Z_POSITION
};

struct  Joint;

struct  Channel
{
	Joint *              joint;
	ChannelEnum          type;
	int                  index;
};

struct  Joint
{
	string               name;
	int                  index;
	Joint *              parent;
	vector< Joint * >    children;
	double               offset[3];
	bool                 has_site;
	double               site[3];
	vector< Channel * >  channels;
};

#endif
