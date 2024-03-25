#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);

const int width = 2048;
const int height = 2048;

Model *model = new Model("obj/diablo3_pose/diablo3_pose.obj");