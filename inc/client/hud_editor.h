/* Native HUD editor. Draft settings are visible only in the preview pass. */
#pragma once

#include "client/hud_layout.h"

typedef enum {
    HUD_EDIT_UPS, HUD_EDIT_STRAFE, HUD_EDIT_EFFICIENCY,
    HUD_EDIT_NETWORK, HUD_EDIT_LAYOUT_FIRST,
    HUD_EDIT_COUNT = HUD_EDIT_LAYOUT_FIRST + HUD_LAYOUT_MAX
} hud_edit_id_t;

#if USE_UI
void HUD_EditorInit(void);
void HUD_EditorShutdown(void);
bool HUD_EditorActive(void);
bool HUD_EditorPreview(void);
bool HUD_EditorSelected(int id);
/* Focus affects preview drawing only; bounds remain available for alignment. */
bool HUD_EditorShow(int id);
int HUD_EditorNetworkMode(void);
bool HUD_EditorKey(int key, bool down);
void HUD_EditorMouse(int x, int y);
/* Draft values are read only during preview; normal drawing reads live cvars. */
float HUD_EditorValue(const cvar_t *var);
float HUD_EditorClamp(cvar_t *var, float low, float high);
/* Union bounds in scaled HUD coordinates; ignored outside the preview pass. */
void HUD_EditorBounds(hud_edit_id_t id, float x, float y, float w, float h);
#else
#define HUD_EditorActive() false
#define HUD_EditorPreview() false
#define HUD_EditorSelected(id) false
#define HUD_EditorShow(id) true
#define HUD_EditorNetworkMode() 0
#define HUD_EditorValue(var) ((var)->value)
#define HUD_EditorClamp(var, low, high) Cvar_ClampValue(var, low, high)
#define HUD_EditorBounds(id, x, y, w, h) ((void)0)
#endif
