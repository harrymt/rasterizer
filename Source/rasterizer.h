#ifndef RASTERIZER_INCLUDE
#define RASTERIZER_INCLUDE
#include <iostream>
#include <glm/glm.hpp>
#include <SDL.h>
#include <stdexcept>
#include <cstdlib>
#include <ctime>
#include <limits.h>
#include <cmath>
#include "SDLauxiliary.h"
#include "TestModel.h"
#include <sstream>
#include <fstream>

#define OPEN_CL

#ifdef OPEN_CL
#include <CL/opencl.h>
#endif


#define FOCAL 2.0f
#define SCREEN_WIDTH 512
#define SCREEN_HEIGHT 512
#define FOCAL_LIGHT 2.0f
#define LIGHT_WIDTH 512
#define LIGHT_HEIGHT 512
#define FOCAL_LENGTH SCREEN_HEIGHT / FOCAL
#define FOCAL_LENGTH_LIGHT LIGHT_HEIGHT / FOCAL_LIGHT
#define GROUP_SIZE 256

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

const float pi = (float) atan(1.0)*4;

struct pixel_t;
struct fpixel_t;
typedef glm::vec3 vertex_t;

struct pixel_t
{
    int x, y;
    float zinv;
    glm::vec3 pos3d;
    int lx, ly;
    float lzinv;
    /*
    inline pixel_t() {}
    pixel_t(int x, int y, float zinv, glm::vec3 pos3d);
    pixel_t(const fpixel_t& p);
    friend pixel_t operator-(const pixel_t& a, const pixel_t& b);
    friend pixel_t operator+(const pixel_t& a, const pixel_t& b);*/
};

/*struct fpixel_t
{
    float x, y;
    float zinv;
    glm::vec3 pos3d;

    fpixel_t(float x, float y, float zinv, glm::vec3 pos3d);
    fpixel_t(const pixel_t& p);
    friend fpixel_t operator/(const fpixel_t& a, const float f);
    friend fpixel_t& operator+=(fpixel_t& a, const fpixel_t& b);
    friend fpixel_t operator*(int n, const fpixel_t& a);
};*/

#ifdef OPEN_CL
inline cl_float3 toFloat3(glm::vec3& v)
{
    cl_float3 ret;
    memcpy(&ret, &v, sizeof(glm::vec3));
    return ret;
}

inline glm::vec3 fromFloat3(cl_float3* v)
{
    glm::vec3 ret;
    memcpy(&ret, v, sizeof(glm::vec3));
    return ret;
}
#else
#define toFloat3(v) v
#define fromFloat3(v) (*v)
#endif

#ifdef OPEN_CL
typedef cl_float3 float3;
#else
typedef glm::vec3 float3;
#endif

struct framebuffer_t
{
    float depths[SCREEN_HEIGHT][SCREEN_WIDTH];
    float3 normals[SCREEN_HEIGHT][SCREEN_WIDTH];
    float3 colours[SCREEN_HEIGHT][SCREEN_WIDTH];
#ifndef OPEN_CL
    float3 fxaa_colours[SCREEN_HEIGHT][SCREEN_WIDTH];
#endif
    float3 positions[SCREEN_HEIGHT][SCREEN_WIDTH];
    float3 light_positions[SCREEN_HEIGHT][SCREEN_WIDTH];
};

//float interpolate_f(float start, float end, float step, float max);
//void interpolate(float start, float end, vector<float>& result);
//void interpolateVector(const glm::ivec2& a, const glm::ivec2& b, vector<glm::ivec2>& result);
//void interpolatePixel(const pixel_t& a, const pixel_t& b, vector<pixel_t>& result);
void update();
void draw();

void vertexShader(const vertex_t& v, pixel_t& p);
#ifndef OPEN_CL
void pixelShader(const int x, const int y);
#endif

//void drawPolygonEdges(const vector<vertex_t>& vertices);
//void drawLineSDL(SDL_Surface* surface, const pixel_t& a, const pixel_t& b, const colour_t& colour);
//void computePolygonRows(const pixel_t* vertex_pixels, vector<pixel_t>& left_pixels, vector<pixel_t>& right_pixels);
//void drawRows(const vector<pixel_t>& left_pixels, const vector<pixel_t>& right_pixels, const vector<pixel_t>& left_light, const vector<pixel_t>& right_light, glm::vec3 normal, glm::vec3 colour);
void drawPolygon(Triangle& triangle);
void rasterize(const pixel_t* vertex_pixels, Triangle& triangle);
void rasterizeLight(const pixel_t* vertex_pixels, Triangle& triangle);

#ifndef OPEN_CL
void fxaa(int x, int y);
#endif

#ifdef OPEN_CL
typedef struct
{
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;

    int num_work_groups;
    int work_group_size;

    cl_program program;
    cl_kernel fxaa;
    cl_kernel pixelShader;

    cl_mem depths;
    cl_mem normals;
    cl_mem colours;
    cl_mem fxaa_colours;
    cl_mem positions;
    cl_mem light_depths;
    cl_mem light_positions;
} ocl_t;
#endif

#endif //RASTERIZER_INCLUDE
