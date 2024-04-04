#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);

const int width = 1024;
const int height = 1024;
const int depth = 256;

Model *model = new Model("obj/diablo3_pose/diablo3_pose.obj");

struct Triangle
{
	Vec3i p0;
	Vec3i p1;
	Vec3i p2;
};

Triangle triangle
{
	Vec3i(45, 180, 0),
	Vec3i(220, 120, 0),
	Vec3i(130, 50, 0)
};