/* Single, non-nesting 2D quad transform and bounds in physical pixels.
 * Bounds include the transform before clipping, even when emission is hidden.
 */
#pragma once
#include <stdbool.h>
#include <math.h>

typedef struct {
    bool active, hidden, have_bounds;
    float x, y, left, top, right, bottom;
    float scale_x, scale_y, pivot_x, pivot_y;
    float raw_left, raw_top, raw_right, raw_bottom;
} draw_group_t;

static inline void DrawGroup_SetScale(draw_group_t *g, float sx, float sy,
                                      float pivot_x, float pivot_y)
{
    g->scale_x = isfinite(sx) && sx > 0 ? sx : 1;
    g->scale_y = isfinite(sy) && sy > 0 ? sy : 1;
    g->pivot_x = isfinite(pivot_x) ? pivot_x : 0;
    g->pivot_y = isfinite(pivot_y) ? pivot_y : 0;
}

static inline void DrawGroup_Rect(draw_group_t *g, float scale,
                                   float *x, float *y, float *w, float *h)
{
    if (!g->active)
        return;
    if (!isfinite(scale) || scale <= 0)
        scale = 1;
    float raw_l = fminf(*x, *x + *w) / scale, raw_r = fmaxf(*x, *x + *w) / scale;
    float raw_t = fminf(*y, *y + *h) / scale, raw_b = fmaxf(*y, *y + *h) / scale;
    /* Zero-initialized groups retain the original translation-only behavior. */
    float sx = isfinite(g->scale_x) && g->scale_x > 0 ? g->scale_x : 1;
    float sy = isfinite(g->scale_y) && g->scale_y > 0 ? g->scale_y : 1;
    if (sx != 1) {
        *x = g->pivot_x * scale + (*x - g->pivot_x * scale) * sx;
        *w *= sx;
    }
    if (sy != 1) {
        *y = g->pivot_y * scale + (*y - g->pivot_y * scale) * sy;
        *h *= sy;
    }
    *x += g->x * scale;
    *y += g->y * scale;
    float l = fminf(*x, *x + *w) / scale, r = fmaxf(*x, *x + *w) / scale;
    float t = fminf(*y, *y + *h) / scale, b = fmaxf(*y, *y + *h) / scale;
    if (!g->have_bounds) {
        g->left = l;
        g->top = t;
        g->right = r;
        g->bottom = b;
        g->raw_left = raw_l;
        g->raw_top = raw_t;
        g->raw_right = raw_r;
        g->raw_bottom = raw_b;
    } else {
        g->left = fminf(g->left, l);
        g->top = fminf(g->top, t);
        g->right = fmaxf(g->right, r);
        g->bottom = fmaxf(g->bottom, b);
        g->raw_left = fminf(g->raw_left, raw_l);
        g->raw_top = fminf(g->raw_top, raw_t);
        g->raw_right = fmaxf(g->raw_right, raw_r);
        g->raw_bottom = fmaxf(g->raw_bottom, raw_b);
    }
    g->have_bounds = true;
}

/* Presentation transform after bounds capture. The origin tx/ty is in physical
 * pixels; quad coordinates use the caller's logical scale. Zoom is presentation only. */
static inline void DrawTransform_Rect(float tx, float ty, float zoom, float scale,
                                      float *x, float *y, float *w, float *h)
{
    *x = *x * zoom + tx * scale;
    *y = *y * zoom + ty * scale;
    *w *= zoom;
    *h *= zoom;
}
