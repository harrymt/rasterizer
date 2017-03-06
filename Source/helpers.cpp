#include "raytracer.h"
#include "float.h"

extern glm::vec3 indirectLight;
extern glm::vec3 lightPos;
extern glm::vec3 lightColor;

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

void interpolate_v(vec3 a, vec3 b, vector<vec3> &result)
{
    float max_size = result.size();
    for (float i = 0; i < max_size; i++)
    {
        result[i].x = interpolate_f(a.x, b.x, i, max_size);
        result[i].y = interpolate_f(a.y, b.y, i, max_size);
        result[i].z = interpolate_f(a.z, b.z, i, max_size);
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
