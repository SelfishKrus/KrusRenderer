#include "main.h"

#include <iostream>

// Bresenham’s line drawing algorithm
void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color) 
{
    bool steep = false;

    // if the line is steep, we transpose the image 
    if (std::abs(x0 - x1) < std::abs(y0 - y1)) 
    { 
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }

    // make it left−to−right 
    if (x0 > x1) 
    { 
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dx = x1 - x0;
    int dy = y1 - y0;

    // remap error by multiplying by 2 * dx 
    int derror2 = std::abs(dy) * 2;
    int error2 = 0;

    int y = y0;

    for (int x = x0; x <= x1; x++) 
    {
        if (steep) 
        {
            image.set(y, x, color); // if transposed, de−transpose 
        }
        else 
        {
            image.set(x, y, color);
        }

        error2 += derror2;
        if (error2 > dx)
        {
            y += (y1 > y0 ? 1 : -1);
            error2 -= 2 * dx ;
        }
    }
}

void DrawWireframe(Model* model, TGAImage& image, TGAColor color)
{
    for (int i = 0; i < model->nfaces(); i++)
    {   
        for (int j = 0; j < 3; j++)
        {
			Vec3f v0 = model->vert(i, j);
            Vec3f v1 = model->vert(i, (j + 1) % 3);
			int x0 = (v0.x + 1.) * width / 2.;
			int y0 = (v0.y + 1.) * height / 2.;
			int x1 = (v1.x + 1.) * width / 2.;
			int y1 = (v1.y + 1.) * height / 2.;
			line(x0, y0, x1, y1, image, white);
        }
    }
}

void DrawTriangleWireframe(Triangle triangle, TGAImage& image, TGAColor color)
{
    line(triangle.p0.x, triangle.p0.y, triangle.p1.x, triangle.p1.y, image, color);
    line(triangle.p1.x, triangle.p1.y, triangle.p2.x, triangle.p2.y, image, color);
    line(triangle.p2.x, triangle.p2.y, triangle.p0.x, triangle.p0.y, image, color);
}

// Line sweeping
void RasterizeTriangle_LS(Triangle triangle, TGAImage& image, TGAColor color)
{
    // sort the vertices p0, p1, p2 according to ascending y-coordinate
    if (triangle.p0.y > triangle.p1.y) std::swap(triangle.p0, triangle.p1);
    if (triangle.p0.y > triangle.p2.y) std::swap(triangle.p0, triangle.p2);
    if (triangle.p1.y > triangle.p2.y) std::swap(triangle.p1, triangle.p2);

    // Rasterize the triangle line by line 
    int totalHeight = triangle.p2.y - triangle.p0.y;

    // lower half part 
    for (int y = triangle.p0.y; y <= triangle.p1.y; y++)
    {   
        int segmentHeight = triangle.p1.y - triangle.p0.y;

        float alpha = (float)(y - triangle.p0.y) / totalHeight;
        float beta = (float)(y - triangle.p0.y) / segmentHeight;
        
        // start pt & end pt of the scanline
        Vec2i A = Vec2i(triangle.p0.x, triangle.p0.y) + Vec2i(triangle.p2.x - triangle.p0.x, triangle.p2.y - triangle.p0.y) * alpha;
        Vec2i B = Vec2i(triangle.p0.x, triangle.p0.y) + Vec2i(triangle.p1.x - triangle.p0.x, triangle.p1.y - triangle.p0.y) * beta;
        
        line(A.x, y, B.x, y, image, color);
    }

    // upper half part 
    for (int y = triangle.p1.y; y <= triangle.p2.y; y++)
    {
        int segmentHeight = triangle.p2.y - triangle.p1.y;

        float alpha = (float)(y - triangle.p0.y) / totalHeight;
        float beta = (float)(y - triangle.p1.y) / segmentHeight;

        Vec2i A = Vec2i(triangle.p0.x, triangle.p0.y) + Vec2i(triangle.p2.x - triangle.p0.x, triangle.p2.y - triangle.p0.y) * alpha;
        Vec2i B = Vec2i(triangle.p1.x, triangle.p1.y) + Vec2i(triangle.p2.x - triangle.p1.x, triangle.p2.y - triangle.p1.y) * beta;

        line(A.x, y, B.x, y, image, color);
    }
}

Vec3f Barycentric(Triangle triangle, Vec2i P)
{
    Vec3f u = Vec3f(triangle.p2.x-triangle.p0.x, triangle.p1.x-triangle.p0.x, triangle.p0.x-P.x) ^ Vec3f(triangle.p2.y - triangle.p0.y, triangle.p1.y - triangle.p0.y, triangle.p0.y - P.y);
    if (std::abs(u.z) < 1) return Vec3f(-1, 1, 1);
    return Vec3f(1.0f-(u.x+u.y)/u.z, u.x/u.z, u.y/u.z);
}

// Barycentric Coordinate
void RasterizeTriangle_BC(Triangle triangle, int* zBuffer, TGAImage& diffuseTex, TGAImage& image, TGAColor defaultCol)
{   
    // Get bounding box of the triangle
    Vec2i bboxMin = Vec2i(image.width() - 1, image.height() - 1);
    Vec2i bboxMax = Vec2i(0, 0);
    Vec2i clamp(image.width() - 1, image.height() - 1);

    Vec3i pos_faceVertices[3] = { triangle.p0, triangle.p1, triangle.p2 };
    Vec2f uv_faceVertices[3] = { triangle.p0_uv, triangle.p1_uv, triangle.p2_uv };
    for (int i = 0; i < 3; i++) 
    {
        bboxMin.x = std::max(0, std::min(bboxMin.x, pos_faceVertices[i].x));
        bboxMin.y = std::max(0, std::min(bboxMin.y, pos_faceVertices[i].y));
        bboxMax.x = std::min(clamp.x, std::max(bboxMax.x, pos_faceVertices[i].x));
        bboxMax.y = std::min(clamp.y, std::max(bboxMax.y, pos_faceVertices[i].y));
    }

    // Rasterize each pixel inside the bounding box 
    // inside the triangle - fill 
    // outside the triangle - blank
    Vec3i ptSS;
    for (ptSS.x = bboxMin.x; ptSS.x <= bboxMax.x; ptSS.x++)
    {
        for (ptSS.y = bboxMin.y; ptSS.y <= bboxMax.y; ptSS.y++)
        {
            Vec3f bc_screen = Barycentric(triangle, Vec2i(ptSS.x, ptSS.y));
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;
            
            // lerp z-value by barycentric coordinate
            ptSS.z = 0;
            Vec2f uv = {0.f, 0.f};
            for (int index = 0; index < 3; index++)
            {
                float bc_component;
                switch (index) 
                {
                    case 0: bc_component = bc_screen.x; break;
                    case 1: bc_component = bc_screen.y; break;
                    case 2: bc_component = bc_screen.z; break;
                }
                ptSS.z += pos_faceVertices[index].z * bc_component;
                uv = uv + uv_faceVertices[index] * bc_component;
			}
            // Z Test 
            if (zBuffer[ptSS.x + ptSS.y * image.width()] < ptSS.z)
            {
				zBuffer[ptSS.x + ptSS.y * image.width()] = ptSS.z;
                TGAColor col = diffuseTex.get(uv.x * diffuseTex.width(), uv.y * diffuseTex.height());
                //std::cout << uv.x << " " << uv.y << std::endl;
                image.set(ptSS.x, ptSS.y, defaultCol);
            }
        }
    }
}

void FlatShading(Model *model, int* zBuffer, TGAImage& diffuseTex, TGAImage& image, TGAColor color)
{   
    for (int i = 0; i < (model->nfaces()); i++)
    {   
        float width = image.width();
        float height = image.height();

        // get face-vertex coordinate 
        Vec3i posSS[3];
        Vec3f posWS[3];
        for (int j = 0; j < 3; j++)
        {
            Vec3f vertex = model->vert(i, j);
            posWS[j] = vertex;
            posSS[j] = Vec3i((vertex.x + 1.) * width / 2., (vertex.y + 1.) * height / 2., (vertex.z + 1.) * depth / 2.);
        }

        // transfer to triangle 
        Triangle tri;
        tri.p0 = posSS[0];
        tri.p1 = posSS[1];
        tri.p2 = posSS[2];



        // Blinn-Phong
        Vec3f lightDirWS = Vec3f(0, 0, -1);
        Vec3f normalWS = (posWS[2] - posWS[0]) ^ (posWS[1] - posWS[0]);
        normalWS.normalize();
        float NoL = normalWS * lightDirWS;
        float shadingCol = NoL * 255.f;

        if (NoL > 0)
        {
            RasterizeTriangle_BC(tri, zBuffer, diffuseTex, image, TGAColor(shadingCol, shadingCol, shadingCol, 255));
        }
    }
}

int main(int argc, char** argv) 
{
    // initialize the image 
    TGAImage image(width, height, TGAImage::RGB);
    // 2d z-buffer to 1d array
    int* zBuffer = new int[width * height];

    // read textures
    TGAImage diffuseTex = TGAImage();
    diffuseTex.read_tga_file("obj/african_head/african_head_diffuse.tga");
    std::cout << diffuseTex.width() << " " << diffuseTex.height() << std::endl;

    // draw 
    //DrawWireframe(model, image, white);
    //RasterizeTriangle_BC(triangle, image, red);
    //DrawTriangleWireframe(triangle, image, white);
    FlatShading(model, zBuffer, diffuseTex, image, red);

    // write in 
    image.write_tga_file("framebuffer.tga");
    return 0;
}