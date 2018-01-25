#include <vector>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <limits>

#define _USE_MATH_DEFINES
#include "math.h"

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

Model *model = NULL;
float *shadowbuffer = NULL;

const int width = 800;
const int height = 800;

//Vec3f light_dir{ 1.f,1.f,1.f };
Vec3f eye{ 1.2f, -.8f, 3.f };
Vec3f center{ 0,0,0 };
Vec3f up{ 0,1,0 };

TGAImage total(1024, 1024, TGAImage::GRAYSCALE);
TGAImage  occl(1024, 1024, TGAImage::GRAYSCALE);

struct ZShader : public IShader {
	// triangle coordinates (clip coordinates), written by VS, read by FS
	mat<4, 3, float> varying_tri;

	virtual Vec4f vertex(int iface, int nthvert) {
		// read the vertex from .obj file and transform it to screen coordinates
		Vec4f gl_Vertex = Viewport * Projection * ModelView * embed<4>(model->vert(iface, nthvert));
		varying_tri.set_col(nthvert, gl_Vertex / gl_Vertex[3]);
		// return vertex coordinates
		return gl_Vertex;
	}

	virtual bool fragment(Vec3f gl_FragCoord, Vec3f bar, TGAColor &color) {
		color = TGAColor(255, 255, 255) * ((gl_FragCoord.z + 1.f) / 2.f);
		// no, we do not discard this pixel
		return false;
	}
};

struct Shader : public IShader {
	mat<2, 3, float> varying_uv;		// triangle uv coordinates, written by the VS read by the FS
	mat<4, 3, float> varying_tri;		// triangle coordinates before Viewport transform, written by VS, read by FS


	virtual Vec4f vertex(int iface, int nthvert) {
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		Vec4f gl_Vertex = Viewport * Projection * ModelView * embed<4>(model->vert(iface, nthvert));
		varying_tri.set_col(nthvert, gl_Vertex);
		return gl_Vertex;
	}

	virtual bool fragment(Vec3f gl_FragCoord, Vec3f bar, TGAColor &color) {
		Vec2f uv = varying_uv * bar;
		if (std::abs(shadowbuffer[int(gl_FragCoord.x + gl_FragCoord.y * width)] - gl_FragCoord.z < 1e-2)) {
			occl.set(uv.x * 1024, uv.y * 1024, TGAColor(255));
		}
		color = TGAColor(255, 0, 0);
		return false;
	}
};

Vec3f rand_point_on_unit_sphere() {
	float u = (float)rand() / (float)RAND_MAX;
	float v = (float)rand() / (float)RAND_MAX;
	float theta = 2.f * M_PI * u;
	float phi = acos(2.f * v - 1.f);
	// convert spherical coordinates to cartesian coordinate
	//  x = rsin(phi)cos(theta)
	//  y = rsin(phi)sin(theta)
	//  z = rcos(phi)
	return Vec3f(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
}

int main(int argc, char** argv) {
	if (2 > argc) {
		std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1;
	}

	float *zbuffer = new float[width*height];
	shadowbuffer = new float[width*height];		// defined globally -> used in the fragmetn shader
	model = new Model(argv[1]);

	TGAImage frame(width, height, TGAImage::RGB);
	lookat(eye, center, up);
	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	projection(-1.f / (eye - center).norm());
	for (int i = width*height; --i; ) {
		zbuffer[i] = shadowbuffer[i] = -std::numeric_limits<float>::max();
	}


	delete model;
	delete [] shadowbuffer;
	delete [] zbuffer;
	return 0;
}