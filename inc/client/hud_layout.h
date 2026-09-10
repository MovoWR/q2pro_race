/* Client-owned placement of existing HUD drawing groups. */
#pragma once

#define HUD_LAYOUT_MAX 128

typedef enum {
    HL_HEALTH, HL_ITEM, HL_SPEED, HL_TARGET, HL_TIMER, HL_INPUTS, HL_SERVER_FPS,
    HL_VOTE, HL_MAPCOUNT, HL_STATUS1, HL_STATUS2, HL_STATUS3, HL_STATUS4,
    HL_MAP, HL_PREVMAP1, HL_PREVMAP2, HL_PREVMAP3, HL_ADDEDTIME, HL_TIMELEFT,
    HL_SERVER_COUNT,
    HL_CROSSHAIR = HL_SERVER_COUNT, HL_HITMARKER, HL_CHAT, HL_CENTER,
    HL_NOTIFY, HL_MESSAGE, HL_NETALERT, HL_NETICON, HL_TURTLE, HL_PAUSE,
    HL_INVENTORY, HL_SCOREBOARD, HL_OTHER_STATUS, HL_DEMO, HL_LOADING,
    HL_DEBUGGRAPH, HL_NERDSTATS, HL_RENDER_FPS, HL_MOVE_FPS,
    HL_DEBUG_STATS, HL_DEBUG_PMOVE,
    HL_BUILTIN_COUNT
} hud_layout_id_t;

/* Keep runtime IDs/settings stable; these groups are excluded only from editing. */
static inline bool HUD_LayoutEditable(int id)
{
    switch (id) {
    case HL_CROSSHAIR:
    case HL_HITMARKER:
    case HL_DEBUG_PMOVE:
    case HL_DEBUG_STATS:
    case HL_NERDSTATS:
    case HL_LOADING:
    case HL_DEMO:
    case HL_PAUSE:
        return false;
    default:
        return true;
    }
}

typedef struct {
    const char *key, *name;
    cvar_t *x, *y, *visible;
} hud_layout_item_t;

void HUD_LayoutInit(void);
void HUD_LayoutFrame(void);
int HUD_LayoutCount(void);
const hud_layout_item_t *HUD_LayoutItem(int id);
int HUD_LayoutObject(const char *name);
/* Begin closes the previous group. Groups do not nest; saved offsets use
 * scaled HUD coordinates. End stores unshifted gameplay bounds or reports
 * translated preview bounds to the editor, then clears the active group.
 */
void HUD_LayoutBegin(int id);
void HUD_LayoutEnd(void);
void HUD_LayoutPreview(void);
bool HUD_LayoutMatch(const char *layout);
int HUD_LayoutToken(const char *layout, const char *position);
void SCR_HudEditorPrepare(void);
/* Supply unshifted bounds in scaled HUD coordinates for the current viewport. */
void HUD_LayoutSetBounds(int id, vrect_t bounds);
