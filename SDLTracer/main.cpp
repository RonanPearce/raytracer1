//Using SDL and standard IO
#include <SDL.h>
#include <iostream>
#include <fstream>
#include "sphere.h"
#include "hitable_list.h"
#include "camera.h"
#include <cfloat>
#include <thread>
#include <vector>
#include <conio.h>
#include <chrono>
#include <cstdio>
#include <cstring>

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"


const int nx = 320;
const int ny = 160;
const int ns = 25;
const int MAX_BOUNCES = 4;
const int NUM_THREADS = 8;
const int TILE_MAX_HEIGHT = 64;
const int TILE_MAX_WIDTH = 64;

struct Tile
{
	int left;
	int top;
	int width;
	int height;
};


inline double drand48() {
	return double(rand()) / double(RAND_MAX);
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
		return 0.5f * color(maxBounces + 1, ray(rec.p, target - rec.p), world);
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

vec3 resultBuffer[nx *ny];
std::vector<Tile> tiles;

void task(int tileIndex, int n) {
	//std::cout << "TileSpan: " << tileIndex << "," << tileIndex + n << std::endl;
	for (int i = tileIndex; i < tileIndex + n; i++) {
		const Tile& tile = tiles[i];
		//std::cout << "Tile" << std::endl
		//	<< "  left: " << tile.left << std::endl
		//	<< "  top: " << tile.top << std::endl
		//	<< "  width: " << tile.width << std::endl
		//	<< "  height: " << tile.height << std::endl;
		const int startJ = tile.top;
		const int startI = tile.left;
		const int endJ = startJ + tile.height;
		const int endI = startI + tile.width;
		//std::cout << startJ << ", "  << endJ << ", " << startI << ", " << endI << std::endl;
		for (int j = startJ; j < endJ; j++) {
			vec3* pBuff = &(resultBuffer[j * nx + startI]);
			for (int i = startI; i < endI; i++) {
				vec3 col(0.0f, 0.0f, 0.0f);
				for (int s = 0; s < ns; s++) {
					float u = float(i + drand48()) / float(nx);
					float v = float(j + drand48()) / float(ny);
					ray r = cam.get_ray(u, v);
					vec3 p = r.point_at_parameter(2.0f);
					col += color(1, r, world);
				}
				col /= float(ns);
				(*pBuff)[0] = col[0];
				(*pBuff)[1] = col[1];
				(*pBuff)[2] = col[2];
				pBuff++;
			}
		}
	}
}

char * outputData = new char[nx * ny * 4];
void writeOutput() {
	//	Execute Gimp
	//"C:\ProgramData\Microsoft\Windows\Start Menu\Programs\GIMP 2.lnk" "C:\Users\Ronan\Documents\Visual Studio 2017\Projects\RayTracerOneWeekend\RayTracerOneWeekend\output.png"

	char * pOut = outputData;

	for (int j = ny-1; j >=0; j--) {
		for (int i = 0; i < nx; i++) {
			auto & col = resultBuffer[j * nx + i];
			//int ir = int(255.99 * sqrt(col[0]));
			//int ig = int(255.99 * sqrt(col[1]));
			//int ib = int(255.99 * sqrt(col[2]));
			int ir = int(255.99 * (col[0]));
			int ig = int(255.99 * (col[1]));
			int ib = int(255.99 * (col[2]));
			*pOut++ = char(ib);
			*pOut++ = char(ig);
			*pOut++ = char(ir);
			*pOut++ = 0;
		}
	}

	//stbi_write_png("output.png", nx, ny, 3, outputData, nx * 3);
}

float delta = 0.0f;
int render()
{
	auto start = std::chrono::system_clock::now();

	srand(start.time_since_epoch().count());

	// Clear result
	for (int i = 0; i < nx * ny; i++) {
		resultBuffer[i].zero();
	}

	// Define scene hitables
	delta += 0.05f;
	list[0] = new sphere(vec3(0.0f, 0.0f, -1.0f), 0.5f, 0);
	list[1] = new sphere(vec3(0.0f, -100.5f, -1.0f), 100.0f - delta, 0);
	list[2] = new sphere(vec3(0.5f, 0.0f, -1.0f), 0.25f, 0);

	const int divX = nx / TILE_MAX_WIDTH;
	const int modX = nx % TILE_MAX_WIDTH;
	const int numX = divX + (modX > 0 ? 1 : 0);
	const int divY = ny / TILE_MAX_HEIGHT;
	const int modY = ny % TILE_MAX_HEIGHT;
	const int numY = divY + (modY > 0 ? 1 : 0);

	// Create tiles
	for (int ty = 0; ty < numY; ty++) {
		for (int tx = 0; tx < numX; tx++) {
			Tile t;
			t.left = tx * TILE_MAX_WIDTH;
			t.width = tx >= divX ? modX : TILE_MAX_WIDTH;
			t.top = ty * TILE_MAX_HEIGHT;
			t.height = ty >= divY ? modY : TILE_MAX_HEIGHT;
			tiles.push_back(t);
		}
	}

	// Assign tiles to threads
	const int numTiles = numY * numX;
	const int divTiles = numTiles / NUM_THREADS;
	const int modTiles = numTiles % NUM_THREADS;
	std::vector<std::thread> threads;
	int curTile = 0;
	int curThread = 0;
	for (; (curThread < modTiles) && (curTile < numTiles); ++curThread) {
		threads.push_back(std::thread(task, curTile, 1 + divTiles));
		curTile += 1 + divTiles;
	}
	for (; (curThread < NUM_THREADS) && (curTile < numTiles); ++curThread) {
		threads.push_back(std::thread(task, curTile, divTiles));
		curTile += divTiles;
	}

	// Run threads
	for (int it = 0; it < threads.size(); ++it) {
		auto& th = threads[it];
		th.join();
	}
	auto end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end - start;

	writeOutput();

	//std::cout << "Time taken: " << elapsed_seconds.count() << std::endl
	//	<< "Press any key to continue..." << std::endl;
	//_getch();
}


int main(int argc, char* argv[]) {

	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window *MainWindow = SDL_CreateWindow("My Game Window",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		640, 320,
		SDL_WINDOW_SHOWN
	);

	SDL_Renderer *renderer = SDL_CreateRenderer(MainWindow, -1, 0);

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	SDL_RenderClear(renderer);

	SDL_Texture *Tile = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING, nx, ny);

	for (int i = 0; i < 500; ++i) {
		render();

		SDL_UpdateTexture(Tile, NULL, outputData, nx * 4);

		SDL_RenderCopy(renderer, Tile, NULL, NULL);
		SDL_RenderPresent(renderer);
		SDL_Delay(10);
	}

	//Clean up
	SDL_DestroyTexture(Tile);
	SDL_DestroyWindow(MainWindow);
	SDL_Quit();

	return 0;
}