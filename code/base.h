
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

typedef uint8_t  b8;
typedef uint16_t b16;
typedef uint32_t b32;
typedef uint64_t b64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef  int8_t  i8;
typedef  int16_t i16;
typedef  int32_t i32;
typedef  int64_t i64;

typedef float  f32;
typedef double f64;

#define restrict __restrict

#define function static
#define local    static
#define global   static
#define global_const static const

#define foreach(i,N) for(i32 i=0; i<N; i++)
#define ArrayCount(a) (sizeof(a)/sizeof((a)[0]))

#define Min(A,B) ( ((A)<(B)) ? (A) : (B) )
#define Max(A,B) ( ((A)>(B)) ? (A) : (B) )

#define ClampTop(A,X) Min(A,X)
#define ClampBot(X,B) Max(X,B)
#define Clamp(A,X,B) (((X) < (A)) ? (A) :\
((X) > (B)) ? (B) : (X))
#define ClampN(p) Clamp(0.f, p, 1.f)

global_const f32 PI32 = 3.14159265258979f;
global_const f32 TAU32 = 6.28318531f;

global_const f32 F32_MAX = f32(0x7FFFFFFF);
global_const u32 U32_MAX = 0xFFFFFFFF;

/// Math
function f32 SqRt(f32 v){ return sqrtf(v); }
function f32 Cos(f32 v){ return cosf(v); }
function f32 Sin(f32 v){ return sinf(v); }
function f32 Tan(f32 v){ return tanf(v); }
function f32 Abs(f32 v){ return fabsf(v); }
function f32 Pow(f32 a, f32 b){ return powf(a, b); }
function f32 ATan2(f32 a, f32 b){ return atan2f(a, b); }
function f32 ASin(f32 v){ return asinf(v); }

#define Lerp(a,t,b) ((1.f-(t))*(a) + (t)*(b))

struct v2{
	union{
		struct{ f32 x,y; };
		struct{ f32 u,v; };
		f32 e[2];
	};
	v2 operator+(const v2 &v) const { return v2{x + v.x, y + v.y}; }
	v2 operator-(const v2 &v) const { return v2{x - v.x, y - v.y}; }
	v2 operator*(f32 s) const { return v2{x*s, y*s}; }
	v2& operator+=(const v2 &v){ return *this=(*this+v);}
	v2& operator-=(const v2 &v){ return *this=(*this-v);}
	v2& operator*=(f32 s) { return *this=(*this * s); }
	b32 operator==(const v2 &v) const { return x==v.x && y==v.y; }
	b32 operator!=(const v2 &v) const { return !(*this == v); }
};
v2 operator*(f32 s, const v2 &v){ return v*s; }
function v2 V2(f32 x, f32 y){ return v2{x,y}; }

struct v3{
	union{
		struct{ f32 x,y,z; };
		struct{ f32 r,g,b; };
		struct{ v2 xy; f32 ignore_z; };
		struct{ f32 ignore_x; v2 yz; };
		f32 e[3];
	};
	v3 operator+(const v3 &v) const { return v3{x + v.x, y + v.y, z + v.z}; }
	v3 operator-(const v3 &v) const { return v3{x - v.x, y - v.y, z - v.z}; }
	v3 operator*(f32 s) const { return v3{x*s, y*s, z*s}; }
	v3& operator+=(const v3 &v){ return *this=(*this+v);}
	v3& operator-=(const v3 &v){ return *this=(*this-v);}
	v3& operator*=(f32 s){ return *this=(*this * s); }
	b32 operator==(const v3 &v) const { return x==v.x && y==v.y && z==v.z; }
	b32 operator!=(const v3 &v) const { return !(*this == v); }
};
v3 operator*(f32 s, const v3 &v){ return v*s; }
function v3 V3(f32 x, f32 y, f32 z){ return v3{x,y,z}; }
function v3 V3(v2 v, f32 z){ return v3{v.x, v.y, z}; }
function v3 V3(u32 c){
	f32 r = ((c >> 16) & 0xFF)/255.f;
	f32 g = ((c >>  8) & 0xFF)/255.f;
	f32 b = ((c >>  0) & 0xFF)/255.f;
	return v3{r,g,b};
}
function u32 U32FromV3(v3 c){
	return u32((0xFF000000)           |
			   (u32(c.r*255.f) << 16) |
			   (u32(c.g*255.f) <<  8) |
			   (u32(c.b*255.f) <<  0));
}

global_const v3 AXIS_X = V3(1, 0, 0);
global_const v3 AXIS_Y = V3(0, 1, 0);
global_const v3 AXIS_Z = V3(0, 0, 1);

struct Image{
	i32 wid, hit;
	u32 *pixels;
};