#include "raytracer.h"
#include "omp.h"

#define D2R(x) x*pi/180

SDL_Surface* screen;
int t;
vector<Triangle> triangles;
vec3 cameraPos(0, 0, -FOCAL);
const float delta_displacement = 0.1f;
glm::vec3 indirectLight = 0.5f * glm::vec3(1, 1, 1);
glm::vec3 lightPos(0, -0.5, -0.7);
glm::vec3 lightColor = 14.f * glm::vec3(1, 1, 1);

const float theta = D2R(5);

const mat3 rota(cos(theta),  0, sin(theta),
                0,           1, 0,
                -sin(theta), 0, cos(theta));

const mat3 rotc(cos(-theta),  0, sin(-theta),
                0,            1, 0,
                -sin(-theta), 0, cos(-theta));

mat3 currentRot(1, 0, 0, 0, 1, 0, 0, 0, 1);

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
        currentRot = mat3(1, 0, 0, 0, 1, 0, 0, 0, 1);
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

void vertexShader(const glm::vec3& v, glm::ivec2& p)
{
	vec3 point = cameraPos - v;

	float x = point.x;
	float y = point.y;
	float z = point.z;

  p.x = FOCAL_LENGTH * x/z + SCREEN_WIDTH / 2;
  p.y = FOCAL_LENGTH * y/z + SCREEN_HEIGHT / 2;
}

void draw()
{
    SDL_FillRect(screen, 0, 0);
    if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);

    // #pragma omp parallel for
    for(unsigned int i = 0; i < triangles.size(); ++i)
    {
        vector<vec3> vertices(3);
        vertices[0] = triangles[i].v0;
        vertices[1] = triangles[i].v1;
        vertices[2] = triangles[i].v2;

        for(int v = 0; v < 3; ++v)
        {
            glm::ivec2 projPos;
            vertexShader(vertices[v], projPos);
            vec3 color(1,1,1);
            PutPixelSDL(screen, projPos.x, projPos.y, color);
        }
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
