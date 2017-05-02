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


#define FOCAL 2.0f
#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 500
#define FOCAL_LIGHT 1.0f
#define LIGHT_WIDTH 500
#define LIGHT_HEIGHT 500
#define FOCAL_LENGTH SCREEN_HEIGHT / FOCAL
#define FOCAL_LENGTH_LIGHT LIGHT_HEIGHT / FOCAL_LIGHT

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

const float pi = (float) atan(1.0);

using std::cout;
using std::vector;
using std::endl;

typedef glm::vec3 colour_t;

struct pixel_t;
struct fpixel_t;
struct vertex_t;

struct pixel_t
{
    int x, y;
    float zinv;
    glm::vec3 pos3d;

    inline pixel_t() {}
    pixel_t(int x, int y, float zinv, glm::vec3 pos3d);
    pixel_t(const fpixel_t& p);
    friend pixel_t operator-(const pixel_t& a, const pixel_t& b);
};

struct fpixel_t
{
    float x, y;
    float zinv;
    glm::vec3 pos3d;

    fpixel_t(float x, float y, float zinv, glm::vec3 pos3d);
    fpixel_t(const pixel_t& p);
    friend fpixel_t operator/(const fpixel_t& a, const float f);
    friend fpixel_t& operator+=(fpixel_t& a, const fpixel_t& b);
};

struct vertex_t
{
    glm::vec3 position;
};

struct framedata_t
{
    float depth;
    glm::vec3 normal;
    glm::vec3 colour;
    glm::vec3 fxaa_colour;
    glm::vec3 pos;
};

float interpolate_f(float start, float end, float step, float max);
void interpolate(float start, float end, vector<float>& result);
void interpolateVector(const glm::ivec2& a, const glm::ivec2& b, vector<glm::ivec2>& result);
void interpolatePixel(const pixel_t& a, const pixel_t& b, vector<pixel_t>& result);
void printVector(const char* name, glm::vec3 v);
void update();
void draw();

void vertexShader(const vertex_t& v, pixel_t& p, pixel_t& l);
void pixelShader(const int x, const int y);

//void drawPolygonEdges(const vector<vertex_t>& vertices);
//void drawLineSDL(SDL_Surface* surface, const pixel_t& a, const pixel_t& b, const colour_t& colour);
void computePolygonRows(const pixel_t* vertex_pixels, vector<pixel_t>& left_pixels, vector<pixel_t>& right_pixels);
void drawRows(const vector<pixel_t>& left_pixels, const vector<pixel_t>& right_pixels);
void drawPolygon(const vertex_t* vertices);

void fxaa(int x, int y);

#endif //RASTERIZER_INCLUDE
