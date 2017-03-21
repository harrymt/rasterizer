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
#define SCREEN_WIDTH 750
#define SCREEN_HEIGHT 750
#define FOCAL_LENGTH SCREEN_HEIGHT / FOCAL

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

constexpr float pi = atan(1.0);

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
    pixel_t(int x, int y, float zinv);
    pixel_t(const fpixel_t& p);
    friend pixel_t operator-(const pixel_t& a, const pixel_t& b);
};

struct fpixel_t
{
    float x, y;
    float zinv;
    glm::vec3 pos3d;

    fpixel_t(float x, float y, float zinv);
    fpixel_t(const pixel_t& p);
    friend fpixel_t operator/(const fpixel_t& a, const float f);
    friend fpixel_t& operator+=(fpixel_t& a, const fpixel_t& b);
};

struct vertex_t
{
    glm::vec3 position;
};

float interpolate_f(float start, float end, float step, float max);
void interpolate(float start, float end, vector<float>& result);
void interpolateVector(const glm::ivec2& a, const glm::ivec2& b, vector<glm::ivec2>& result);
void interpolatePixel(const pixel_t& a, const pixel_t& b, vector<pixel_t>& result);
int rand_i(int min, int max);
float rand_f();
glm::vec2 convertTo2D(glm::vec3 coords);
void printVector(const char* name, glm::vec3 v);
void update();
void draw();

void vertexShader(const vertex_t& v, pixel_t& p);
void pixelShader(const pixel_t& p);

void drawPolygonEdges(const vector<vertex_t>& vertices);
void drawLineSDL(SDL_Surface* surface, const pixel_t& a, const pixel_t& b, const colour_t& colour);
void computePolygonRows(const vector<pixel_t>& vertex_pixels, vector<pixel_t>& left_pixels, vector<pixel_t>& right_pixels);
void drawRows(const vector<pixel_t>& left_pixels, const vector<pixel_t>& right_pixels);
void drawPolygon(const vector<vertex_t>& vertices);

#endif //RASTERIZER_INCLUDE
