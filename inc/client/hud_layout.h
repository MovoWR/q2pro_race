/* Client-owned placement of existing HUD drawing groups. */
#pragma once

#define HUD_LAYOUT_MAX 128
#define HUD_SCALE_MIN 0.25f
#define HUD_SCALE_MAX 4.0f

typedef enum {
    HL_HEALTH, HL_ITEM, HL_SPEED, HL_TARGET, HL_TIMER, HL_INPUTS, HL_SERVER_FPS,
    HL_VOTE, HL_MAPCOUNT, HL_STATUS1, HL_STATUS2, HL_STATUS3, HL_STATUS4,
    HL_MAP, HL_PREVMAP1, HL_PREVMAP2, HL_PREVMAP3, HL_ADDEDTIME, HL_TIMELEFT,
    HL_SERVER_COUNT,
    HL_CROSSHAIR = HL_SERVER_COUNT, HL_HITMARKER, HL_CHAT, HL_CENTER,
    HL_NOTIFY, HL_MESSAGE, HL_NETALERT, HL_NETICON, HL_TURTLE, HL_PAUSE,
    HL_INVENTORY, HL_SCOREBOARD, HL_OTHER_STATUS, HL_DEMO, HL_LOADING,
    HL_DEBUGGRAPH, HL_NERDSTATS, HL_RENDER_FPS, HL_MOVE_FPS,
    HL_DEBUG_STATS, HL_DEBUG_PMOVE, HL_BIND_REMINDERS,
    HL_BUILTIN_COUNT
} hud_layout_id_t;

/* Edit recognized jumpmod server groups and client overlays only.
 * Keep runtime IDs/settings stable; exclusions affect editing only. */
static inline bool HUD_LayoutEditable(int id)
{
    switch (id) {
    case HL_OTHER_STATUS:
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

/* Synthetic editor contexts; never modify captured server/client state. */
typedef enum {
    HUD_PREVIEW_LIVE, HUD_PREVIEW_RUNNING, HUD_PREVIEW_SPECTATING,
    HUD_PREVIEW_VOTING, HUD_PREVIEW_SCOREBOARD, HUD_PREVIEW_NETWORK,
    HUD_PREVIEW_COUNT
} hud_preview_scenario_t;

/* Persistent groups remain available in every sample context. */
static inline bool HUD_LayoutScenarioIncludes(int id, hud_preview_scenario_t scenario)
{
    if (id < 0 || id >= HUD_LAYOUT_MAX || !HUD_LayoutEditable(id))
        return false;
    switch (id) {
    case HL_TARGET:
        return scenario == HUD_PREVIEW_SPECTATING;
    case HL_VOTE:
        return scenario == HUD_PREVIEW_VOTING;
    case HL_CENTER:
        return scenario == HUD_PREVIEW_RUNNING;
    case HL_CHAT:
        return scenario == HUD_PREVIEW_RUNNING || scenario == HUD_PREVIEW_VOTING;
    case HL_SCOREBOARD:
        return scenario == HUD_PREVIEW_SCOREBOARD;
    case HL_NETALERT:
    case HL_NETICON:
    case HL_TURTLE:
    case HL_DEBUGGRAPH:
        return scenario == HUD_PREVIEW_NETWORK;
    case HL_NOTIFY:
    case HL_MESSAGE:
    case HL_INVENTORY:
        return false;
    default:
        return true;
    }
}

typedef struct {
    const char *key, *name;
    cvar_t *x, *y, *visible, *scale;
} hud_layout_item_t;

void HUD_LayoutInit(void);
void HUD_LayoutFrame(void);
int HUD_LayoutCount(void);
const hud_layout_item_t *HUD_LayoutItem(int id);
int HUD_LayoutObject(const char *name);
/* Stable screen/text anchors in scaled HUD coordinates, before saved offsets. */
void HUD_LayoutSetScaleAnchor(int id, float x, float y);
void HUD_LayoutScaleAnchor(int id, float *x, float *y);
/* Begin closes the previous group. Groups do not nest; saved offsets use
 * scaled HUD coordinates. End stores original gameplay bounds or reports
 * scaled and translated preview bounds to the editor, then clears the group.
 */
void HUD_LayoutBegin(int id);
void HUD_LayoutEnd(void);
void HUD_LayoutPreview(void);
/* Transient samples are available only while their live group is drawing. */
bool HUD_LayoutPreviewAvailable(int id);
/* True only when the live drawing pass produced bounds this frame. */
bool HUD_LayoutCaptured(int id);
bool HUD_LayoutMatch(const char *layout);
int HUD_LayoutToken(const char *layout, const char *position);
void SCR_HudEditorPrepare(void);
/* Supply unshifted bounds in scaled HUD coordinates for the current viewport. */
void HUD_LayoutSetBounds(int id, vrect_t bounds);
