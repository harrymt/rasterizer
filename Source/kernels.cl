// Pixel Shader Constants
#define INDIRECT_ILLUMINATION (float3)(0.2f)
#define LIGHT_POWER (float3)(16.0f)
#define INDIRECT_LIGHT_POWER_PER_AREA (float3)(0.5f)

// FXAA Constants
#define REDUCE_MUL 1.0f/8.0f
#define REDUCE_MIN 1.0f/128.0f
#define U_STRENGTH 2.5f

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
    float3 lpos = light_positions[idx];
    float lx = lpos.x;
    float ly = lpos.y;
    float lz = lpos.z;
    int ilx = (int)(light_focal * lx / lz) + light_width / 2;
    int ily = (int)(light_focal * ly / lz) + light_height / 2;

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

inline float3 __OVERLOADABLE__ f_texture2D(global float3* texture, int width, int height, float x, float y)
{
    return texture[clamp((int) (clamp(y, 0.0f, 1.0f)*height-1), 0, height-1)*width +
                   clamp((int) (clamp(x, 0.0f, 1.0f)*width-1), 0, width-1)];
}

inline float3 __OVERLOADABLE__ f_texture2D(global float3* texture, int width, int height, float2 coords)
{
    return f_texture2D(texture, width, height, coords.x, coords.y);
}

#define vtexture2D(texture, coords) f_texture2D(texture, width, height, coords)
#define texture2D(texture, x, y) f_texture2D(texture, width, height, x, y)

kernel void fxaa(
    global float3* colours,
    global float3* fxaa_colours,
    int width,
    int height,
    float2 u_texel)
{
    float x = (float) get_global_id(0)/ (float) width;
    float y = (float) get_global_id(1)/ (float) height;
    float2 coords = (float2)(x, y);

    float3 centre = vtexture2D(colours, coords);
    float3 nw = vtexture2D(colours, coords-u_texel);
    float3 ne = texture2D(colours, x + u_texel.x, y - u_texel.y);
    float3 sw = texture2D(colours, x - u_texel.x, y + u_texel.y);
    float3 se = vtexture2D(colours, coords+u_texel);

    float3 gray = (float3)(0.299f, 0.587f, 0.114f);
    float mono_centre = dot(centre, gray);
    float mono_nw = dot(nw, gray);
    float mono_ne = dot(ne, gray);
    float mono_sw = dot(sw, gray);
    float mono_se = dot(se, gray);

    float mono_min = min(mono_centre, min(mono_nw, min(mono_ne, min(mono_sw, mono_se))));
    float mono_max = max(mono_centre, max(mono_nw, max(mono_ne, max(mono_sw, mono_se))));

    float2 dir = (float2)(-((mono_nw + mono_ne) - (mono_sw + mono_se)), ((mono_nw + mono_sw) - (mono_ne + mono_se)));
    float dir_reduce = max((mono_nw + mono_ne + mono_sw + mono_se) * REDUCE_MUL * 0.25f, REDUCE_MIN);
    float dir_min = 1.0f / (min(fabs(dir.x), fabs(dir.y)) + dir_reduce);
    dir = min((float2)(U_STRENGTH), max((float2)(-U_STRENGTH), dir * dir_min)) * u_texel;

    float3 resultA = 0.5f * (vtexture2D(colours, coords + (-0.166667f * dir)) + vtexture2D(colours, coords + 0.166667f * dir));
    float3 resultB = 0.5f * resultA + 0.25f * (vtexture2D(colours, coords + (-0.5f*dir)) + vtexture2D(colours, coords + 0.5f*dir));

    float mono_b = dot(resultB, gray);

    fxaa_colours[get_global_id(1) * width + get_global_id(0)] = mono_b < mono_min || mono_b > mono_max ? resultA : resultB;
}
