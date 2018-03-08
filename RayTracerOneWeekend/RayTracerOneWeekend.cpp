// RayTracerOneWeekend.cpp : Defines the entry point for the console application.
//

// #define STB_IMAGE_IMPLEMENTATION

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include "sphere.h"
#include "hitable_list.h"
#include "camera.h"
#include <cfloat>

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

inline double drand48() {
	return double(rand()) / RAND_MAX;
}

void writeOutput(const char *const outputData);
const int nx = 400;
const int ny = 200;
const int ns = 100;

vec3 random_in_unit_sphere() {
	vec3 p;
	do
	{
		p = 2.0f * vec3(float(drand48()), float(drand48()), float(drand48())) - vec3(1.0f, 1.0f, 1.0f);
	} while (p.squared_length() >= 1.0f);
	return p;
}

vec3 color(const ray& r, hitable *world) {
	hit_record rec;
	if (world->hit(r, 0.001f, FLT_MAX, rec)) {
		vec3 target = rec.p + rec.normal + random_in_unit_sphere();
		return 0.5f * color(ray(rec.p, target - rec.p), world);
	}
	else {
		vec3 unit_direction = unit_vector(r.direction());
		float t = 0.5f * (unit_direction.y() + 1.0f);
		return (1.0f - t) * vec3(1.0f, 1.0f, 1.0f) + t * vec3(0.5f, 0.7f, 1.0f);
	}
}


int main()
{
	char * outputData = new char[nx * ny * 3];
	char * pOut = outputData;
	vec3 lower_left_corner(-2.0f, -1.0f, -1.0f);
	vec3 horizontal(4.0f, 0.0f, 0.0f);
	vec3 vertical(0.0f, 2.0f, 0.0f);
	vec3 origin(0.0f, 0.0f, 0.0f);
	hitable *list[2];
	list[0] = new sphere(vec3(0.0f, 0.0f, -1.0f), 0.5f, 0);
	list[1] = new sphere(vec3(0.0f, -100.5f, -1.0f), 100.0f, 0);
	hitable *world = new hitable_list(list, 2);
	camera cam;
	for (int j = ny - 1; j >= 0; j--) {
		for (int i = 0; i < nx; i++) {
			vec3 col(0.0f, 0.0f, 0.0f);
			for (int s = 0; s < ns; s++) {
				float u = float(i + drand48()) / float(nx);
				float v = float(j + drand48()) / float(ny);
				ray r = cam.get_ray(u, v);
				vec3 p = r.point_at_parameter(2.0f);
				col += color(r, world);
			}
			col /= float(ns);
			col = vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));
			int ir = int(255.99 * col[0]);
			int ig = int(255.99 * col[1]);
			int ib = int(255.99 * col[2]);
			*pOut++ = char(ir);
			*pOut++ = char(ig);
			*pOut++ = char(ib);
		}
	}
	writeOutput(outputData);
    return 0;
}

void writeOutput(const char *const outputData) {
	//	Execute Gimp
	//"C:\ProgramData\Microsoft\Windows\Start Menu\Programs\GIMP 2.lnk" "C:\Users\Ronan\Documents\Visual Studio 2017\Projects\RayTracerOneWeekend\RayTracerOneWeekend\output.png"

	stbi_write_png("output.png", nx, ny, 3, outputData, nx * 3);
}

