/* Exercise production laser allocation, parsing and drawing at runtime boundaries. */
#include "../src/client/laser.c"
#undef NDEBUG
#include <assert.h>

client_state_t cl;
tent_params_t te;
static cvar_t game_var, alpha_var, width_var, life_var, color_var;
cvar_t *fs_game = &game_var;
cvar_t *cl_race_alpha = &alpha_var;
cvar_t *cl_race_width = &width_var;
cvar_t *cl_race_life = &life_var;
cvar_t *cl_race_color = &color_var;
static entity_t drawn[8];
static int draw_count, color_calls, random_calls;
static uint32_t random_value;
static bool valid_color;

void V_AddEntity(const entity_t *ent)
{
    assert(draw_count < q_countof(drawn));
    drawn[draw_count++] = *ent;
}

bool shc_ParseColorCvar(const char *text, uint32_t *packed, color_t *color)
{
    color_calls++;
    assert(text == color_var.string);
    assert(!packed);
    if (!valid_color)
        return false;
    color->u32 = 0x80563412;
    return true;
}

int Q_strcasecmp(const char *a, const char *b)
{
    while (*a && Q_tolower(*a) == Q_tolower(*b)) { a++; b++; }
    return Q_tolower(*a) - Q_tolower(*b);
}

uint32_t Q_rand(void)
{
    random_calls++;
    return random_value;
}

static void reset(const char *game)
{
    CL_ClearLasers();
    cl.time = 1000;
    VectorSet(te.pos1, 1, 2, 3);
    VectorSet(te.pos2, 4, 5, 6);
    game_var.string = (char *)game;
    fs_game = game ? &game_var : NULL;
    alpha_var.value = 0;
    width_var.value = 12;
    life_var.value = 4000;
    color_var.string = "custom";
    draw_count = color_calls = random_calls = 0;
    random_value = 2;
    valid_color = true;
}

static void draw_at(int time)
{
    cl.time = time;
    draw_count = 0;
    CL_AddLasers();
}

static void check_entity(const entity_t *ent, int width, int color, float alpha)
{
    assert(ent->frame == width && ent->skinnum == color);
    assert(fabsf(ent->alpha - alpha) < 0.00001f);
    assert(ent->flags == (RF_TRANSLUCENT | RF_BEAM));
    assert(VectorCompare(ent->origin, te.pos1));
    assert(VectorCompare(ent->oldorigin, te.pos2));
}

static void test_ordinary_lasers(void)
{
    static const char *games[] = { "", "baseq2", "ctf", "rogue", "jumpx", NULL };
    for (int i = 0; i < q_countof(games); i++) {
        for (int j = 0; j < 4; j++) {
            reset(games[i]);
            random_value = j;
            CL_ParseLaser(0xd0d1d2d3);
            assert(color_calls == 0 && random_calls == 1);
            draw_at(1000);
            assert(draw_count == 1);
            check_entity(&drawn[0], 4, 0xd3 - j, 0.30f);
            draw_at(1050);
            assert(draw_count == 1);
            check_entity(&drawn[0], 4, 0xd3 - j, 0.30f);
            draw_at(1099);
            assert(draw_count == 1);
            check_entity(&drawn[0], 4, 0xd3 - j, 0.30f);
            draw_at(1100);
            assert(draw_count == 0);
        }
    }

    reset("baseq2");
    CL_ParseLaser(0xd0d1d2d3);
    game_var.string = "jump";
    draw_at(1000);
    assert(draw_count == 1);
    check_entity(&drawn[0], 4, 0xd1, 0.30f);
}

static void test_race_lasers(void)
{
    reset("JuMp");
    alpha_var.value = 0.8f;
    CL_ParseLaser(0xd0d1d2d3);
    assert(color_calls == 1 && random_calls == 0);
    draw_at(1000);
    assert(draw_count == 1);
    check_entity(&drawn[0], 12, -1, 0.8f);
    assert(drawn[0].rgba.u32 == 0x80563412);
    draw_at(3000);
    assert(draw_count == 1);
    check_entity(&drawn[0], 12, -1, 0.4f);

    /* Only opacity remains dynamic after a race laser is created. */
    game_var.string = "baseq2";
    width_var.value = 3;
    life_var.value = 200;
    alpha_var.value = 2;
    draw_at(3000);
    assert(draw_count == 1);
    check_entity(&drawn[0], 12, -1, 0.5f);
    alpha_var.value = 0.2f;
    draw_at(3000);
    assert(draw_count == 1);
    check_entity(&drawn[0], 12, -1, 0.1f);
    alpha_var.value = 0;
    draw_at(3000);
    assert(draw_count == 0);
    alpha_var.value = -1;
    draw_at(3000);
    assert(draw_count == 0);
    alpha_var.value = 1;
    draw_at(5000);
    assert(draw_count == 0);
}

/* Use the same allocator contract as CL_RailCore in tent.c. */
static void add_rail_core(void)
{
    laser_t *l = CL_AllocLaser();
    assert(l);
    VectorCopy(te.pos1, l->start);
    VectorCopy(te.pos2, l->end);
    l->color = -1;
    l->lifetime = 1000;
    l->width = 2;
    l->rgba.u32 = 0xffabcdef;
}

static void test_rail_core_isolation(void)
{
    static const char *games[] = { "baseq2", "jump" };
    for (int i = 0; i < q_countof(games); i++) {
        reset(games[i]);
        add_rail_core();
        draw_at(1000);
        assert(draw_count == 1);
        check_entity(&drawn[0], 2, -1, 1);
        assert(drawn[0].rgba.u32 == 0xffabcdef);
        draw_at(1500);
        assert(draw_count == 1);
        check_entity(&drawn[0], 2, -1, 0.5f);
    }

    reset("jump");
    life_var.value = 1000;
    CL_ParseLaser(0xd0d1d2d3);
    add_rail_core();
    draw_at(1500);
    assert(draw_count == 1);
    check_entity(&drawn[0], 2, -1, 0.5f);
    alpha_var.value = 0.2f;
    draw_at(1500);
    assert(draw_count == 2);
    check_entity(&drawn[0], 12, -1, 0.1f);
    check_entity(&drawn[1], 2, -1, 0.5f);
    assert(drawn[0].rgba.u32 == 0x80563412 && drawn[1].rgba.u32 == 0xffabcdef);

    /* Reusing the expired race slot must clear its styling classification. */
    cl.time = 2000;
    alpha_var.value = 0;
    add_rail_core();
    draw_at(2000);
    assert(draw_count == 1);
    check_entity(&drawn[0], 2, -1, 1);
    CL_ClearLasers();
    draw_at(2000);
    assert(draw_count == 0);
}

static void test_race_limits(void)
{
    reset("jump");
    alpha_var.value = 2;
    width_var.value = 40;
    life_var.value = 6000;
    valid_color = false;
    CL_ParseLaser(0);
    draw_at(1000);
    assert(draw_count == 1);
    check_entity(&drawn[0], 20, -1, 1);
    assert(drawn[0].rgba.u32 == 0xff0000ff);
    draw_at(5999);
    assert(draw_count == 1);
    draw_at(6000);
    assert(draw_count == 0);

    reset("jump");
    alpha_var.value = 1;
    width_var.value = life_var.value = -2;
    CL_ParseLaser(0);
    draw_at(1000);
    assert(draw_count == 0);
}

int main(void)
{
    test_ordinary_lasers();
    test_race_lasers();
    test_rail_core_isolation();
    test_race_limits();
    puts("Laser mod scope, palette, opacity, rail isolation and lifecycle passed");
    return 0;
}
