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


using glm::vec2;
using glm::vec3;
using glm::mat3;
using glm::ivec3;
using glm::ivec2;

using std::cout;
using std::vector;
using std::endl;

struct Intersection
{
    vec3 position;
    float distance;
    int triangleIndex;
};

struct pixel_t
{
    int x, y;
    float zinv;

    friend pixel_t operator+(const pixel_t& a, const pixel_t& b);
    friend pixel_t operator-(const pixel_t& a, const pixel_t& b);
    friend pixel_t operator/(const pixel_t& a, const float f);
    friend pixel_t& operator+=(pixel_t& a, const pixel_t& b);
};

vec3 directLight(const Intersection &i, Triangle closestTriangle, const vector<Triangle>& triangles);
float interpolate_f(float start, float end, float step, float max);
void interpolate(float start, float end, vector<float>& result);
void interpolateVector(const ivec2& a, const ivec2& b, vector<ivec2>& result);
void interpolatePixel(const pixel_t& a, const pixel_t& b, vector<pixel_t>& result);
int rand_i(int min, int max);
float rand_f();
vec2 convertTo2D(vec3 coords);
bool closestIntersection(vec3 start, vec3 dir, const vector<Triangle>& triangles, Intersection& closest);
void getRayDirection(int x, int y, vec3 &rayDir);
void printVector(const char* name, vec3 v);
bool triangleIntersection(vec3& point);
void update();
void draw();

void vertexShader(const vec3& v, ivec2& p);

void drawPolygonEdges(const vector<vec3>& vertices);
void drawLineSDL(SDL_Surface* surface, const ivec2& a, const ivec2& b, const vec3& colour);
void computePolygonRows(const vector<ivec2>& vertex_pixels, vector<ivec2>& left_pixels, vector<ivec2>& right_pixels);
void drawRows(const vector<ivec2>& left_pixels, const vector<ivec2>& right_pixels);
void drawPolygon(const vector<vec3>& vertices);

#endif //RASTERIZER_INCLUDE
