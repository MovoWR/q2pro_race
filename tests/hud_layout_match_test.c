#ifdef NDEBUG
#undef NDEBUG
#endif

#include "shared/shared.h"
#include "client/hud_layout.h"
#include "../src/client/hud_layout_match.h"
#include "../src/client/hud_layout_server.h"
#include "fixtures/server_statusbar.h"
#include <assert.h>
static hud_layout_token_t tokens[HUD_LAYOUT_TOKEN_MAX];

static int match(const char *s)
{
    return HudLayout_Match(s, server_statusbar_layout,
                           q_countof(server_statusbar_layout),
                           tokens, q_countof(tokens));
}

int main(void)
{
    int count = match(server_statusbar_fixture);
    assert(count > 100);
    bool groups[HL_SERVER_COUNT] = { false };
    int timer_labels = 0, fps_labels = 0;
    for (int i = 0; i < count; i++) {
        groups[tokens[i].group] = true;
        const char *p = server_statusbar_fixture + tokens[i].start;
        if (!strncmp(p, "string2", 7) && tokens[i].group == HL_TIMER) timer_labels++;
        if (!strncmp(p, "string2", 7) && tokens[i].group == HL_SERVER_FPS) fps_labels++;
    }
    for (int i = 0; i < HL_SERVER_COUNT; i++) assert(groups[i]);
    assert(timer_labels == 2 && fps_labels == 1);
    char changed[HUD_LAYOUT_TEXT_MAX];
    memcpy(changed, server_statusbar_fixture, sizeof(server_statusbar_fixture));
    char *p = strstr(changed, "num 4 17");
    assert(p);
    p[7] = '8';
    assert(!match(changed));
    memcpy(changed, server_statusbar_fixture, sizeof(server_statusbar_fixture));
    static const char suffix[] = " xl 0 yt 0 string extra";
    memcpy(changed + sizeof(server_statusbar_fixture) - 1, suffix, sizeof(suffix));
    assert(!match(changed));
    memcpy(changed, server_statusbar_fixture, sizeof(server_statusbar_fixture));
    changed[80] = 0;
    assert(!match(changed));
    assert(!match("xl 0 yt 0 num 3 23"));
    assert(!match(""));
    memcpy(changed, server_statusbar_fixture, sizeof(server_statusbar_fixture));
    for (p = changed; *p; p++) {
        if (*p == ' ') {
            *p = '\t';
        }
    }
    assert(match(changed));
    p = strstr(changed, "current_map");
    assert(p);
    memset(p, 'x', strlen("current_map"));
    assert(match(changed));
    const hud_layout_fragment_t tiny[] = { { "string @map", 0 } };
    assert(!HudLayout_Match("string \"\"", tiny, 1, tokens, 1));
    assert(HudLayout_Match("string \"\"", tiny, 1, tokens, 2) == 2);
    assert(!HudLayout_Match("string \"unterminated", tiny, 1, tokens, 2));
    puts("Server HUD: 19 groups, labels, whitespace and fallback passed");
    return 0;
}
