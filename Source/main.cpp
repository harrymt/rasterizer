#include "rasterizer.h"
#include "omp.h"

#define D2R(x) x * pi / 180

SDL_Surface* screen;
int t;
Triangle* triangles;
int num_triangles;
glm::vec3 cameraPos(0, 0, -FOCAL);
const float delta_displacement = 0.1f;

glm::vec3 lightPos(0, 0, -FOCAL_LIGHT);// (0, -FOCAL_LIGHT, 0);
glm::mat3 lightRot(1, 0, 0, 0, 1, 0, 0, 0, 1);

//(1, 0, 0, 0, 0, -1, 0, 1, 0);

glm::vec3 lightPower = 4.f * glm::vec3(1, 1, 1);
glm::vec3 indirectLightPowerPerArea = 0.5f * glm::vec3(1, 1, 1);
glm::vec3 indirectIllumination(0.2f, 0.2f, 0.2f);

#ifdef OPEN_CL
ocl_t ocl;
#endif

const float theta = D2R(5);

const glm::mat3 rota(cos(theta),  0, sin(theta),
                     0,           1, 0,
                     -sin(theta), 0, cos(theta));

const glm::mat3 rotc(cos(-theta),  0, sin(-theta),
                     0,            1, 0,
                     -sin(-theta), 0, cos(-theta));

glm::mat3 currentRot(1, 0, 0, 0, 1, 0, 0, 0, 1);

//framedata_t frame_buffer[SCREEN_HEIGHT][SCREEN_WIDTH];
framebuffer_t frame_buffer;
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
    if (keystate[SDLK_t])
    {
        cameraPos = cameraPos * glm::inverse(currentRot);
        cameraPos[1] -= delta_displacement;
        cameraPos = cameraPos * currentRot;
    }
    if (keystate[SDLK_y])
    {
        cameraPos = cameraPos * glm::inverse(currentRot);
        cameraPos[1] += delta_displacement;
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
    p.lzinv = 1 / z;
    p.lx = (int)(FOCAL_LENGTH_LIGHT * x / z) + LIGHT_HEIGHT / 2;
    p.ly = (int)(FOCAL_LENGTH_LIGHT * y / z) + LIGHT_WIDTH / 2;
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
void pixelShader(const int x, const int y)
{
    glm::vec3 illumination;

    float xl = frame_buffer.light_positions[y][x].x;
    float yl = frame_buffer.light_positions[y][x].y;
    float zl = frame_buffer.light_positions[y][x].z;
    int ilx = (int)(FOCAL_LENGTH_LIGHT * xl / zl) + LIGHT_WIDTH / 2;
    int ily = (int)(FOCAL_LENGTH_LIGHT * yl / zl) + LIGHT_HEIGHT / 2;
    if (// If any of these are true, then the point is not lit by the light at all
        ilx < 0 || ilx >= LIGHT_WIDTH || ily < 0 || ily >= LIGHT_HEIGHT  || zl < 0
        // If the above was false, then we are within the field, but if our depth isn't
        // what the light_buffer tells us is the closest depth to the light, then we are
        // obviously occluded
        || 1/zl < light_buffer[ily][ilx] - 0.005f
        )
    {
        // There is ambient lighting in the room
        // TODO: We can make it so that cast shadows are slightly more bright than out of field shadows
        illumination = indirectIllumination;
    }
    else
    {
        glm::vec3 surfaceToLight = lightPos - fromFloat3(&frame_buffer.positions[y][x]);
        float r = glm::length(surfaceToLight);

        float ratio = glm::dot(glm::normalize(surfaceToLight), fromFloat3(&frame_buffer.normals[y][x]));
        if (ratio < 0) ratio = 0;

        glm::vec3 B = lightPower / (4 * pi * r * r);
        glm::vec3 D = B * ratio;
        illumination = D + indirectLightPowerPerArea;
    }

    frame_buffer.colours[y][x] = toFloat3(illumination * fromFloat3(&frame_buffer.colours[y][x]));
}

#ifdef OPEN_CL
void checkError(cl_int err, const char* op, const int line);
void initialiseCl();
void finaliseCl();
#endif

void draw()
{
#ifdef OPEN_CL
    cl_int err;
#endif
    SDL_FillRect(screen, 0, 0);
    if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);

    #pragma omp parallel for
    for (int i = 0; i < SCREEN_HEIGHT; ++i)
    {
        for (int j = 0; j < SCREEN_WIDTH; ++j)
        {
            frame_buffer.depths[i][j] = 0;
            frame_buffer.colours[i][j] = toFloat3(glm::vec3(0.0f, 0.0f, 0.0f));
            if (i < LIGHT_HEIGHT && j < LIGHT_WIDTH)
            {
                light_buffer[i][j] = 0;
            }
        }
    }

    #pragma omp parallel for
    for (int i = 0; i < num_triangles; i++)
    {
        drawPolygon(triangles[i]);
    }

#ifdef OPEN_CL
    err = clEnqueueWriteBuffer(ocl.queue, ocl.depths, CL_TRUE, 0, sizeof(cl_float) * SCREEN_WIDTH * SCREEN_HEIGHT, frame_buffer.depths, 0, nullptr, nullptr);
    checkError(err, "writing depth data", __LINE__);
    err = clEnqueueWriteBuffer(ocl.queue, ocl.light_depths, CL_TRUE, 0, sizeof(cl_float) * LIGHT_WIDTH * LIGHT_HEIGHT, light_buffer, 0, nullptr, nullptr);
    checkError(err, "writing light depth buffer data", __LINE__);
    err = clEnqueueWriteBuffer(ocl.queue, ocl.normals, CL_TRUE, 0, sizeof(cl_float3) * SCREEN_WIDTH * SCREEN_HEIGHT, frame_buffer.normals, 0, nullptr, nullptr);
    checkError(err, "writing normal data", __LINE__);
    err = clEnqueueWriteBuffer(ocl.queue, ocl.colours, CL_TRUE, 0, sizeof(cl_float3) * SCREEN_WIDTH * SCREEN_HEIGHT, frame_buffer.colours, 0, nullptr, nullptr);
    checkError(err, "writing colour data", __LINE__);
    err = clEnqueueWriteBuffer(ocl.queue, ocl.positions, CL_TRUE, 0, sizeof(cl_float3) * SCREEN_WIDTH * SCREEN_HEIGHT, frame_buffer.positions, 0, nullptr, nullptr);
    checkError(err, "writing position data", __LINE__);
    err = clEnqueueWriteBuffer(ocl.queue, ocl.light_positions, CL_TRUE, 0, sizeof(cl_float3) * SCREEN_WIDTH * SCREEN_HEIGHT, frame_buffer.light_positions, 0, nullptr, nullptr);
    checkError(err, "writing light position data", __LINE__);

    cl_int width = SCREEN_WIDTH;
    cl_int height = SCREEN_HEIGHT;
    cl_int light_width = LIGHT_WIDTH;
    cl_int light_height = LIGHT_HEIGHT;
    cl_float3 light_pos = toFloat3(lightPos);
    cl_float light_focal = FOCAL_LENGTH_LIGHT;

    err = clSetKernelArg(ocl.pixelShader, 0, sizeof(cl_mem), &ocl.positions);
    checkError(err, "setting pixelShader arg 0", __LINE__);
    err = clSetKernelArg(ocl.pixelShader, 1, sizeof(cl_mem), &ocl.normals);
    checkError(err, "setting pixelShader arg 1", __LINE__);
    err = clSetKernelArg(ocl.pixelShader, 2, sizeof(cl_mem), &ocl.colours);
    checkError(err, "setting pixelShader arg 2", __LINE__);
    err = clSetKernelArg(ocl.pixelShader, 3, sizeof(cl_mem), &ocl.light_depths);
    checkError(err, "setting pixelShader arg 3", __LINE__);
    err = clSetKernelArg(ocl.pixelShader, 4, sizeof(cl_mem), &ocl.light_positions);
    checkError(err, "setting pixelShader arg 4", __LINE__);
    err = clSetKernelArg(ocl.pixelShader, 5, sizeof(cl_float), &light_focal);
    checkError(err, "setting pixelShader arg 5", __LINE__);
    err = clSetKernelArg(ocl.pixelShader, 6, sizeof(cl_int), &width);
    checkError(err, "setting pixelShader arg 6", __LINE__);
    err = clSetKernelArg(ocl.pixelShader, 7, sizeof(cl_int), &height);
    checkError(err, "setting pixelShader arg 7", __LINE__);
    err = clSetKernelArg(ocl.pixelShader, 8, sizeof(cl_int), &light_width);
    checkError(err, "setting pixelShader arg 8", __LINE__);
    err = clSetKernelArg(ocl.pixelShader, 9, sizeof(cl_int), &light_height);
    checkError(err, "setting pixelShader arg 9", __LINE__);
    err = clSetKernelArg(ocl.pixelShader, 10, sizeof(cl_float3), &light_pos);
    checkError(err, "setting pixelShader arg 10", __LINE__);

    size_t global[2] = { width, height };
    err = clEnqueueNDRangeKernel(ocl.queue, ocl.pixelShader, 2, nullptr, global, nullptr, 0, nullptr, nullptr);
    checkError(err, "enqueueing pixelShader kernel", __LINE__);
    clFinish(ocl.queue);

    err = clEnqueueReadBuffer(ocl.queue, ocl.colours, CL_TRUE, 0, sizeof(cl_float3) * SCREEN_WIDTH * SCREEN_HEIGHT, frame_buffer.colours, 0, nullptr, nullptr);
    //err = clEnqueueReadBuffer(ocl.queue, ocl.fxaa_colours, CL_TRUE, 0, sizeof(cl_float3) * SCREEN_WIDTH * SCREEN_HEIGHT, frame_buffer.colours, 0, nullptr, nullptr);
    checkError(err, "reading frame data", __LINE__);
#else
    #pragma omp parallel for
    for (int i = 0; i < SCREEN_HEIGHT; ++i)
    {
        for (int j = 0; j < SCREEN_WIDTH; ++j)
        {
            pixelShader(j, i);
        }
    }
#endif

    #pragma omp parallel for
    for (int i = 0; i < SCREEN_HEIGHT; ++i)
    {
        for (int j = 0; j < SCREEN_WIDTH; ++j)
        {
            //fxaa(j, i);
            PutPixelSDL(screen, j, i, fromFloat3(&frame_buffer.colours[i][j]));
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
    return fromFloat3(&frame_buffer.colours[glm::clamp((int) (glm::clamp(y, 0.0f, 1.0f)*SCREEN_HEIGHT-1), 0, SCREEN_HEIGHT-1)]
                                           [glm::clamp((int) (glm::clamp(x, 0.0f, 1.0f)*SCREEN_WIDTH-1), 0, SCREEN_WIDTH-1)]);
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
    frame_buffer.fxaa_colours[y_][x_] = toFloat3(mono_b < mono_min || mono_b > mono_max ? resultA : resultB);
}

int main()
{
#ifdef OPEN_CL
    initialiseCl();
#endif
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
#ifdef OPEN_CL
    finaliseCl();
#endif
    return 0;
}

#ifdef OPEN_CL
cl_device_id selectOpenCLDevice();
void die(const char* message, const int line, const char* file);

void initialiseCl()
{
    cl_int err;
    ocl.device = selectOpenCLDevice();
    ocl.context = clCreateContext(nullptr, 1, &ocl.device, nullptr, nullptr, &err);
    checkError(err, "creating context", __LINE__);

    FILE* fp;
#ifdef _MSC_VER
    fopen_s(&fp, "kernels.cl", "r"); //TODO: LINUX!
#else
    fp = fopen("kernels.cl", "r");
#endif
    if (!fp)
    {
        char message[1024];
        printf(message, "could not open OpenCL kernel file: kernels.cl");
        die(message, __LINE__, __FILE__);
    }

    ocl.queue = clCreateCommandQueue(ocl.context, ocl.device, 0, &err);
    checkError(err, "creating command queue", __LINE__);

    fseek(fp, 0, SEEK_END);
    long ocl_size = ftell(fp) + 1;
    char* ocl_src = (char*)calloc(ocl_size, sizeof(char));
    memset(ocl_src, 0, ocl_size);
    fseek(fp, 0, SEEK_SET);
    fread(ocl_src, 1, ocl_size, fp);
    fclose(fp);

    ocl.program = clCreateProgramWithSource(ocl.context, 1, (const char**)&ocl_src, nullptr, &err);
    free(ocl_src);
    checkError(err, "creating program", __LINE__);

    ocl.work_group_size = GROUP_SIZE;
    ocl.num_work_groups = (SCREEN_WIDTH * SCREEN_HEIGHT) / ocl.work_group_size;

    std::stringstream flags;
    flags << "-D NUM_GROUPS=" << ocl.work_group_size << " -D GROUP_SIZE=" << ocl.num_work_groups;
    err = clBuildProgram(ocl.program, 1, &ocl.device, flags.str().c_str(), nullptr, nullptr);
    if (err == CL_BUILD_PROGRAM_FAILURE)
    {
        size_t sz;
        clGetProgramBuildInfo(ocl.program, ocl.device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &sz);
        char* buildlog = (char*)calloc(sz, sizeof(char));
        clGetProgramBuildInfo(ocl.program, ocl.device, CL_PROGRAM_BUILD_LOG, sz, buildlog, nullptr);
        std::cout << "\nOpenCL build log : \n\n" << buildlog << std::endl;
        free(buildlog);
    }
    checkError(err, "building program", __LINE__);

    ocl.pixelShader = clCreateKernel(ocl.program, "pixelShader", &err);
    checkError(err, "creating pixelShader kernel", __LINE__);
    ocl.fxaa = clCreateKernel(ocl.program, "fxaa", &err);
    checkError(err, "creating fxaa kernel", __LINE__);

    ocl.depths = clCreateBuffer(ocl.context, CL_MEM_READ_ONLY, sizeof(cl_float) * SCREEN_HEIGHT * SCREEN_WIDTH, nullptr, &err);
    checkError(err, "creating depth buffer", __LINE__);
    ocl.normals = clCreateBuffer(ocl.context, CL_MEM_READ_ONLY, sizeof(cl_float3) * SCREEN_HEIGHT * SCREEN_WIDTH, nullptr, &err);
    checkError(err, "creating normal buffer", __LINE__);
    ocl.colours = clCreateBuffer(ocl.context, CL_MEM_READ_WRITE, sizeof(cl_float3) * SCREEN_HEIGHT * SCREEN_WIDTH, nullptr, &err);
    checkError(err, "creating colour buffer", __LINE__);
    ocl.fxaa_colours = clCreateBuffer(ocl.context, CL_MEM_WRITE_ONLY, sizeof(cl_float3) * SCREEN_HEIGHT * SCREEN_WIDTH, nullptr, &err);
    checkError(err, "creating fxaa colour buffer", __LINE__);
    ocl.positions = clCreateBuffer(ocl.context, CL_MEM_READ_ONLY, sizeof(cl_float3) * SCREEN_HEIGHT * SCREEN_WIDTH, nullptr, &err);
    checkError(err, "creating position buffer", __LINE__);
    ocl.light_positions = clCreateBuffer(ocl.context, CL_MEM_READ_ONLY, sizeof(cl_float3) * SCREEN_HEIGHT * SCREEN_WIDTH, nullptr, &err);
    checkError(err, "creating light position buffer", __LINE__);
    ocl.light_depths = clCreateBuffer(ocl.context, CL_MEM_READ_ONLY, sizeof(cl_float) * LIGHT_HEIGHT * LIGHT_WIDTH, nullptr, &err);
    checkError(err, "creating light depth buffer", __LINE__);
}

void finaliseCl()
{
    clReleaseMemObject(ocl.depths);
    clReleaseMemObject(ocl.normals);
    clReleaseMemObject(ocl.colours);
    clReleaseMemObject(ocl.fxaa_colours);
    clReleaseMemObject(ocl.positions);
    clReleaseMemObject(ocl.light_positions);
    clReleaseMemObject(ocl.light_depths);
    clReleaseKernel(ocl.pixelShader);
    clReleaseKernel(ocl.fxaa);
    clReleaseProgram(ocl.program);
    clReleaseCommandQueue(ocl.queue);
    clReleaseContext(ocl.context);
}

#define MAX_DEVICES 4
#define MAX_DEVICE_NAME 100
cl_device_id selectOpenCLDevice()
{
    cl_int err;
    cl_uint num_platforms = 0;
    cl_uint total_devices = 0;
    cl_platform_id platforms[8];
    cl_device_id devices[MAX_DEVICES];
    char name[MAX_DEVICE_NAME];

    // Get list of platforms
    err = clGetPlatformIDs(8, platforms, &num_platforms);
    checkError(err, "getting platforms", __LINE__);

    // Get list of devices
    for (cl_uint p = 0; p < num_platforms; p++)
    {
        cl_uint num_devices = 0;
        err = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, MAX_DEVICES - total_devices, devices + total_devices, &num_devices);
        checkError(err, "getting device name", __LINE__);
        total_devices += num_devices;
    }

    // Print list of devices
    printf("\nAvailable OpenCL devices:\n");
    for (cl_uint d = 0; d < total_devices; d++)
    {
        clGetDeviceInfo(devices[d], CL_DEVICE_NAME, MAX_DEVICE_NAME, name, nullptr);
        printf("%2d: %s\n", d, name);
    }
    printf("\n");

    // Use first device unless OCL_DEVICE environment variable used
    cl_uint device_index = 0;
#ifdef _MSC_VER
    char* dev_env = nullptr;
    size_t sz = 0;
    _dupenv_s(&dev_env, &sz, "OCL_DEVICE");
#else
    char *dev_env = getenv("OCL_DEVICE");
#endif
    if (dev_env)
    {
        char *end;
        device_index = strtol(dev_env, &end, 10);
        if (strlen(end)) die("invalid OCL_DEVICE variable", __LINE__, __FILE__);
    }

    if (device_index >= total_devices)
    {
        fprintf(stderr, "device index set to %d but only %d devices available\n", device_index, total_devices);
        exit(1);
    }

    // Print OpenCL device name
    clGetDeviceInfo(devices[device_index], CL_DEVICE_NAME, MAX_DEVICE_NAME, name, nullptr);
    printf("Selected OpenCL device:\n-> %s (index=%d)\n\n", name, device_index);
    return devices[device_index];
}

void checkError(cl_int err, const char *op, const int line)
{
    if (err != CL_SUCCESS)
    {
        fprintf(stderr, "OpenCL error during '%s' on line %d: %d\n", op, line, err);
        fflush(stderr);
        exit(EXIT_FAILURE);
    }
}

void die(const char* message, const int line, const char* file)
{
    fprintf(stderr, "Error at line %d of file %s:\n", line, file);
    fprintf(stderr, "%s\n", message);
    fflush(stderr);
    exit(EXIT_FAILURE);
}
#endif