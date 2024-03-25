#include "main.h"

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
        std::vector <int> face = model->face(i);
        for (int j = 0; j < 3; j++)
        {
			Vec3f v0 = model->vert(face[j]);
			Vec3f v1 = model->vert(face[(j + 1) % 3]);
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

void RasterizeTriangle(Triangle triangle, TGAImage& image, TGAColor color)
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
        Vec2i A = triangle.p0 + (triangle.p2 - triangle.p0) * alpha;
        Vec2i B = triangle.p0 + (triangle.p1 - triangle.p0) * beta;
        
        line(A.x, y, B.x, y, image, color);
    }

    // upper half part 
    for (int y = triangle.p1.y; y <= triangle.p2.y; y++)
    {
        int segmentHeight = triangle.p2.y - triangle.p1.y;

        float alpha = (float)(y - triangle.p0.y) / totalHeight;
        float beta = (float)(y - triangle.p1.y) / segmentHeight;

        Vec2i A = triangle.p0 + (triangle.p2 - triangle.p0) * alpha;
        Vec2i B = triangle.p1 + (triangle.p2 - triangle.p1) * beta;

        line(A.x, y, B.x, y, image, color);
    }
}

int main(int argc, char** argv) 
{
    // initialize the image 
    TGAImage image(width, height, TGAImage::RGB);

    // draw 
    //DrawWireframe(model, image, white);
    RasterizeTriangle(triangle, image, red);
    DrawTriangleWireframe(triangle, image, white);

    // write in 
    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("framebuffer.tga");
    return 0;
}