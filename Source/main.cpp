#include "rasterizer.h"
#include "omp.h"

#define D2R(x) x*pi/180

SDL_Surface* screen;
int t;
vector<Triangle> triangles;
glm::vec3 cameraPos(0, 0, -FOCAL);
const float delta_displacement = 0.1f;

glm::vec3 lightPos(0, -0.5, -0.7);
glm::vec3 lightPower = 2.0f * glm::vec3(1, 1, 1); // P
glm::vec3 indirectLightPowerPerArea = 0.5f * glm::vec3(1, 1, 1); // D
glm::vec3 currentNormal;
glm::vec3 currentColor;
glm::vec3 currentReflectance;

const float theta = D2R(5);

const glm::mat3 rota(cos(theta),  0, sin(theta),
                     0,           1, 0,
                     -sin(theta), 0, cos(theta));

const glm::mat3 rotc(cos(-theta),  0, sin(-theta),
                     0,            1, 0,
                     -sin(-theta), 0, cos(-theta));

glm::mat3 currentRot(1, 0, 0, 0, 1, 0, 0, 0, 1);

float depth_buffer[SCREEN_HEIGHT][SCREEN_WIDTH];

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

void vertexShader(const vertex_t& v, pixel_t& p)
{
    glm::vec3 point = (v.position - cameraPos) * currentRot;

    float x = point.x;
    float y = point.y;
    float z = point.z;

    p.zinv = 1/z;
    p.x = (int) (FOCAL_LENGTH * x/z) + SCREEN_WIDTH / 2;
    p.y = (int) (FOCAL_LENGTH * y/z) + SCREEN_HEIGHT / 2;
}

const glm::vec3 fastNormalize(const glm::vec3 &v)
{
	const float len_sq = v.x * v.x + v.y * v.y + v.z * v.z;
	const float len_inv = sqrt(len_sq);
	return glm::vec3(v.x * len_inv, v.y * len_inv, v.z * len_inv);
}

void pixelShader(const pixel_t& p)
{
	// Don't calculate the shader if pixel is off screen
	if (p.x < SCREEN_WIDTH && p.x >= 0 &&
		p.y < SCREEN_HEIGHT && p.y >= 0 &&
		p.zinv > depth_buffer[p.y][p.x])
	{
		depth_buffer[p.y][p.x] = p.zinv;

		// Calculate illumination
		glm::vec3 surfaceToLight = lightPos - p.pos3d;
		float r = glm::length(surfaceToLight);
		float area = 4 * pi * r * r; // Bottom part of equation
		float ratio = glm::dot(currentNormal, glm::normalize(surfaceToLight));
		if (ratio < 0) { ratio = 0; }
		glm::vec3 b = lightPower / area;
		glm::vec3 d = b * ratio;
		glm::vec3 illumination = d + indirectLightPowerPerArea; //  (lightPower * glm::max(d, 0.0f)) * currentReflectance + indirectLightPowerPerArea;

		PutPixelSDL(screen, p.x, p.y, illumination * currentColor);
	}
}

void draw()
{
    SDL_FillRect(screen, 0, 0);
    if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);

    for (int i = 0; i < SCREEN_HEIGHT; ++i)
    {
        for (int j = 0; j < SCREEN_WIDTH; ++j)
        {
            depth_buffer[i][j] = 0;
        }
    }

    vector<vertex_t> vertices(3);

    // #pragma omp parallel for
    for (Triangle& triangle : triangles)
    {
        currentReflectance = triangle.color;
        currentNormal = triangle.normal;
        currentColor = triangle.color;
        vertices[0].position = triangle.v0;
        vertices[1].position = triangle.v1;
        vertices[2].position = triangle.v2;

        drawPolygon/*Edges*/(vertices);
    }

    if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
    SDL_UpdateRect(screen, 0, 0, 0, 0);
}


int main()
{
    screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT);
    t = SDL_GetTicks();

    // Fill triangles with test model
    LoadTestModel(triangles);

    while (NoQuitMessageSDL())
    {
        update();
        draw();
    }

    SDL_SaveBMP(screen, "screenshot.bmp");
    return 0;
}
