/* Renderer boundary model: native fixtures test group ownership and geometry. */
#pragma once

static float test_draw_scale = 1.0f;
static struct {
    bool active, hidden, measured;
    float sx, sy, px, py, x, y;
    float left, top, right, bottom;
} test_group;
static vrect_t test_group_bounds;

void R_BeginDrawGroup(float x, float y, bool hidden)
{
    assert(!test_group.active);
    memset(&test_group, 0, sizeof(test_group));
    test_group.active = true;
    test_group.hidden = hidden;
    test_group.sx = test_group.sy = 1;
    test_group.x = x;
    test_group.y = y;
}

void R_SetDrawGroupScale(float sx, float sy, float px, float py)
{
    assert(test_group.active);
    test_group.sx = sx;
    test_group.sy = sy;
    test_group.px = px;
    test_group.py = py;
}

static bool Test_GroupRect(float x, float y, float width, float height)
{
    if (!test_group.active || width <= 0 || height <= 0)
        return !test_group.active || !test_group.hidden;
    x = test_group.px + (x / test_draw_scale - test_group.px) * test_group.sx + test_group.x;
    y = test_group.py + (y / test_draw_scale - test_group.py) * test_group.sy + test_group.y;
    width = width / test_draw_scale * test_group.sx;
    height = height / test_draw_scale * test_group.sy;
    if (!test_group.measured) {
        test_group.left = x;
        test_group.top = y;
        test_group.right = x + width;
        test_group.bottom = y + height;
    } else {
        test_group.left = min(test_group.left, x);
        test_group.top = min(test_group.top, y);
        test_group.right = max(test_group.right, x + width);
        test_group.bottom = max(test_group.bottom, y + height);
    }
    test_group.measured = true;
    return !test_group.hidden;
}

bool R_EndDrawGroup(vrect_t *bounds)
{
    assert(test_group.active);
    test_group.active = false;
    if (!test_group.measured)
        return false;
    test_group_bounds = (vrect_t) {
        (int)floorf(test_group.left), (int)floorf(test_group.top),
        (int)ceilf(test_group.right) - (int)floorf(test_group.left),
        (int)ceilf(test_group.bottom) - (int)floorf(test_group.top)
    };
    if (bounds)
        *bounds = test_group_bounds;
    return true;
}

static bool Test_GroupText(int x, int y, int flags, const char *text)
{
    const int width = strlen(text) * CHAR_WIDTH;
    if (flags & UI_CENTER) x -= width / 2;
    else if (flags & UI_RIGHT) x -= width;
    return Test_GroupRect(x, y, width, CHAR_HEIGHT);
}
