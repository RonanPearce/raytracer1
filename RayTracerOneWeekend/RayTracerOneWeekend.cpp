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
#include <thread>
#include <vector>

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"


const int nx = 800;
const int ny = 400;
const int ns = 500; // Per Thread
const int MAX_BOUNCES = 100;
const int NUM_THREADS = 16;

struct TraceBuffer
{
	vec3 **data;
};


inline double drand48() {
	return double(rand()) / RAND_MAX;
}

vec3 random_in_unit_sphere() {
	vec3 p;
	do
	{
		p = 2.0f * vec3(float(drand48()), float(drand48()), float(drand48())) - vec3(1.0f, 1.0f, 1.0f);
	} while (p.squared_length() >= 1.0f);
	return p;
}

vec3 color(const int maxBounces, const ray& r, hitable *world) {
	hit_record rec;
	if (world->hit(r, 0.001f, FLT_MAX, rec)) {
		if (maxBounces > MAX_BOUNCES) {
			return vec3(0.0f, 0.0f, 0.0f);
		}
		vec3 target = rec.p + rec.normal + random_in_unit_sphere();
		return 0.5f * color(maxBounces+ 1, ray(rec.p, target - rec.p), world);
	}
	else {
		vec3 unit_direction = unit_vector(r.direction());
		float t = 0.5f * (unit_direction.y() + 1.0f);
		return (1.0f - t) * vec3(1.0f, 1.0f, 1.0f) + t * vec3(0.5f, 0.7f, 1.0f);
	}
}


vec3 lower_left_corner(-2.0f, -1.0f, -1.0f);
vec3 horizontal(4.0f, 0.0f, 0.0f);
vec3 vertical(0.0f, 2.0f, 0.0f);
vec3 origin(0.0f, 0.0f, 0.0f);
hitable *list[3];
hitable *world = new hitable_list(list, 3);
camera cam;


TraceBuffer resultBuffer;

void trace(TraceBuffer& buffer) {
	for (int j = ny - 1; j >= 0; j--) {
		for (int i = 0; i < nx; i++) {
			vec3 col(0.0f, 0.0f, 0.0f);
			for (int s = 0; s < ns; s++) {
				float u = float(i + drand48()) / float(nx);
				float v = float(j + drand48()) / float(ny);
				ray r = cam.get_ray(u, v);
				vec3 p = r.point_at_parameter(2.0f);
				col += color(1, r, world);
			}
			col /= float(ns);
			buffer.data[i][j] = vec3(col[0], col[1], col[2]);
		}
	}
}

void writeOutput() {
	//	Execute Gimp
	//"C:\ProgramData\Microsoft\Windows\Start Menu\Programs\GIMP 2.lnk" "C:\Users\Ronan\Documents\Visual Studio 2017\Projects\RayTracerOneWeekend\RayTracerOneWeekend\output.png"

	char * outputData = new char[nx * ny * 3];
	char * pOut = outputData;

	for (int j = ny - 1; j >= 0; j--) {
		for (int i = 0; i < nx; i++) {
			auto & col = resultBuffer.data[i][j];
			int ir = int(255.99 * sqrt(col[0]));
			int ig = int(255.99 * sqrt(col[1]));
			int ib = int(255.99 * sqrt(col[2]));
			*pOut++ = char(ir);
			*pOut++ = char(ig);
			*pOut++ = char(ib);
		}
	}

	stbi_write_png("output.png", nx, ny, 3, outputData, nx * 3);
}

int main()
{
	list[0] = new sphere(vec3(0.0f, 0.0f, -1.0f), 0.5f, 0);
	list[1] = new sphere(vec3(0.0f, -100.5f, -1.0f), 100.0f, 0);
	list[2] = new sphere(vec3(0.5f, 0.0f, -1.0f), 0.25f, 0);

	std::vector<std::thread> threads;
	std::vector<TraceBuffer> buffers;
	buffers.resize(NUM_THREADS);

	for (int it = 0; it < NUM_THREADS; ++it) {
		buffers[it].data = new vec3*[nx];
		for (int x = 0; x < nx; x++)
		{
			buffers[it].data[x] = new vec3[ny];
			for (int y = 0; y < ny; y++) {
				buffers[it].data[x][y] = vec3(0.0f, 0.0f, 0.0f);
			}
		}
	}

	resultBuffer.data = new vec3*[nx];
	for (int x = 0; x < nx; x++)
	{
		resultBuffer.data[x] = new vec3[ny];
		for (int y = 0; y < ny; y++) {
			resultBuffer.data[x][y] = vec3(0.0f, 0.0f, 0.0f);
		}
	}

	for (int i = 0; i < NUM_THREADS; ++i) {
		threads.push_back(std::thread(trace, buffers[i]));
	}

	for (int it = 0; it < NUM_THREADS; ++it) {
		auto& th = threads[it];
		th.join();
		for (int j = ny - 1; j >= 0; j--) {
			for (int i = 0; i < nx; i++) {
				resultBuffer.data[i][j] += buffers[it].data[i][j];
			}
		}
	}

	for (int j = ny - 1; j >= 0; j--) {
		for (int i = 0; i < nx; i++) {
			resultBuffer.data[i][j] /= float(NUM_THREADS);
		}
	}

	writeOutput();
    return 0;
}



