#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);

const int width = 256;
const int height = 256;

Model *model = new Model("obj/diablo3_pose/diablo3_pose.obj");

struct Triangle
{
	Vec2i p0;
	Vec2i p1;
	Vec2i p2;
};

Triangle triangle
{
	Vec2i(45, 180),
	Vec2i(220, 120),
	Vec2i(130, 50)
};