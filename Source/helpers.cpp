#include "rasterizer.h"
#include "float.h"

extern SDL_Surface* screen;

extern framedata_t frame_buffer[SCREEN_HEIGHT][SCREEN_WIDTH];

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
    glm::vec2 step = glm::vec2(b - a) / (float) std::fmax(n-1, 1);
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
    float max = float(std::fmax(n - 1, 1));
    glm::vec3 diff = glm::vec3(b.x - a.x, b.y - a.y, b.zinv - a.zinv) / max;
    glm::vec3 diffPos = glm::vec3(b.pos3d * b.zinv - a.pos3d * a.zinv) / max;

    glm::vec3 current(a.x, a.y, a.zinv);
    glm::vec3 currentPos(a.pos3d * a.zinv);

    for (int i = 0; i < n; ++i)
    {
        result[i].x = current.x;
        result[i].y = current.y;
        result[i].zinv = current.z;
        result[i].pos3d = currentPos / current.z;

        current.x += diff.x;
        current.y += diff.y;
        current.z += diff.z;
        currentPos += diffPos;
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

namespace glm
{
    pixel_t abs(pixel_t p)
    {
        return pixel_t(std::abs(p.x), std::abs(p.y), std::abs(p.zinv), glm::abs(p.pos3d));
    }
}

/*
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
}*/

void computePolygonRows(const pixel_t* vertex_pixels, vector<pixel_t>& left_pixels, vector<pixel_t>& right_pixels)
{
    int min = +std::numeric_limits<int>::max();
    int max = -std::numeric_limits<int>::max();
    for (int i = 0; i < 3; i++)
    {
        min = MIN(min, vertex_pixels[i].y);
        max = MAX(max, vertex_pixels[i].y);
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

    for (size_t i = 0, j = 1; i < 3; ++i, ++j)
    {
        if (i == 2) j = 0;
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
                left.pos3d = pixel.pos3d;
            }
            if (pixel.x > right.x)
            {
                right.x = pixel.x;
                right.zinv = pixel.zinv;
                right.pos3d = pixel.pos3d;
            }
        }
    }
}

void drawRows(const vector<pixel_t>& left_pixels,
              const vector<pixel_t>& right_pixels, 
              const vector<pixel_t>& left_light, 
              const vector<pixel_t>& right_light, 
              glm::vec3 normal, glm::vec3 colour)
{
    for (size_t i = 0; i < left_pixels.size(); ++i)
    {
        const pixel_t& left = left_pixels[i];
        const pixel_t& right = right_pixels[i];
        float zinv = left.zinv;
        float zinv_step;
        if (right.x - left.x == 0) zinv_step = 0;
        else zinv_step = (right.zinv - left.zinv)/(right.x - left.x);

        if (left.x == right.x) { continue; }
        vector<pixel_t> row(right_pixels[i].x - left_pixels[i].x + 1);
        interpolatePixel(left_pixels[i], right_pixels[i], row);
        for (int row_number = 0; row_number < row.size() && row[row_number].x <= right.x; row_number++, zinv += zinv_step)
        {
            if (row[row_number].y >= SCREEN_HEIGHT || row[row_number].x >= SCREEN_WIDTH
                || row[row_number].y < 0 || row[row_number].y < 0) continue;
            pixel_t& px = row[row_number];
            // Back face culling needs to be done for the light version!
            if (px.zinv > frame_buffer[px.y][px.x].depth)
            {
                framedata_t& data = frame_buffer[px.y][px.x];
                data.depth = px.zinv;
                data.normal = normal;
                data.colour = colour;
                data.pos = px.pos3d;
            }
        }
    }
}

void drawPolygon(Triangle& triangle)
{
    pixel_t vertex_pixels[3];
    pixel_t vertex_light[3];
    vertexShader(triangle.v0, vertex_pixels[0], vertex_light[0]);
    vertexShader(triangle.v1, vertex_pixels[1], vertex_light[1]);
    vertexShader(triangle.v2, vertex_pixels[2], vertex_light[2]);

    vector<pixel_t> left_pixels;
    vector<pixel_t> right_pixels;
    vector<pixel_t> left_light;
    vector<pixel_t> right_light;
    computePolygonRows(vertex_pixels, left_pixels, right_pixels);
    //computePolygonRows(vertex_light, left_light, right_light);
    drawRows(left_pixels, right_pixels, left_light, right_light, triangle.normal, triangle.color);
}
