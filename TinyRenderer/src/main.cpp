#include <vector>
#include <iostream>
#include <algorithm>

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

Model *model = NULL;
float *shadowbuffer = NULL;

const int width = 800;
const int height = 800;

Vec3f light_dir{ 0.5,1,1 };
Vec3f eye{ 1,1,3 };
Vec3f center{ 0,0,0 };
Vec3f up{ 0,1,0 };

struct DepthShader : public IShader {
	// triangle coordinates (clip coordinates), written by VS, read by FS
	mat<3, 3, float> varying_tri;

	DepthShader() : varying_tri() {}

	virtual Vec4f vertex(int iface, int nthvert) {
		// read the vertex from .obj file and transform it to screen coordinates
		Vec4f gl_Vertex = Viewport * Projection * ModelView * embed<4>(model->vert(iface, nthvert));
		varying_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));
		// return vertex coordinates
		return gl_Vertex;
	}

	virtual bool fragment(Vec3f bar, TGAColor &color) {
		Vec3f p = varying_tri * bar;
		color = TGAColor(255, 255, 255) * (p.z / depth);
		// no, we do not discard this pixel
		return false;
	}
};

int main(int argc, char** argv) {
	if (2 > argc) {
		std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1;
	}

	shadowbuffer = new float[width*height];
	for (int i = width*height; --i; ) {
		shadowbuffer[i] = -std::numeric_limits<float>::max();
	}

	model = new Model(argv[1]);
	light_dir.normalize();

	// rendering the shadow buffer
	{
		TGAImage depth(width, height, TGAImage::RGB);
		lookat(light_dir, center, up);
		viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
		projection(0);  // orthogonal projection

		DepthShader depthshader;
		Vec4f screen_coords[3];
		for (int i = 0; i < model->nfaces(); i++) {
			for (int j = 0; j < 3; j++) {
				screen_coords[j] = depthshader.vertex(i, j);
			}
			triangle(screen_coords, depthshader, depth, shadowbuffer);
		}
		depth.flip_vertically();
		depth.write_tga_file("depth.tga");
	}

	//float *zbuffer = new float[width*height];
	//for (int i = width * height; i--; zbuffer[i] = -std::numeric_limits<float>::max());

	//TGAImage frame(width, height, TGAImage::RGB);
	//lookat(eye, center, up);
	//viewport(width/8, height/8, width*3/4, height*3/4);
	//projection(-1.f / (eye - center).norm());
	//light_dir = proj<3>((Projection * ModelView * embed<4>(light_dir, 0.f))).normalize();

	//for (int m = 1; m < argc; m++) {
	//	model = new Model(argv[m]);
	//	Shader shader;
	//	// for each face
	//	for (int i = 0; i < model->nfaces(); i++) {
	//		// for each vertex of current face
	//		for (int j = 0; j < 3; j++) {
	//			shader.vertex(i, j);
	//		}
	//		triangle(shader.varying_tri, shader, frame, zbuffer);
	//	}
	//	delete model;
	//}

	//frame.flip_vertically();
	//frame.write_tga_file("framebuffer.tga");

	delete model;
	delete[] shadowbuffer;
	//delete [] zbuffer;
	return 0;
}