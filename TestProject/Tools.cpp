#include "Tools.h"


float Tools::rad2deg(float radians)
{
	return radians*180.0f / pi;
}


float Tools::deg2rad(float degrees)
{
	return degrees*pi / 180.0f;
}