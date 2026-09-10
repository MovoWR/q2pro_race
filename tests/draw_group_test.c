#ifdef NDEBUG
#undef NDEBUG
#endif

#include "../src/refresh/draw_group.h"
#include <assert.h>
#include <stdio.h>
static void near(float a, float b) { assert(fabsf(a - b) < .001f); }
int main(void)
{
    float scales[] = { 1, .5f, 1.f / 3, .25f };
    for (unsigned i = 0; i < sizeof(scales) / sizeof(scales[0]); i++) {
        float scale = scales[i], x = 100 * scale, y = 50 * scale;
        draw_group_t g = { .active = true, .x = 20, .y = -10 };
        DrawGroup_Rect(&g, scale, &x, &y, 40 * scale, 8 * scale);
        near(x / scale, 120); near(y / scale, 40);
        near(g.left, 120); near(g.right, 160); near(g.top, 40); near(g.bottom, 48);
        x = 80 * scale; y = 60 * scale;
        DrawGroup_Rect(&g, scale, &x, &y, 20 * scale, 16 * scale);
        near(g.left, 100); near(g.right, 160); near(g.bottom, 66);
        g = (draw_group_t) { 0 }; x = 3; y = 4;
        DrawGroup_Rect(&g, scale, &x, &y, 10, 10);
        near(x, 3); near(y, 4); assert(!g.have_bounds);
    }
    puts("2D translation, scale conversion, bounds union and reset passed");
    return 0;
}
