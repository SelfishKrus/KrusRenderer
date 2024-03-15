#include "main.h"

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

    for (int x = x0; x <= x1; x++) 
    {
        float t = (x - x0) / (float)(x1 - x0);
        int y = y0 * (1. - t) + y1 * t;
        if (steep) 
        {
            image.set(y, x, color); // if transposed, de−transpose 
        }
        else 
        {
            image.set(x, y, color);
        }
    }
}

int main(int argc, char** argv) 
{
    // initialize the image 
    TGAImage image(100, 100, TGAImage::RGB);

    // draw 
    line(13, 20, 80, 40, image, white);
    line(80, 40, 13, 20, image, red);

    // write in 
    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("framebuffer.tga");
    return 0;
}