#include "rasterizer.h"
#include "float.h"

extern glm::vec3 indirectLight;
extern glm::vec3 lightPos;
extern glm::vec3 lightColor;
extern SDL_Surface* screen;
extern glm::vec3 currentColour;

void printVector(const char* name, vec3 v)
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

void interpolateVector(const ivec2& a, const ivec2& b, vector<ivec2>& result)
{
    int n = result.size();
    vec2 step = vec2(b - a) / (float) std::max(n-1, 1);
    vec2 current(a);
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
        vector<ivec2> edge(std::abs(vertex_pixels[i].y - vertex_pixels[j].y) + 1);
        interpolateVector(vertex_pixels[i], vertex_pixels[j], edge);
        for (ivec2& pixel : edge)
        {
            ivec2& left = left_pixels[pixel.y-min];
            ivec2& right = right_pixels[pixel.y-min];
            left.x = MIN(left.x, pixel.x);
            right.x = MAX(right.x, pixel.x);
        }
    }
}

void drawRows(const vector<ivec2>& left_pixels, const vector<ivec2>& right_pixels)
{
    for (size_t i = 0; i < left_pixels.size(); ++i)
    {
        const ivec2& left = left_pixels[i];
        const ivec2& right = right_pixels[i];
        for (int j = left.x; j <= right.x; ++j)
        {
            PutPixelSDL(screen, j, left.y, currentColour);
        }
    }
}

void drawPolygon(const vector<vec3>& vertices)
{
    int num_vertices = vertices.size();
    vector<ivec2> vertex_pixels(num_vertices);
    for (int i = 0; i < num_vertices; ++i)
    {
        vertexShader(vertices[i], vertex_pixels[i]);
    }

    vector<ivec2> left_pixels;
    vector<ivec2> right_pixels;
    computePolygonRows(vertex_pixels, left_pixels, right_pixels);
    drawRows(left_pixels, right_pixels);
}

pixel_t::pixel_t(int x, int y, float zinv)
{
    this->x = x;
    this->y = y;
    this->zinv = zinv;
}

pixel_t::pixel_t(const fpixel_t& p)
{
    this->x = (int) p.x;
    this->y = (int) p.y;
    this->zinv = p.zinv;
}

fpixel_t::fpixel_t(float x, float y, float zinv)
{
    this->x = x;
    this->y = y;
    this->zinv = zinv;
}

fpixel_t::fpixel_t(const pixel_t& p)
{
    this->x = (float) p.x;
    this->y = (float) p.y;
    this->zinv = p.zinv;
}

pixel_t operator-(const pixel_t& a, const pixel_t& b)
{
    return pixel_t(a.x - b.x, a.y - b.y, a.zinv - b.zinv);
}

fpixel_t operator/(const fpixel_t& a, const float f)
{
    return fpixel_t(a.x / f, a.y / f, a.zinv / f);
}

fpixel_t& operator+=(fpixel_t& a, const fpixel_t& b)
{
    a.x += b.x;
    a.y += b.y;
    a.zinv += b.zinv;
    return a;
}
