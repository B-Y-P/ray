
/// RNG
#ifndef PCG_SEED
#define PCG_SEED time(NULL)
#endif

#ifndef PCG_INC
#define PCG_INC 1
#endif

struct Rng_State{ u64 state; };
function Rng_State MakeRNG(){ return Rng_State{ u64(PCG_SEED) }; }
function Rng_State MakeRNG(u64 state){ return Rng_State{state}; }

function u32 Rand_(Rng_State *rng){
    rng->state = rng->state*6364136223846793005ull + (PCG_INC|1);
    return (rng->state ^ (rng->state >> 22)) >> (22 + (rng->state >> 61));
}
function f32 RandNorm(Rng_State *rng){ return Rand_(rng)/f32(U32_MAX); }
function f32 RandBiNorm(Rng_State *rng){ return 2*((Rand_(rng)/f32(U32_MAX)) - 0.5f); }
function f32 Rand(Rng_State *rng, f32 min, f32 max){ return (max-min)*RandNorm(rng) + min; }

/// Vec
function f32 Dot(v2 a, v2 b){ return a.x*b.x + a.y*b.y; }
function f32 Dot(v3 a, v3 b){ return a.x*b.x + a.y*b.y + a.z*b.z; }

function v2 Hadamard(v2 a, v2 b){ return v2{a.x*b.x, a.y*b.y}; }
function v3 Hadamard(v3 a, v3 b){ return v3{a.x*b.x, a.y*b.y, a.z*b.z}; }

#define LenSq(v)    Dot(v,v)
#define Len(v) SqRt(Dot(v,v))
#define Normalize(v) (v)*(1.f/Len(v))
#define SetLen(v, len) len*Normalize(v)


function v3 Cross(v3 a, v3 b){
	return v3{
		a.y*b.z - a.z*b.y,
		a.x*b.z - a.z*b.x,
		a.x*b.y - a.y*b.x,
	};
}

function f32 GammaCorrect(f32 v){
	if(0){}
	else if(v < 0.f){ v = 0.f; }
	else if(v > 1.f){ v = 1.f; }
	f32 s = v*12.92f;
	if(v > 0.0031308f){ s = 1.055f*Pow(v, 1.f/2.4f) - 0.055f; }
	return s;
}
function v3 GammaCorrect(v3 v){
	return V3(GammaCorrect(v.r), GammaCorrect(v.g), GammaCorrect(v.b));
}

function v3 BiSample(Image *texture, f32 u, f32 v){
	u *= texture->wid;
	v *= texture->hit;
	f32 x0 = i32(u);
	f32 y0 = i32(v);
	f32 x1 = x0 + 1.f;
	f32 y1 = y0 + 1.f;

	u32 *p = (u32 *)texture->pixels + i32(x0 + texture->wid*y0);
	v3 c00 = V3(*(p));
	v3 c10 = V3(*(p + 1));
	v3 c01 = V3(*(p + texture->hit));
	v3 c11 = V3(*(p + texture->hit + 1));

	f32 a = u - x0;
	f32 b = v - y0;
	f32 ai = (1.f - a);
	f32 bi = (1.f - b);

	return bi*ai*c00 + bi*a*c01 + b*ai*c10 + b*a*c11;
}

#pragma pack(push, 1)
struct BMP_Header{
	u16 file_type;
	u32 file_size, reserve_zeroes;
	u32 offset, size;
	i32 wid, hit;
	u16 planes, bits_per_pixel;
	u32 compression, bitmap_size;
	i32 horz_res, vert_res;
	u32 colors_used, colors_important;
	u32 rmask, gmask, bmask;
	u8  padding[84];
};
#pragma pack(pop)

function Image LoadImage(const char *file_path){
	Image result = {};
	FILE *file = fopen(file_path, "rb");
	if(file == 0){ return result; }
	fseek(file, 0, SEEK_END);
	u64 bytes = ftell(file);
	result.pixels = (u32 *)malloc(bytes);
	if(result.pixels == 0){ return result; }
	fseek(file, 0, SEEK_SET);
	fread(result.pixels, bytes, 1, file);
	fclose(file);

	BMP_Header *header = (BMP_Header *)result.pixels;
	result.pixels = (u32 *)(header+1);
	result.wid = header->wid;
	result.hit = header->hit;
	u32 pixel_count = result.wid*result.hit;
	foreach(i, pixel_count){
		v3 c = V3(result.pixels[i]);
		c = Hadamard(c, c);
		result.pixels[i] = U32FromV3(c);
	}
	return result;
}