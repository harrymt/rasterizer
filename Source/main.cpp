#include "rasterizer.h"
#include "omp.h"

#define D2R(x) x * pi / 180

SDL_Surface* screen;
int t;
Triangle* triangles;
int num_triangles;
glm::vec3 cameraPos(0, 0, -FOCAL);
const float delta_displacement = 0.1f;

glm::vec3 lightPos(0, -0.5, -0.7);
glm::mat3 lightRot(1, 0, 0, 0, -1, 0, 0, 0, 1);

glm::vec3 lightPower = 9.f * glm::vec3(1, 1, 1);
glm::vec3 indirectLightPowerPerArea = 0.5f * glm::vec3(1, 1, 1);
glm::vec3 indirectIllumination(0.2f, 0.2f, 0.2f);

const float theta = D2R(5);

const glm::mat3 rota(cos(theta),  0, sin(theta),
                     0,           1, 0,
                     -sin(theta), 0, cos(theta));

const glm::mat3 rotc(cos(-theta),  0, sin(-theta),
                     0,            1, 0,
                     -sin(-theta), 0, cos(-theta));

glm::mat3 currentRot(1, 0, 0, 0, 1, 0, 0, 0, 1);

framedata_t frame_buffer[SCREEN_HEIGHT][SCREEN_WIDTH];
float light_buffer[LIGHT_HEIGHT][LIGHT_WIDTH];


void update()
{
    // Compute frame time:
    int t2 = SDL_GetTicks();
    float dt = (float) (t2-t);
    t = t2;
    cout << "Render time: " << dt << " ms.\n";

    Uint8* keystate = SDL_GetKeyState(0);
    if (keystate[SDLK_w])
    {
        cameraPos = cameraPos * glm::inverse(currentRot);
        cameraPos[2] += delta_displacement;
        cameraPos = cameraPos * currentRot;
    }
    if (keystate[SDLK_s])
    {
        cameraPos = cameraPos * glm::inverse(currentRot);
        cameraPos[2] -= delta_displacement;
        cameraPos = cameraPos * currentRot;
    }
    if (keystate[SDLK_d])
    {
        cameraPos = cameraPos * glm::inverse(currentRot);
        cameraPos[0] += delta_displacement;
        cameraPos = cameraPos * currentRot;
    }
    if (keystate[SDLK_a])
    {
        cameraPos = cameraPos * glm::inverse(currentRot);
        cameraPos[0] -= delta_displacement;
        cameraPos = cameraPos * currentRot;
    }
    if (keystate[SDLK_q])
    {
        currentRot = currentRot * rota;
        cameraPos = cameraPos * rota;
    }
    if (keystate[SDLK_e])
    {
        currentRot = currentRot * rotc;
        cameraPos = cameraPos * rotc;
    }
    if (keystate[SDLK_r])
    {
        cameraPos = {0, 0, -FOCAL};
        lightPos = {0, -0.5, -0.7};
        currentRot = glm::mat3(1, 0, 0, 0, 1, 0, 0, 0, 1);
    }


    if (keystate[SDLK_UP])
    {
        lightPos[2] += delta_displacement;
    }
    if (keystate[SDLK_DOWN])
    {
        lightPos[2] -= delta_displacement;
    }
    if (keystate[SDLK_RIGHT])
    {
        lightPos[0] += delta_displacement;
    }
    if (keystate[SDLK_LEFT])
    {
        lightPos[0] -= delta_displacement;
    }

    if (keystate[SDLK_PAGEUP])
    {
        lightPos[1] -= delta_displacement;
    }

    if (keystate[SDLK_PAGEDOWN])
    {
        lightPos[1] += delta_displacement;
    }
}

void vertexShader(const vertex_t& v, pixel_t& p, pixel_t& l)
{
    glm::vec3 point = (v - cameraPos) * currentRot;
    glm::vec3 lightRel = (v - lightPos) * lightRot;

    float x = point.x;
    float y = point.y;
    float z = point.z;

    p.pos3d = v;
    p.zinv = 1/z;
    p.x = (int) (FOCAL_LENGTH * x/z) + SCREEN_WIDTH / 2;
    p.y = (int) (FOCAL_LENGTH * y/z) + SCREEN_HEIGHT / 2;

    x = lightRel.x;
    y = lightRel.y;
    z = lightRel.z;
    // XXX: don't think this will be necessary!
    l.pos3d = v;
    l.zinv = 1 / z;
    l.x = (int)(FOCAL_LENGTH_LIGHT * y / z) + LIGHT_HEIGHT / 2;
    l.y = (int)(FOCAL_LENGTH_LIGHT * x / z) + LIGHT_WIDTH / 2;
}

const glm::vec3 fastNormalize(const glm::vec3 &v)
{
	const float len_sq = v.x * v.x + v.y * v.y + v.z * v.z;
	const float len_inv = sqrt(len_sq);
	return glm::vec3(v.x * len_inv, v.y * len_inv, v.z * len_inv);
}

/**
* The nearer the pixel.pos3d is to the lightPos, the brighter it should be.
*/
// XXX: Pixel shader will now run after the rest of the work is done
// This allows us to perform it on the GPU
void pixelShader(const int x, const int y)
{
    framedata_t& px = frame_buffer[y][x];
    glm::vec3 illumination;

    glm::vec3 lightRel = (px.pos - lightPos) * lightRot;
    float xl = lightRel.x;
    float yl = lightRel.y;
    float zl = lightRel.z;
    int ilx = (int)(FOCAL_LENGTH_LIGHT * xl / zl) + LIGHT_WIDTH / 2;
    int ily = (int)(FOCAL_LENGTH_LIGHT * yl / zl) + LIGHT_HEIGHT / 2;
    if (// If any of these are true, then the point is not lit by the light at all
        ilx < 0 || ilx >= LIGHT_WIDTH || ily < 0 || ily >= LIGHT_HEIGHT 
        // If the above was false, then we are within the field, but if our depth isn't
        // what the light_buffer tells us is the closest depth to the light, then we are
        // obviously occluded
        || 1 / zl < light_buffer[ily][ilx])
    {
        // There is ambient lighting in the room
        // TODO: We can make it so that cast shadows are slightly more bright than out of field shadows
        illumination = indirectIllumination;
    }
    else
    {
        glm::vec3 surfaceToLight = lightPos - px.pos;
        float r = glm::length(surfaceToLight);

        float ratio = glm::dot(glm::normalize(surfaceToLight), px.normal);
        if (ratio < 0) ratio = 0;

        glm::vec3 B = lightPower / (4 * pi * r * r);
        glm::vec3 D = B * ratio;
        illumination = D + indirectLightPowerPerArea;
    }

    px.colour = illumination * px.colour;
}


void draw()
{
    SDL_FillRect(screen, 0, 0);
    if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);

    #pragma omp parallel for
    for (int i = 0; i < SCREEN_HEIGHT; ++i)
    {
        for (int j = 0; j < SCREEN_WIDTH; ++j)
        {
            frame_buffer[i][j].depth = 0;
            frame_buffer[i][j].colour = glm::vec3(0.0f, 0.0f, 0.0f);
            if (i < LIGHT_HEIGHT && j < LIGHT_WIDTH)
            {
                light_buffer[i][j] = 0;
            }
        }
    }

    //#pragma omp parallel for
    for (int i = 0; i < num_triangles; i++)
    {
        drawPolygon(triangles[i]);
    }

    #pragma omp parallel for
    for (int i = 0; i < SCREEN_HEIGHT; ++i)
    {
        for (int j = 0; j < SCREEN_WIDTH; ++j)
        {
            pixelShader(j, i);
        }
    }

    #pragma omp parallel for
    for (int i = 0; i < SCREEN_HEIGHT; ++i)
    {
        for (int j = 0; j < SCREEN_WIDTH; ++j)
        {
            fxaa(j, i);
            PutPixelSDL(screen, j, i, frame_buffer[i][j].fxaa_colour);
        }
    }

    if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
    SDL_UpdateRect(screen, 0, 0, 0, 0);
}

float reducemul = 1.0f/8.0f;
float reducemin = 1.0f/128.0f;
float u_strength = 2.5f;
glm::vec2 u_texel(1.0f/SCREEN_WIDTH, 1.0f/SCREEN_HEIGHT);

inline glm::vec3 texture2D(float x, float y)
{
    return frame_buffer[glm::clamp((int) (glm::clamp(y, 0.0f, 1.0f)*SCREEN_HEIGHT-1), 0, SCREEN_HEIGHT-1)]
                       [glm::clamp((int) (glm::clamp(x, 0.0f, 1.0f)*SCREEN_WIDTH-1), 0, SCREEN_WIDTH-1)].colour;
}

inline glm::vec3 texture2D(glm::vec2 coords)
{
    return texture2D(coords.x, coords.y);
}

void fxaa(int x_, int y_)
{
    glm::vec2 coords((float) x_/(float)SCREEN_WIDTH, (float) y_/(float)SCREEN_HEIGHT);
    glm::vec3 centre = texture2D(coords);
    glm::vec3 nw = texture2D(coords-u_texel);
    glm::vec3 ne = texture2D(coords.x + u_texel.x, coords.y - u_texel.y);
    glm::vec3 sw = texture2D(coords.x - u_texel.x, coords.y + u_texel.y);
    glm::vec3 se = texture2D(coords+u_texel);

    glm::vec3 gray(0.299f, 0.587f, 0.114f);
    float mono_centre = glm::dot(centre, gray);
    float mono_nw = glm::dot(nw, gray);
    float mono_ne = glm::dot(ne, gray);
    float mono_sw = glm::dot(sw, gray);
    float mono_se = glm::dot(se, gray);

    float mono_min = MIN(mono_centre, MIN(mono_nw, MIN(mono_ne, MIN(mono_sw, mono_se))));
    float mono_max = MAX(mono_centre, MAX(mono_nw, MAX(mono_ne, MAX(mono_sw, mono_se))));

    glm::vec2 dir(-((mono_nw + mono_ne) - (mono_sw + mono_se)), ((mono_nw + mono_sw) - (mono_ne + mono_se)));
    float dir_reduce =  MAX((mono_nw + mono_ne + mono_sw + mono_se) * reducemul * 0.25, reducemin);
    float dir_min = 1.0f / (MIN(abs(dir.x), abs(dir.y)) + dir_reduce);
    // This can be changed to use glm min and max I think
    dir = glm::vec2(MIN(u_strength, MAX(-u_strength, dir.x * dir_min))*u_texel.x, MIN(u_strength, MAX(-u_strength, dir.y * dir_min))*u_texel.y);

    glm::vec3 resultA = 0.5f * (texture2D(coords + (-0.166667f * dir)) + texture2D(coords + 0.166667f * dir));
    glm::vec3 resultB = 0.5f * resultA + 0.25f * (texture2D(coords + (-0.5f*dir)) + texture2D(coords + 0.5f*dir));

    float mono_b = glm::dot(resultB, gray);
    frame_buffer[y_][x_].fxaa_colour = mono_b < mono_min || mono_b > mono_max ? resultA : resultB;
}

int main()
{
    screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT);
    t = SDL_GetTicks();

    // Fill triangles with test model
    std::vector<Triangle> triangles_;
    LoadTestModel(triangles_);

    num_triangles = triangles_.size();
    triangles = new Triangle[num_triangles];

    for (size_t i = 0; i < num_triangles; ++i)
    {
        triangles[i] = triangles_[i];
    }

    while (NoQuitMessageSDL())
    {
        update();
        draw();
    }

    delete[] triangles;

    SDL_SaveBMP(screen, "screenshot.bmp");
    return 0;
}
