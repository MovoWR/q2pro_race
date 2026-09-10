/* Single, non-nesting 2D quad translation and bounds in physical pixels.
 * Bounds include the offset before clipping, even when emission is hidden.
 */
#pragma once
#include <stdbool.h>
#include <math.h>

typedef struct {
    bool active, hidden, have_bounds;
    float x, y, left, top, right, bottom;
} draw_group_t;

static inline void DrawGroup_Rect(draw_group_t *g, float scale,
                                   float *x, float *y, float w, float h)
{
    if (!g->active)
        return;
    if (scale <= 0)
        scale = 1;
    *x += g->x * scale;
    *y += g->y * scale;
    float l = fminf(*x, *x + w) / scale, r = fmaxf(*x, *x + w) / scale;
    float t = fminf(*y, *y + h) / scale, b = fmaxf(*y, *y + h) / scale;
    if (!g->have_bounds) {
        g->left = l;
        g->top = t;
        g->right = r;
        g->bottom = b;
    } else {
        g->left = fminf(g->left, l);
        g->top = fminf(g->top, t);
        g->right = fmaxf(g->right, r);
        g->bottom = fmaxf(g->bottom, b);
    }
    g->have_bounds = true;
}
