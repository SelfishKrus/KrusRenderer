#include "main.h"
#include "geometry.h"

#include <iostream>

float dot(Vec3f a, Vec3f b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3f cross(Vec3f a, Vec3f b)
{
	return Vec3f(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

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
void RasterizeTriangle_BC(Triangle triangle, int* zBuffer, TGAImage& diffuseTex, TGAImage& image)
{   
    // Get bounding box of the triangle
    Vec2i bboxMin = Vec2i(image.width() - 1, image.height() - 1);
    Vec2i bboxMax = Vec2i(0, 0);
    Vec2i clamp(image.width() - 1, image.height() - 1);

    // pass vertex data 
    Vec3i pos_faceVertices[3] = { triangle.p0, triangle.p1, triangle.p2 };
    Vec2f uv_faceVertices[3] = { triangle.p0_uv, triangle.p1_uv, triangle.p2_uv };
    Vec3f normalOS_faceVertices[3] = { triangle.p0_normal, triangle.p1_normal, triangle.p2_normal };

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
            
            // lerp by barycentric coordinate
            ptSS.z = 0;
            Vec2f uv = {0.f, 0.f};
            Vec3f normalWS = {0.f, 0.f, 0.f};

            for (int index = 0; index < 3; index++)
            {
                float bc_component;
                switch (index) 
                {
                    case 0: bc_component = bc_screen.x; break;
                    case 1: bc_component = bc_screen.y; break;
                    case 2: bc_component = bc_screen.z; break;
                }
                ptSS.z = ptSS.z + pos_faceVertices[index].z * bc_component;
                uv = uv + uv_faceVertices[index] * bc_component;
                normalWS = normalWS + normalOS_faceVertices[index] * bc_component;

			}
            // Z Test 
            if (zBuffer[ptSS.x + ptSS.y * image.width()] < ptSS.z)
            {   
                // Z Write 
				zBuffer[ptSS.x + ptSS.y * image.width()] = ptSS.z;
                // Shading 
                TGAColor baseCol_TGA = diffuseTex.get(uv.x * diffuseTex.width(), uv.y * diffuseTex.height());
                Vec3f baseCol = baseCol_TGA.toVec3f();
                Vec3f lightDirWS = Vec3f(1.f, 1.f, 1.f);
                //Vec3f lightDirWS = Vec3f(0.f, 0.f, -1.f);
                lightDirWS.normalize();
                Vec3f lightCol = Vec3f(1.0f, 1.0f, 1.0f);
                float NoL01 = dot(normalWS, lightDirWS) * 0.5f + 0.5f;
                float zVal = ptSS.z / float(depth);

                Vec3f finalCol = baseCol * lightCol * NoL01;
                //Vec3f finalCol = zVal;
                //Vec3f finalCol = baseCol;
                image.set(ptSS.x, ptSS.y, TGAColor(finalCol.x * 255.f, finalCol.y * 255.f, finalCol.z * 255.f, 255.f));
            }
        }
    }
}

void FragShading(Model *model, Camera cam, int* zBuffer, TGAImage& diffuseTex, TGAImage& image, TGAColor color)
{   
    for (int faceIdx = 0; faceIdx < (model->nfaces()); faceIdx++)
    {   
        float width = image.width();
        float height = image.height();

        // get face-vertex coordinate 
        Vec3i posSS[3];
        Vec3f normal[3];

        // space transform 
        for (int j = 0; j < 3; j++)
        {   
            // OS to WS
            Vec3f posOS = model->vert(faceIdx, j);  // [-1, 1]
            Vec3f normalOS = model->normal(faceIdx, j);
            Vec3f posWS = posOS;
            Vec3f normalWS = normalOS;

            // WS to VS
            Vec3f translationFromCamToOrigion = cam.posWS * -1.0f; // translation
            Matrix matrix_view = Matrix::identity(3); // rotation
            matrix_view[0][0] = cam.rightDir.x; matrix_view[0][1] = cam.rightDir.y; matrix_view[0][2] = cam.rightDir.z;
            matrix_view[1][0] = cam.upDir.x; matrix_view[1][1] = cam.upDir.y; matrix_view[1][2] = cam.upDir.z;
            matrix_view[2][0] = cam.frontDir.x; matrix_view[2][1] = cam.frontDir.y; matrix_view[2][2] = cam.frontDir.z;
            
            Vec3f posVS = matrix_view * (posWS + translationFromCamToOrigion);

            // VS to HCS
            // perspective transformation 
            Matrix matrix_persToOrtho = Matrix::identity(4);
            matrix_persToOrtho[0][0] = cam.near;
            matrix_persToOrtho[1][1] = cam.near;
            matrix_persToOrtho[2][2] = cam.near + cam.far;
            matrix_persToOrtho[2][3] = -cam.near * cam.far;
            matrix_persToOrtho[3][2] = 1.0f;

            Matrix matrix_ortho_translate = Matrix::identity(4);
            //matrix_ortho_translate[0][3] = -(cam.right + cam.left) / 2.0f;
            //matrix_ortho_translate[1][3] = -(cam.top + cam.bottom) / 2.0f;
            //matrix_ortho_translate[2][3] = -(cam.far + cam.near) / 2.0f;
            Matrix matrix_ortho_scale = Matrix::identity(4);
            matrix_ortho_scale[0][0] = 2.0f / (cam.right - cam.left);
            matrix_ortho_scale[1][1] = 2.0f / (cam.top - cam.bottom);
            matrix_ortho_scale[2][2] = 2.0f / (cam.near - cam.far);

            Matrix matrix_ortho = matrix_ortho_scale * matrix_ortho_translate;

            Vec4f posHCS;
            if (cam.isOrtho == true)
            {
                posHCS = matrix_ortho * Vec4f(posVS.x, posVS.y, posVS.z, 1.0f);
            }
            else
            {
                posHCS = matrix_ortho * matrix_persToOrtho * Vec4f(posVS.x, posVS.y, posVS.z, 1.0f);
            }

            // HCS to NDC
            Vec4f posNDC = posHCS / posHCS.w;

            // NDC, Screen Space
            posSS[j] = Vec3i((posNDC.x + 1.) * width / 2., (posNDC.y + 1.) * height / 2., posNDC.z * depth);
        }

        // transfer to triangle 
        Triangle tri;
        tri.p0 = posSS[0];
        tri.p1 = posSS[2];
        tri.p2 = posSS[1];
        tri.p0_uv = model->uv(faceIdx, 0);
        tri.p1_uv = model->uv(faceIdx, 1);
        tri.p2_uv = model->uv(faceIdx, 2);
        tri.p0_normal = model->normal(faceIdx, 0);
        tri.p1_normal = model->normal(faceIdx, 1);
        tri.p2_normal = model->normal(faceIdx, 2);

        RasterizeTriangle_BC(tri, zBuffer, diffuseTex, image);
    }
}

int main(int argc, char** argv) 
{
    // initialize the image 
    TGAImage image(width, height, TGAImage::RGB);
    // 2d z-buffer to 1d array
    int* zBuffer = new int[width * height];

    // draw 
    //DrawWireframe(model, image, white);
    //RasterizeTriangle_BC(triangle, image, red);
    //DrawTriangleWireframe(triangle, image, white);
    Camera cam;
    cam.posWS = Vec3f(1.0f, 0.f, 1.0f);
    cam.lookAt = Vec3f(0.0f, 0.0f, 0.0f);
    cam.frontDir = cam.lookAt - cam.posWS;
    cam.frontDir.normalize();
    cam.rightDir = cross(Vec3f(0.f, 1.f, 0.f), cam.frontDir);
    cam.rightDir.normalize();
    cam.upDir = cross(cam.frontDir, cam.rightDir);
    cam.upDir.normalize();
    cam.far = 5.0f;
    cam.near = 1.5f;
    cam.left = -1.0f;
    cam.right = 1.0f;
    cam.top = 1.0f;
    cam.bottom = -1.0f;

    cam.isOrtho = false;

    FragShading(model, cam, zBuffer, diffuseTex, image, red);

    // write in 
    //image.flip_vertically();
    image.write_tga_file("framebuffer.tga");
    return 0;
}