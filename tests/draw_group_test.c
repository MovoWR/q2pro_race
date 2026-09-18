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
        float scale = scales[i], x = 100 * scale, y = 50 * scale, w = 40 * scale, h = 8 * scale;
        draw_group_t g = { .active = true, .x = 20, .y = -10 };
        DrawGroup_Rect(&g, scale, &x, &y, &w, &h);
        near(x / scale, 120); near(y / scale, 40);
        near(g.left, 120); near(g.right, 160); near(g.top, 40); near(g.bottom, 48);
        x = 80 * scale; y = 60 * scale; w = 20 * scale; h = 16 * scale;
        DrawGroup_Rect(&g, scale, &x, &y, &w, &h);
        near(g.left, 100); near(g.right, 160); near(g.bottom, 66);
        g = (draw_group_t) { 0 }; x = 3; y = 4; w = h = 10;
        DrawGroup_Rect(&g, scale, &x, &y, &w, &h);
        near(x, 3); near(y, 4); assert(!g.have_bounds);
    }
    /* Presentation changes screen pixels, never captured layout coordinates. */
    for (unsigned i = 0; i < sizeof(scales) / sizeof(scales[0]); i++) {
        float scale = scales[i], x = 100 * scale, y = 50 * scale, w = 40 * scale, h = 8 * scale;
        draw_group_t g = { .active = true, .x = 20, .y = -10 };
        DrawGroup_Rect(&g, scale, &x, &y, &w, &h);
        DrawTransform_Rect(264, 0, .792f, scale, &x, &y, &w, &h);
        near(x / scale, 264 + 120 * .792f);
        near(y / scale, 40 * .792f);
        near(w / scale, 40 * .792f);
        near(h / scale, 8 * .792f);
        near(g.left, 120);
        near(g.right, 160);
        near(g.bottom, 48);
        float before = x;
        DrawTransform_Rect(0, 0, 1, scale, &x, &y, &w, &h);
        near(x, before);
    }
    /* Item scale is local to its pivot; moving the item does not scale its offset.
     * Hidden groups measure exactly the same geometry, at every HUD scale. */
    const float factors[] = { .25f, .5f, 1, 1.5f, 2, 4 };
    for (unsigned i = 0; i < sizeof(scales) / sizeof(scales[0]); i++) {
        for (unsigned j = 0; j < sizeof(factors) / sizeof(factors[0]); j++) {
            for (int hidden = 0; hidden < 2; hidden++) {
                float scale = scales[i], factor = factors[j];
                float x = 100 * scale, y = 50 * scale, w = 40 * scale, h = 8 * scale;
                draw_group_t g = { .active = true, .hidden = hidden, .x = 20, .y = -10 };
                DrawGroup_SetScale(&g, factor, factor, 120, 54);
                DrawGroup_Rect(&g, scale, &x, &y, &w, &h);
                near(x / scale, 140 - 20 * factor);
                near(y / scale, 44 - 4 * factor);
                near(w / scale, 40 * factor); near(h / scale, 8 * factor);
                near(g.raw_left, 100); near(g.raw_right, 140);
                near(g.raw_top, 50); near(g.raw_bottom, 58);
                near(g.left, 140 - 20 * factor); near(g.right, 140 + 20 * factor);
                near(g.top, 44 - 4 * factor); near(g.bottom, 44 + 4 * factor);
                DrawTransform_Rect(264, 16, .792f, scale, &x, &y, &w, &h);
                near(x / scale, 264 + (140 - 20 * factor) * .792f);
                near(y / scale, 16 + (44 - 4 * factor) * .792f);
                near(w / scale, 40 * factor * .792f);
                near(h / scale, 8 * factor * .792f);
                near(g.raw_left, 100); near(g.left, 140 - 20 * factor);
            }
        }
    }
    /* Independent axes and reversed rectangles still produce ordered unions. */
    {
        float x = 100, y = 50, w = 40, h = 8;
        draw_group_t g = { .active = true, .x = 20, .y = -10 };
        DrawGroup_SetScale(&g, 1.5f, .5f, 120, 54);
        DrawGroup_Rect(&g, 1, &x, &y, &w, &h);
        near(x, 110); near(y, 42); near(w, 60); near(h, 4);
        x = 90; y = 60; w = -20; h = -20;
        DrawGroup_Rect(&g, 1, &x, &y, &w, &h);
        near(x, 95); near(y, 47); near(w, -30); near(h, -10);
        near(g.raw_left, 70); near(g.raw_right, 140);
        near(g.raw_top, 40); near(g.raw_bottom, 60);
        near(g.left, 65); near(g.right, 170);
        near(g.top, 37); near(g.bottom, 47);
    }
    /* Malformed settings never turn otherwise valid geometry into NaN/Inf. */
    const float invalid[] = { 0, -1, NAN, INFINITY, -INFINITY };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        float x = 100, y = 50, w = 40, h = 8;
        draw_group_t g = { .active = true, .x = 20, .y = -10 };
        DrawGroup_SetScale(&g, invalid[i], invalid[i], NAN, INFINITY);
        near(g.scale_x, 1); near(g.scale_y, 1);
        near(g.pivot_x, 0); near(g.pivot_y, 0);
        DrawGroup_Rect(&g, invalid[i], &x, &y, &w, &h);
        near(x, 120); near(y, 40); near(w, 40); near(h, 8);
        near(g.raw_left, 100); near(g.left, 120);
    }
    /* Setter has no drawing effect outside an active group. */
    {
        float x = 100, y = 50, w = 40, h = 8;
        draw_group_t g = { 0 };
        DrawGroup_SetScale(&g, 2, 2, 120, 54);
        DrawGroup_Rect(&g, 1, &x, &y, &w, &h);
        near(x, 100); near(y, 50); near(w, 40); near(h, 8);
        assert(!g.have_bounds);
    }
    puts("2D item transforms, raw/rendered bounds and presentation composition passed");
    return 0;
}
