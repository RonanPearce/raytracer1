//Using SDL and standard IO
#include <SDL.h>
#include <iostream>
#include <fstream>
#include "sphere.h"
#include "moving_sphere.h"
#include "hitable_list.h"
#include "bvh.h"
#include "camera.h"
#include "metal.h"
#include "lambertian.h"
#include "dielectric.h"
#include <cfloat>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <deque>
#include <conio.h>
#include <chrono>
#include "utils.h"

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"


const int nx = 384;
const int ny = 216;
const int ns = 25000;
const int MAX_BOUNCES = 100;
const int NUM_THREADS = 14;
const int TILE_MAX_HEIGHT = 32;
const int TILE_MAX_WIDTH = 32;
const bool DO_ANIMATE = false;
const bool WRITE_OUTPUT = true;
const double PROGRESS_DRAW_DELAY = 5000;
bool quit = false;


struct Tile
{
	int left;
	int top;
	int width;
	int height;
};

struct JobInfo
{
	std::function<void(int, int)> fn;
	int tileIndex;
	int n;
	bool stop;
};

std::vector<std::thread> threads;
std::mutex jobMutex;
std::mutex jobDoneMutex;
std::condition_variable jobQueueCondition;
std::condition_variable jobsDoneCondition;
std::deque<JobInfo> jobQueue;
std::atomic<int> remainingJobs = 0;

void runThread() {
	while (!quit) {
		//std::cout << "Waiting for job" << std::endl;
		JobInfo job;
		{
			std::unique_lock<std::mutex> lock(jobMutex);
			jobQueueCondition.wait(lock, [] {return quit || !jobQueue.empty(); });
			if (quit) {
				return;
			}
			job = jobQueue.front();
			jobQueue.pop_front();
			//std::cout << "Taking job: " << job.tileIndex << ", " << job.n <<std::endl;
		}
		job.fn(job.tileIndex, job.n);
		remainingJobs--;
		jobQueueCondition.notify_all();
		jobsDoneCondition.notify_one();
	}
	//std::cout << "Quitting!" << std::endl;
}

void addJob(const JobInfo & newJob) {
	//std::cout << "Adding job: " << newJob.tileIndex << ", " << newJob.n << std::endl;
	std::unique_lock<std::mutex> lock(jobMutex);
	remainingJobs++;
	jobQueue.push_back(newJob);
	jobQueueCondition.notify_one();
}


vec3 color(const ray& r, hitable *world, int curBounce) {
	hit_record rec;
	if (world->hit(r, 0.001f, FLT_MAX, rec)) {
		ray scattered;
		vec3 attenuation;
		if (curBounce < MAX_BOUNCES && rec.mat_ptr->scatter(r, rec, attenuation, scattered)) {
			return attenuation * color(scattered, world, curBounce + 1);
		}
		else {
			return vec3(0.0f, 0.0f, 0.0f);
		}
	}
	else {
		vec3 unit_direction = unit_vector(r.direction());
		float t = 0.5f * (unit_direction.y() + 1.0f);
		return (1.0f - t) * vec3(1.0f, 1.0f, 1.0f) + t * vec3(0.5f, 0.7f, 1.0f);
	}
}

hitable * world;
vec3 lookfrom(13.0f, 2.0f, 3.0f);
vec3 lookat(0.0f, 0.0f, 0.0f);
float dist_to_focus = 10.0f;
float aperture = 0.05f;

camera cam(lookfrom, lookat, vec3(0.0f, 1.0f, 0.0f), 20.0f, float(nx)/float(ny), aperture, dist_to_focus, 0.0f, 1.0f);
float R = cos(float(M_PI) / 4.0f);
vec3 resultBuffer[nx *ny];
std::vector<Tile> tiles;

void task(int tileIndex, int n) {

	const double nsDouble = double(ns);
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
				double col0 = 0;
				double col1 = 0;
				double col2 = 0;
				for (int s = 0; s < ns; s++) {
					float u = float(i + drand48()) / float(nx);
					float v = float(j + drand48()) / float(ny);
					ray r = cam.get_ray(u, v);
					vec3 p = r.point_at_parameter(2.0f);
					vec3 col = color(r, world, 0);
					col0 += col[0];
					col1 += col[1];
					col2 += col[2];
				}
				(*pBuff)[0] = (float(col0 / nsDouble));
				(*pBuff)[1] = (float(col1 / nsDouble));
				(*pBuff)[2] = (float(col2 / nsDouble));
				pBuff++;
			}
		}
	}
}

char * writeOutputData = new char[nx * ny * 3];
void writeOutput() {
	//	Execute Gimp
	//"C:\ProgramData\Microsoft\Windows\Start Menu\Programs\GIMP 2.lnk" "C:\Users\Ronan\Documents\Visual Studio 2017\Projects\RayTracerOneWeekend\RayTracerOneWeekend\output.png"

	char * pOut = writeOutputData;
	for (int j = ny - 1; j >= 0; j--) {
		for (int i = 0; i < nx; i++) {
			auto & col = resultBuffer[j * nx + i];
			int ir = int(255.99 * (col[0]));
			int ig = int(255.99 * (col[1]));
			int ib = int(255.99 * (col[2]));
			*pOut++ = char(ir);
			*pOut++ = char(ig);
			*pOut++ = char(ib);
		}
	}

	stbi_write_png("output.png", nx, ny, 3, writeOutputData, nx * 3);

}

char * displayOutputData = new char[nx * ny * 4];
void displayOutput() {
	char * pOut = displayOutputData;
	for (int j = ny - 1; j >= 0; j--) {
		for (int i = 0; i < nx; i++) {
			auto & col = resultBuffer[j * nx + i];
			int ir = int(255.99 * (col[0]));
			int ig = int(255.99 * (col[1]));
			int ib = int(255.99 * (col[2]));
			*pOut++ = char(ib);
			*pOut++ = char(ig);
			*pOut++ = char(ir);
			*pOut++ = 0;
		}
	}
}

void render()
{
	// Clear result
	for (int i = 0; i < nx * ny; i++) {
		resultBuffer[i].zero();
	}

	const int divX = nx / TILE_MAX_WIDTH;
	const int modX = nx % TILE_MAX_WIDTH;
	const int numX = divX + (modX > 0 ? 1 : 0);
	const int divY = ny / TILE_MAX_HEIGHT;
	const int modY = ny % TILE_MAX_HEIGHT;
	const int numY = divY + (modY > 0 ? 1 : 0);

	// Create tiles
	if (tiles.size() == 0) {
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
	}

	// Assign tiles to jobs
	const int numTiles = numY * numX;
	const int divTiles = numTiles / NUM_THREADS;
	const int modTiles = numTiles % NUM_THREADS;

	remainingJobs = 0;
	for (int curTile = 0; curTile < numTiles; ++curTile) {
		JobInfo jobInfo;
		jobInfo.fn = task;
		jobInfo.stop = false;
		jobInfo.tileIndex = curTile;
		jobInfo.n = 1;
		jobInfo.n = 1;
		addJob(jobInfo);
	}
}

hitable * random_scene() {
	int n = 50000;
	hitable ** list = new hitable*[n + 1];
	list[0] = new sphere(vec3(0.0f, -1000.f, 0.0f), 1000.0f, new metal(vec3(0.25f, 0.6f, 0.25f), 0.8f));
	//list[0] = new sphere(vec3(0.0f, -1000.f, 0.0f), 1000.0f, new dielectric(1.5f));
	int i = 1;
	for (int a = -10; a < 10; a++) {
		for (int b = -10; b < 10; b++) {
			float choose_mat = float(drand48());
			vec3 center(a + 0.9f * float(drand48()), 0.15f, b + 0.9f * float(drand48()));
			if ((center - vec3(4.0f, 0.2f, 0.0f)).length() > 0.9f) {
				if (choose_mat < 0.8f) {
					list[i++] = new sphere(center, 0.2f, new lambertian(vec3(float(drand48() * drand48()), float(drand48() * drand48() * 0.6f), float(drand48() * drand48()))));
				}
				else if (choose_mat < 0.95f) {
					list[i++] = new sphere(center, 0.2f, new metal(vec3(0.5f *(1.0f + float(drand48())), 0.5f*(1.0f + float(drand48())), 0.5f*(1.0f + float(drand48()))), 0.5f * float(drand48())));
				}
				else {
					list[i++] = new sphere(center, 0.2f, new dielectric(1.5f));
				}
			}
		}
	}
	list[i++] = new sphere(vec3(0.0f, 1.0f, 0.0f), 1.0f, new dielectric(1.5f));
	list[i++] = new sphere(vec3(-4.0f, 1.0f, 0.0f), 1.0f, new lambertian(vec3(0.4f, 0.2f, 0.1f)));
	list[i++] = new sphere(vec3(4.0f, 1.0f, 0.0f), 1.0f, new metal(vec3(0.7f, 0.6f, 0.5f), 0.0f));

	//return new hitable_list(list, i);

	return new bvh_node(list, i, 0.0, 1.0);
}

int main(int argc, char* argv[]) {

	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window *MainWindow = SDL_CreateWindow("My Game Window",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		640, (640 * ny) / nx,
		SDL_WINDOW_SHOWN
	);

	SDL_Renderer *renderer = SDL_CreateRenderer(MainWindow, -1, 0);

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	SDL_RenderClear(renderer);

	SDL_Texture *Tile = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING, nx, ny);


	auto start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	srand(start.count());

	world = random_scene();
	
	for (int i = 0; i < NUM_THREADS; i++)
	{
		threads.push_back(std::thread(runThread));
	}


	SDL_Event ev;
	render();
	double timeAtLastDraw = 0;
	while (!quit) {
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_KEYDOWN:
				switch (ev.key.keysym.sym) {
				case SDLK_UP:
					lookfrom = vec3(lookfrom[0], lookfrom[1], lookfrom[2] + 0.1f);
					cam.setup(lookfrom, lookat, vec3(0.0f, 1.0f, 0.0f), 20.0f, float(nx) / float(ny), aperture, dist_to_focus, 0.0f, 1.0f);
					break;
				case SDLK_DOWN:
					lookfrom = vec3(lookfrom[0], lookfrom[1], lookfrom[2] - 0.1f);
					cam.setup(lookfrom, lookat, vec3(0.0f, 1.0f, 0.0f), 20.0f, float(nx) / float(ny), aperture, dist_to_focus, 0.0f, 1.0f);
					break;
				}
				break;

			case SDL_QUIT:
				quit = true;
				break;
			}
		}
		
		if (DO_ANIMATE) {
			std::unique_lock<std::mutex> lock(jobDoneMutex);
			jobsDoneCondition.wait(lock, []() {return remainingJobs.load() == 0; });
			render();
			SDL_Delay(1);
		}
		else {
			auto cur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
			double timeSinceStart = (cur - start).count();
			if (timeSinceStart - timeAtLastDraw >= PROGRESS_DRAW_DELAY) {
				displayOutput();
				SDL_UpdateTexture(Tile, NULL, displayOutputData, nx * 4);
				SDL_RenderCopy(renderer, Tile, NULL, NULL);
				SDL_RenderPresent(renderer);
				timeAtLastDraw = timeSinceStart;
			}
			SDL_Delay(67);
		}
	}

	// Stop jobs
	jobQueueCondition.notify_all();
	for (int i = 0; i < NUM_THREADS; i++)
	{
		threads[i].join();
	}

	if (WRITE_OUTPUT) {
		writeOutput();
	}

	auto end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	std::chrono::duration<double> elapsed_seconds = end - start;

	std::cout << "Time taken: " << elapsed_seconds.count() << std::endl
		<< "Press any key to continue..." << std::endl;
	_getch();

	//Clean up
	SDL_DestroyTexture(Tile);
	SDL_DestroyWindow(MainWindow);
	SDL_Quit();

	return 0;
}