#include "rasterizer.h"
#include "float.h"

extern glm::vec3 indirectLight;
extern glm::vec3 lightPos;
extern glm::vec3 lightColor;
extern SDL_Surface* screen;

/**
 * Calculates the direct light from an intersection.
 */
vec3 directLight(const Intersection &i, Triangle closestTriangle, const vector<Triangle>& triangles) {
    vec3 directionFromSurfaceToLight = glm::normalize(lightPos - i.position); // r hat
    float distFromLightPosandIntersectionPos = glm::distance(i.position, lightPos); // r
    vec3 normalOfSurface = glm::normalize(closestTriangle.normal); // n hat

    Intersection intersectFromThis;

    glm::vec3 colour = lightColor;

    // Check intersection from intersection to lightsource
    if(closestIntersection(i.position, directionFromSurfaceToLight, triangles, intersectFromThis))
    {
        // If in shadow, darken the colour
        if(intersectFromThis.triangleIndex != i.triangleIndex && intersectFromThis.distance < glm::length(directionFromSurfaceToLight))
        {
            colour -= vec3(10.5f, 10.5f, 10.5f);
        }
    }

    // (25)
    float area = 4 * pi * (distFromLightPosandIntersectionPos * distFromLightPosandIntersectionPos);

    vec3 powerPerArea = colour / area; // P / A0

    // (27)
    vec3 directIllumination = powerPerArea * glm::max(glm::dot(directionFromSurfaceToLight, normalOfSurface), 0.0f); // D

    return directIllumination;
}


/*
 * Checks for intersection against all triangles, if found returns true, else false.
 * If intersection found, then return info about the closest intersection.
 */
bool closestIntersection(vec3 start, vec3 dir, const vector<Triangle>& triangles, Intersection& closest)
{
    float minimumDistance = std::numeric_limits<float>::max();

    bool found = false;
    for (size_t i = 0; i < triangles.size(); ++i)
    {
        const vec3& v0 = triangles[i].v0;
        const vec3& v1 = triangles[i].v1;
        const vec3& v2 = triangles[i].v2;
        vec3 edge1 = v1 - v0;
        vec3 edge2 = v2 - v0;
        vec3 b = start - v0;
        mat3 A(-dir, edge1, edge2);

        // Instead of using matrix inverse we can use Cramer's rule
        // Namely, instead of x = A^-1*b
        // We have x_i = |A_i|/|A|
        // With A_i as the replacement of column i in A with b
        float detA = glm::determinant(A);
        mat3 A_i = A;
        A_i[0] = b;
        float t = glm::determinant(A_i)/detA;
        if (t < 0.0f) continue; // inequality 7
        A_i[0] = A[0];
        A_i[1] = b;
        float u = glm::determinant(A_i)/detA;
        if (u < 0.0f) continue; // inequality 8
        A_i[1] = A[1];
        A_i[2] = b;
        float v = glm::determinant(A_i)/detA;
        if (v < 0.0f || (u + v) > 1.0f) continue; // inequalities 9 & 11

        if(t < FLT_EPSILON) continue;

        // Check inequalities (7), (8), (9) and (11)
        vec3 point = v0 + (edge1 * u) + (edge2 * v);
        float r = glm::distance(start, point);
        if(r < minimumDistance)
        {
            minimumDistance = r;
            closest.triangleIndex = i;
            closest.position = point;
            closest.distance = r;
            found = true;
        }
    }
    return found;
}

void getRayDirection(int x, int y, vec3 &rayDir)
{
    rayDir.x = x - (SCREEN_WIDTH / 2);
    rayDir.y = y - (SCREEN_HEIGHT / 2);
    rayDir.z = FOCAL_LENGTH;
}

void printVector(const char* name, vec3 v)
{
    cout << name << ": " << v.x << "," << v.y << "," << v.z << endl;
}

bool triangleIntersection(vec3& point)
{
    float t = point.x;
    float u = point.y;
    float v = point.z;
    return (t >= 0 && u >= 0 && v >= 0 && (u + v) <= 1);
}

float rand_f(float min, float max)
{
    return min + ((float) rand()) / ((float) (RAND_MAX/(max - min)));
}

float interpolate_f(float start, float end, float step, float max)
{
    return (end - start) * (step / (max - 1)) + start;
}

void interpolateVector(const ivec2& a, const ivec2& b, vector<ivec2>& result)
{
    int n = result.size();
    vec2 step = vec2(b - a) / float(std::max(n-1, 1));
    vec2 current(a);
    for (int i = 0; i < n; ++i)
    {
        result[i] = current;
        current += step;
    }
}

void interpolate(float start, float end, vector<float>& result)
{
    float result_size = (float) result.size();

    if (result_size < 2.0) throw std::invalid_argument("Interpolate size was too small");

    for (float step = 0; step < result_size; step++)
    {
        result[step] = interpolate_f(start, end, step, result_size);
    }
}

vec2 convertTo2D(vec3 coords)
{
    float f = SCREEN_HEIGHT / 2;
    float u = f * coords.x / coords.z + SCREEN_WIDTH / 2;
    float v = f * coords.y / coords.z + SCREEN_HEIGHT / 2;
    return vec2(u, v);
}

void drawLineSDL(SDL_Surface* surface, const ivec2& a, const ivec2& b, const vec3& colour)
{
    ivec2 delta = glm::abs(a - b);
    int pixels = glm::max(delta.x, delta.y) + 1;
    vector<ivec2> line(pixels);
    interpolateVector(a, b, line);
    for (auto& point : line)
    {
        //if (point.x < 0 || point.y < 0 || point.x > SCREEN_WIDTH || point.y > SCREEN_HEIGHT) continue;
        PutPixelSDL(surface, point.x, point.y, colour);
    }
}

void drawPolygonEdges(const vector<vec3>& vertices)
{
    int num_vertices = vertices.size();
    vector<ivec2> projected_vertices(num_vertices);
    vec3 colour(1, 1, 1);

    // Loop jamming optimisation applied
    // Data dependency with j-1 and j
    // partial unrolling performed

    vertexShader(vertices[0], projected_vertices[0]);
    for (int i = 0, j = 1; i < num_vertices-1; ++i, ++j)
    {
        vertexShader(vertices[j], projected_vertices[j]);
        drawLineSDL(screen, projected_vertices[i], projected_vertices[j], colour);
    }
    drawLineSDL(screen, projected_vertices[num_vertices-1], projected_vertices[0], colour);
}

void computePolygonRows(const vector<ivec2>& vertex_pixels, vector<ivec2>& left_pixels, vector<ivec2>& right_pixels)
{
    int min = +std::numeric_limits<int>::max();
    int max = -std::numeric_limits<int>::max();
    for (auto& vertex : vertex_pixels)
    {
        min = MIN(min, vertex.y);
        max = MAX(max, vertex.y);
    }
    int nrows = max - min + 1;

    left_pixels.resize(nrows);
    right_pixels.resize(nrows);

    for (int i = 0; i < nrows; ++i)
    {
        left_pixels[i].x = +std::numeric_limits<int>::max();
        right_pixels[i].x = -std::numeric_limits<int>::max();
    }

    vector<ivec2> edge(nrows);
    int num_vertices = vertex_pixels.size();
    for (int i = 0, j = 1; i < num_vertices; ++i, ++j)
    {
        if (i == num_vertices - 1) j = 0;
        interpolateVector(vertex_pixels[i], vertex_pixels[j], edge);
        for (ivec2& pixel : edge)
        {
            ivec2& left = left_pixels[pixel.y];
            ivec2& right = right_pixels[pixel.y];
            left.x = MIN(left.x, pixel.x);
            right.x = MAX(right.x, pixel.x);
            right.y = left.y = pixel.y;
        }
    }
}
