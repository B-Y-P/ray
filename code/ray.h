
struct Ray{ v3 pos, dir; };

struct Plane{
	v3 pos, n;
	u32 mat_slot;
};

struct Sphere{
	v3 center;
	f32 radius;
	u32 mat_slot;
};

struct Triangle{
	v3 p0, p1, p2;
	u32 mat_slot;
};

struct Light{
	v3 pos;
	f32 radius;
	f32 diffuse_power, specular_power;
	v3 diffuse_color, specular_color;
};

struct Material{
	v3 color;
	f32 reflect, refract; //ambient, diffuse, emit, shine;
	Image *texture;
};

enum SKYBOX_PLANE{
	SKYBOX_RIGHT,
	SKYBOX_LEFT,
	SKYBOX_FRONT,
	SKYBOX_BACK,
	SKYBOX_TOP,
	SKYBOX_BOTTOM,
	SKYBOX_COUNT,
};

struct Scene{
	Plane ground;

	Image box[SKYBOX_COUNT];

	Material *mats;
	Light    *lights;
	Sphere   *spheres;

	u32 mat_count;
	u32 light_count;
	u32 sphere_count;
};

struct Job_Entry{ i32 x0, y0, x1, y1; };

struct Thread_Queue{
	u32 *pixels;
	Scene *scene;
	Job_Entry *entries;
	u64 entry_count, read_index;
	volatile u64 finished_count;
};
