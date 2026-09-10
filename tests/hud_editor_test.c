#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>
#include <stdio.h>
#include "../src/client/ui/hud_editor_math.h"

static void near(float actual, float expected)
{
    assert(fabsf(actual - expected) < 0.001f);
}

int main(void)
{
    const float widths[] = { 320, 640, 960, 1280, 1920, 2560 };
    for (unsigned i = 0; i < sizeof(widths) / sizeof(widths[0]); i++) {
        float width = widths[i], size = 48;
        near(HudEdit_EdgeOffset(width - size, width, size, -1), -1);
        near(HudEdit_EdgeOffset(width - size - 25, width, size, -1), -26);
        near(HudEdit_EdgeOffset(25, width, size, 0), 25);
        near(HudEdit_Snap((width - size) / 2 + 3, size, width, 4), (width - size) / 2);
        near(HudEdit_Snap(width - size - 2, size, width, 4), width - size);
        near(HudEdit_Snap(2, size, width, 4), 0);
        near(HudEdit_Snap(20, size, width, 4), 20);
        near(HudEdit_Snap(-100, size, width, 4), 0);
        near(HudEdit_Snap(width + 100, size, width, 4), width - size);
        /* Signed anchors round-trip across resolution changes. */
        float offset = HudEdit_EdgeOffset(width - size - 10, width, size, -1);
        near((width * 2) + offset - size + 1, width * 2 - size - 10);
    }
    near(HudEdit_Snap(10, 700, 640, 4), 0);
    near(HudEdit_EdgeOffset(20, 640, 700, 0), 0);
    near(HudEdit_EfficiencyOffset(120, 4, 100, 15), 5);
    near(HudEdit_EfficiencyOffset(90, 4, 100, 15), -6);
    near(HudEdit_EfficiencyOffset(103, 4, 100, 15), 0);
    near(HudEdit_Clamp(NAN, 0, 1), 0);
    near(HudEdit_Clamp(INFINITY, 0, 1), 0);
    hud_edit_snap_axis_t axis; float guide;
    HudEdit_SnapAxisInit(&axis, 103, 20, 640, 4);
    HudEdit_SnapAxisTarget(&axis, 100, 40);
    near(HudEdit_SnapAxisResult(&axis, &guide), 100); near(guide, 100);
    HudEdit_SnapAxisTarget(&axis, 102, 40); /* globally nearer target wins */
    near(HudEdit_SnapAxisResult(&axis, &guide), 102); near(guide, 102);
    HudEdit_SnapAxisInit(&axis, 81, 20, 640, 4);
    HudEdit_SnapAxisTarget(&axis, 100, 40); /* own right meets other's left */
    near(HudEdit_SnapAxisResult(&axis, &guide), 80); near(guide, 100);
    HudEdit_SnapAxisInit(&axis, 111, 20, 640, 4);
    HudEdit_SnapAxisTarget(&axis, 100, 40); /* center meets center */
    near(HudEdit_SnapAxisResult(&axis, &guide), 110); near(guide, 120);
    HudEdit_SnapAxisInit(&axis, 312, 20, 640, 4);
    HudEdit_SnapAxisTarget(&axis, 314, 0); /* tie keeps screen center */
    near(HudEdit_SnapAxisResult(&axis, &guide), 310); near(guide, 320);
    HudEdit_SnapAxisInit(&axis, -50, 20, 640, 4);
    near(HudEdit_SnapAxisResult(&axis, &guide), 0); assert(isnan(guide));
    HudEdit_SnapAxisInit(&axis, 10, 700, 640, 4);
    HudEdit_SnapAxisTarget(&axis, 10, 20);
    near(HudEdit_SnapAxisResult(&axis, &guide), 0); assert(isnan(guide));
    HudEdit_SnapAxisInit(&axis, 619, 20, 640, 4);
    HudEdit_SnapAxisTarget(&axis, 639, 40); /* never move off screen */
    assert(HudEdit_SnapAxisResult(&axis, &guide) <= 620);
    puts("hud-editor layout tests passed");
    return 0;
}
