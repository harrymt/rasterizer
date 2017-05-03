#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define INDIRECT_ILLUMINATION (float3)(0.2f, 0.2f, 0.2f)
#define LIGHT_POWER (float3)(4.0f, 4.0f, 4.0f)
#define INDIRECT_LIGHT_POWER_PER_AREA (float3)(0.5f, 0.5f, 0.5f)

kernel void pixelShader(
    global float3* positions,
    global float3* normals,
    global float3* colours,
    // This is the depth buffer of the light
    global float* light_depths,
    global float3* light_positions,
    float light_focal,
    int width,
    int height,
    int light_width,
    int light_height,
    float3 light_pos)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    int idx = y * width + x;

    float3 illumination;
    float lx = light_positions[idx].x;
    float ly = light_positions[idx].y;
    float lz = light_positions[idx].z;
    int ilx = (int)(light_focal * lx / lz) + width / 2;
    int ily = (int)(light_focal * ly / lz) + height / 2;

    // GPU's don't like branching, but hey ho...
    if (ilx < 0 || ilx >= light_width
     || ily < 0 || ily >= light_height
     || lz < 0 || 1.0f/lz < light_depths[ily * light_width + ilx] - 0.005f)
    {
        illumination = INDIRECT_ILLUMINATION;
    }
    else
    {
        float3 surfaceToLight = light_pos - positions[idx];
        float r = length(surfaceToLight);

        float ratio = fmax(dot(normalize(surfaceToLight), normals[idx]), 0);

        float3 D = LIGHT_POWER * (ratio / (4.0f * 3.14159f * r * r));
        illumination = D + INDIRECT_LIGHT_POWER_PER_AREA;
    }

    colours[idx] = illumination * colours[idx];
}

kernel void fxaa(global float3* colours, global float3* fxaa_colours)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
}
