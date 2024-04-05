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
TGAImage diffuseTex = TGAImage("obj/diablo3_pose/diablo3_pose_diffuse.tga");

struct Triangle
{
	Vec3i p0;
	Vec3i p1;
	Vec3i p2;
	Vec2f p0_uv;
	Vec2f p1_uv;
	Vec2f p2_uv;
	Vec3f p0_normalOS;
	Vec3f p1_normalOS;
	Vec3f p2_normalOS;
};

Triangle triangle
{
	Vec3i(45, 180, 0),
	Vec3i(220, 120, 0),
	Vec3i(130, 50, 0)
};
