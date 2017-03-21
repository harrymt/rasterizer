#include "rasterizer.h"
#include "float.h"

extern SDL_Surface* screen;

void printVector(const char* name, glm::vec3 v)
{
    cout << name << ": " << v.x << "," << v.y << "," << v.z << endl;
}

float rand_f(float min, float max)
{
    return min + ((float) rand()) / ((float) (RAND_MAX/(max - min)));
}

float interpolate_f(float start, float end, float step, float max)
{
    return (end - start) * (step / (max - 1)) + start;
}

void interpolateVector(const glm::ivec2& a, const glm::ivec2& b, vector<glm::ivec2>& result)
{
    int n = result.size();
    glm::vec2 step = glm::vec2(b - a) / (float) std::max(n-1, 1);
    glm::vec2 current(a);
    for (int i = 0; i < n; ++i)
    {
        result[i] = current;
        current += step;
    }
}

void interpolatePixel(const pixel_t& a, const pixel_t& b, vector<pixel_t>& result)
{
    int n = result.size();
    fpixel_t step = fpixel_t(b - a) / (float) std::max(n-1, 1);
    fpixel_t current(a);
    for (int i = 0; i < n; ++i)
    {
        result[i] = pixel_t(current);
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

glm::vec2 convertTo2D(glm::vec3 coords)
{
    float f = SCREEN_HEIGHT / 2;
    float u = f * coords.x / coords.z + SCREEN_WIDTH / 2;
    float v = f * coords.y / coords.z + SCREEN_HEIGHT / 2;
    return glm::vec2(u, v);
}

namespace glm
{
    pixel_t abs(pixel_t p)
    {
        return pixel_t(std::abs(p.x), std::abs(p.y), std::abs(p.zinv));
    }
}

void drawLineSDL(SDL_Surface* surface, const pixel_t& a, const pixel_t& b, const colour_t& colour)
{
    pixel_t delta = glm::abs(a - b);
    int pixels = glm::max(delta.x, delta.y) + 1;
    vector<pixel_t> line(pixels);
    interpolatePixel(a, b, line);
    for (auto& point : line)
    {
        //if (point.x < 0 || point.y < 0 || point.x > SCREEN_WIDTH || point.y > SCREEN_HEIGHT) continue;
        PutPixelSDL(surface, point.x, point.y, colour);
    }
}

void drawPolygonEdges(const vector<vertex_t>& vertices)
{
    int num_vertices = vertices.size();
    vector<pixel_t> projected_vertices(num_vertices);
    glm::vec3 colour(1, 1, 1);

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

void computePolygonRows(const vector<pixel_t>& vertex_pixels, vector<pixel_t>& left_pixels, vector<pixel_t>& right_pixels)
{
    int min = +std::numeric_limits<int>::max();
    int max = -std::numeric_limits<int>::max();
    for (auto& vertex : vertex_pixels)
    {
        min = MIN(min, vertex.y);
        max = MAX(max, vertex.y);
    }
    size_t nrows = max - min + 1;

    left_pixels.resize(nrows);
    right_pixels.resize(nrows);

    for (size_t i = 0; i < nrows; ++i)
    {
        left_pixels[i].x = +std::numeric_limits<int>::max();
        right_pixels[i].x = -std::numeric_limits<int>::max();
        left_pixels[i].y = right_pixels[i].y = i + min;
    }

    size_t num_vertices = vertex_pixels.size();
    for (size_t i = 0, j = 1; i < num_vertices; ++i, ++j)
    {
        if (i == num_vertices - 1) j = 0;
        vector<pixel_t> edge(std::abs(vertex_pixels[i].y - vertex_pixels[j].y) + 1);
        interpolatePixel(vertex_pixels[i], vertex_pixels[j], edge);
        for (pixel_t& pixel : edge)
        {
            pixel_t& left = left_pixels[pixel.y-min];
            pixel_t& right = right_pixels[pixel.y-min];
            if (pixel.x < left.x)
            {
                left.x = pixel.x;
                left.zinv = pixel.zinv;
            }
            if (pixel.x > right.x)
            {
                right.x = pixel.x;
                right.zinv = pixel.zinv;
            }
        }
    }
}

void drawRows(const vector<pixel_t>& left_pixels, const vector<pixel_t>& right_pixels)
{
    for (size_t i = 0; i < left_pixels.size(); ++i)
    {
        const pixel_t& left = left_pixels[i];
        const pixel_t& right = right_pixels[i];
        float zinv = left.zinv;
        float zinv_step;
        if (right.x - left.x == 0) zinv_step = 0;
        else zinv_step = (right.zinv - left.zinv)/(right.x - left.x);
        for (int j = left.x; j <= right.x; ++j, zinv += zinv_step)
        {
            if (left.y >= SCREEN_HEIGHT || j >= SCREEN_WIDTH
             || left.y < 0 || j < 0) continue;
            pixelShader(pixel_t(j, left.y, zinv));
        }
    }
}

void drawPolygon(const vector<vertex_t>& vertices)
{
    int num_vertices = vertices.size();
    vector<pixel_t> vertex_pixels(num_vertices);
    for (int i = 0; i < num_vertices; ++i)
    {
        vertexShader(vertices[i], vertex_pixels[i]);
    }

    vector<pixel_t> left_pixels;
    vector<pixel_t> right_pixels;
    computePolygonRows(vertex_pixels, left_pixels, right_pixels);
    drawRows(left_pixels, right_pixels);
}
