
#define CoreCount   4
#define FOV         60.f
#define CAM_DIST    0.77f

#define WID 1280
#define HIT 720

#define RaysPerPixel 32
#define RaysPerBounce 16


#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define PCG_SEED 234587

#include "base.h"
#include "ray.h"

#include "base.cpp"
#include "ray.cpp"

function u64 InterlockedInc(volatile u64 *value){
	return InterlockedIncrement64((volatile LONG64 *)value)-1;
}

function bool RenderTile(Thread_Queue *queue){
	u32 *pixels = queue->pixels;
	Scene *scene = queue->scene;
	Rng_State rng = MakeRNG();

	u64 entry_index = InterlockedInc(&queue->read_index);
	if(entry_index >= queue->entry_count){
		return false;
	}

	Job_Entry *entry = queue->entries + entry_index;
	const i32 x0 = entry->x0;
	const i32 y0 = entry->y0;
	const i32 x1 = entry->x1;
	const i32 y1 = entry->y1;

	v3 cam_pos = V3(-0.4f, 0.f, 1.f);
	v3 cam_dir = V3(1.f, 0.f, -0.2f);
	cam_dir = Normalize(cam_dir);
	v3 cam_up = AXIS_Z;
	v3 cam_x = Cross(cam_dir, cam_up);


	f32 tan_theta = Tan(FOV*TAU32/360.f);
	f32 tan_phi = tan_theta / (f32(WID) / f32(HIT));
	f32 MetersPerPixel = (tan_theta*CAM_DIST) / (0.5f*WID);

	v3 screen_center = cam_pos + CAM_DIST*cam_dir;
	Plane screen_plane = {screen_center, cam_dir};
	f32 contrib = (1.f / RaysPerPixel);

	i32 pixel_count = (x1-x0)*(y1-y0);
	for(i32 i=y0; i<y1; i++){

		for(i32 j=x0; j<x1; j++){
			v3 color = {};

			foreach(r, RaysPerPixel){
				v3 dir = cam_dir;
				v2 diff = V2(j,i) - 0.5f*V2(WID, HIT);
				if(RaysPerPixel > 1){
					diff += 0.5f*V2(RandBiNorm(&rng), RandBiNorm(&rng));
				}
				dir += MetersPerPixel*diff.x*cam_x;
				dir += MetersPerPixel*diff.y*cam_up;
				dir = Normalize(dir);

				Ray ray = {cam_pos, dir};
				f32 t = -1.f;
				color += RayCast(ray, &t, scene, &rng);
			}
			color *= contrib;
			color = GammaCorrect(color);
			pixels[i*WID + j] = U32FromV3(color);

		}
	}

	InterlockedInc(&queue->finished_count);
	return true;
}

function DWORD WINAPI RayProc(void *data){
	while(RenderTile((Thread_Queue *)data));
	return 0;
}

function void RayCreateThread(void *param){
	DWORD thread_id;
	CloseHandle(CreateThread(0, 0, RayProc, param, 0, &thread_id));
}

int main(){
	u32 pixel_count = WID*HIT;
	u32 byte_count = sizeof(BMP_Header) + sizeof(u32)*pixel_count;
	BMP_Header *header = (BMP_Header *)malloc(byte_count);
	u32 *pixels = (u32 *)(header+1);

	*header = BMP_Header{
		0x4d42, byte_count,
		0, 150, 124,
		WID, HIT,
		1, 8*sizeof(u32),
		3, 0,
		2835, 2835, 0, 0,
		0xFF0000, 0xFF00, 0xFF,
	};

	///////////////////////////////////

	Image stone_img  = LoadImage("texture.bmp");
	Image wood_img   = LoadImage("wood.bmp");
	Image sky_img    = LoadImage("skybox.bmp");
	Image *stone = (stone_img.pixels ? &stone_img : 0);
	Image *wood  = (wood_img.pixels  ? &wood_img  : 0);
	Image *sky   = (sky_img.pixels   ? &sky_img   : 0);

	v3 cl_cyan   = V3(0xFF00FFFF);
	v3 cl_blue   = V3(0xFF0000FF);
	v3 cl_red    = V3(0xFFFF0000);
	v3 cl_yellow = V3(0xFFFFFF00);
	v3 cl_green  = V3(0xFF00FF00);
	v3 cl_purple = V3(0xFF8800FF);
	v3 cl_sky    = V3(0xFF00B4BF);
	v3 cl_ground = V3(0xFFAAA9Ad);
	v3 cl_white  = V3(0xFFFFFFFF);

	Material mats[] = {
		// color      |  reflect  |  refract  | texture
		{cl_sky,         0.f,        0.f,       sky},
		{cl_ground},
		{cl_cyan,        0.f,        1.58f},
		{V3(0xFF5555FF), 0.4f,       0.f,       stone},
		{V3(0xFFFF3333), 0.f,        0.f,       wood},
		{cl_white,       0.7f},
		{cl_purple},
		{cl_ground},
	};

	Light lights[] = {
		{V3(+0.f,  +1.f, +8.f),  0.8f, 1.0f, 0.7f, cl_white, cl_white},
		{V3(-2.f,  -3.f, +5.f),  0.3f, 0.8f, 0.7f, cl_white, cl_white},
		{V3(+1.2f, +0.f, +2.5f), 0.3f, 0.8f, 0.7f, cl_white, cl_white},
	};

	Sphere spheres[] = {
		{V3(+2.f,  +0.f,  +0.55f), 0.7f, 2},
		{V3(+1.4f, +0.7f, +1.3f),  0.3f, 3},
		{V3(+1.8f, -0.5f, +1.3f),  0.2f, 4},
		{V3(+3.4f, +2.7f, -0.1f),  0.9f, 5},
		{V3(+2.4f, -2.7f, +0.6f),  0.7f, 6},
		{V3(+4.8f, +0.8f, +0.22f), 1.3f, 7},
	};

	Scene scene;
	scene.ground = {V3(0.f, 0.f, -0.5f), AXIS_Z, 0};
	scene.mats = mats;
	scene.lights = lights;
	scene.spheres = spheres;
	scene.mat_count = ArrayCount(mats);
	scene.light_count = ArrayCount(lights);
	scene.sphere_count = ArrayCount(spheres);

	///////////////////////////////////

	// TODO(BYP): Indivisible values cause crashes not sure exact cause. Make this more robust
	i32 tile_wid = 10;
	i32 tile_hit = 10;
	i32 tile_count_x = WID/tile_wid;
	i32 tile_count_y = HIT/tile_hit;
	u32 job_count = tile_count_x*tile_count_y;
	printf("Tiles %dx%d = %d\n", tile_count_x, tile_count_y, job_count);
	Job_Entry *jobs = (Job_Entry *)malloc(job_count*sizeof(Job_Entry));
	foreach(i, tile_count_y){
		foreach(j, tile_count_x){
			Job_Entry *job = jobs + (i*tile_count_x + j);
			job->x0 = j*tile_wid;
			job->x1 = j*tile_wid + tile_wid;
			job->y0 = i*tile_hit;
			job->y1 = i*tile_hit + tile_hit;
		}
	}

	Thread_Queue queue = {};
	queue.pixels = pixels;
	queue.scene = &scene;
	queue.entries = jobs;
	queue.entry_count = job_count;

	// NOTE: Memory Fence
	volatile u64 fence;
	InterlockedInc(&fence);

	clock_t start_clock = clock();

	for(i32 i=1; i<CoreCount; i++){ RayCreateThread(&queue); }

	char fmt_str[] = "  Ray Tracing: %.2f%% \r";
	char spin[] = {'\\', '|', '/', '|'};

	while(queue.finished_count < queue.entry_count){
		if(RenderTile(&queue)){
			fmt_str[1] = spin[(clock() / 250) % 4];
			printf(fmt_str, (100.f*f32(queue.finished_count)/queue.entry_count));
			fflush(stdout);
		}
	}
	printf("  Ray Tracing: 100.00%% \n");

	clock_t elapsed = clock() - start_clock;
	printf("\nTime: %.3f (s)\n", (1.f/1000.f*elapsed));

	FILE *file = fopen("image.bmp", "wb");
	fwrite(header, byte_count, 1, file);
	fclose(file);
	free(header);

	return 0;
}