#include "rasterizer.h"

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
    this->pos3d = p.pos3d;
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
    this->pos3d = p.pos3d;
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
