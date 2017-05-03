#pragma OPENCL EXTENSION cl_khr_fp64 : enable

kernel void pixelShader(
    global float3* positions,
    global float3* normals,
    global float3* colours,
    global float* light_depth,
    int width,
    int height,
    int light_width,
    int light_height,
    float light_focal)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
}

kernel void fxaa(global float3* colours, global float3* fxaa_colours)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
}
