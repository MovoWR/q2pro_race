/* Exercise production prediction and pmove with collision and HUD boundaries stubbed. */
#include "../src/client/predict.c"
#include "../src/common/pmove/new.c"
#include "../src/common/pmove/common.c"
#include "../src/shared/shared.c"

#include <stdio.h>
#include <stdlib.h>

client_state_t cl;
client_static_t cls;
centity_t cl_entities[MAX_EDICTS];

static cvar_t predict_var, paused_var, step_var, efficiency_var;
cvar_t *cl_predict = &predict_var;
cvar_t *sv_paused = &paused_var;
cvar_t *cl_step_smoothing_mode = &step_var;
cvar_t *cl_strafehelperEfficiency = &efficiency_var;
#if USE_DEBUG
static cvar_t showmiss_var;
cvar_t *cl_showmiss = &showmiss_var;
#endif

static bsp_t empty_bsp;
static mnode_t empty_node;
static bool capturing, helper_have, slick_ground;
static int failures, begins, ends, samples, helper_clears;
static int efficiency_samples;
static float sampled_target, sampled_wishspeed, end_velocity;
static float sampled_accel, sampled_frametime;
static vec3_t sampled_forward, sampled_velocity, sampled_wishdir;
static int sampled_waterlevel, water_contents, ground_contents;
static float water_surface;

static void Check(const char *name, bool condition)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", name);
        failures++;
    }
}

void StrafeHelper_BeginPrediction(void)
{
    Check("capture is not nested", !capturing);
    capturing = true;
    helper_have = false;
    begins++;
}

void StrafeHelper_EndPrediction(void)
{
    Check("end follows begin", capturing);
    capturing = false;
    end_velocity = cl.predicted_velocity[0];
    ends++;
}

bool StrafeHelper_IsPredicting(void) { return capturing; }
void StrafeHelper_Clear(void) { helper_have = false; helper_clears++; }
void StrafeHelper_ClearEfficiency(void) {}


void StrafeHelper_SetAccelerationValues(const float forward[3],
    const float velocity[3], const float wishdir[3], float wishspeed,
    float acceleration_target, float accel, float frametime)
{
    Check("helper sample belongs to final prediction", capturing);
    helper_have = true;
    sampled_target = acceleration_target;
    sampled_wishspeed = wishspeed;
    sampled_accel = accel;
    sampled_frametime = frametime;
    VectorCopy(forward, sampled_forward);
    VectorCopy(velocity, sampled_velocity);
    VectorCopy(wishdir, sampled_wishdir);
    sampled_waterlevel = pm->waterlevel;
    samples++;
}

void StrafeHelper_SetEfficiency(const float velocity[3], const float wishdir[3],
    float target, float budget)
{
    Check("efficiency sample belongs to final prediction", capturing);
    efficiency_samples++;
}


void SH_NetMeter_PredictionError(int len) {}
void Com_LPrintf(print_type_t type, const char *fmt, ...) {}
void Com_Error(error_type_t code, const char *fmt, ...) { abort(); }

const mleaf_t *BSP_PointLeaf(const mnode_t *node, const vec3_t point)
{
    static mleaf_t leaf;
    leaf.contents[0] = leaf.contents[1] = point[2] <= water_surface ? water_contents : 0;
    return &leaf;
}

const mnode_t *CM_HeadnodeForBox(const vec3_t mins, const vec3_t maxs)
{
    return NULL;
}

int CM_TransformedPointContents(const vec3_t point, const mnode_t *headnode,
    const vec3_t origin, const vec3_t angles, bool extended)
{
    return 0;
}

void CM_BoxTrace(trace_t *trace, const vec3_t start, const vec3_t end,
    const vec3_t mins, const vec3_t maxs, const mnode_t *headnode,
    int brushmask, bool extended)
{
    memset(trace, 0, sizeof(*trace));
    trace->fraction = 1.0f;
    VectorCopy(end, trace->endpos);
    if (slick_ground && end[2] == start[2] - 0.25f) {
        static csurface_t slick_surface = { .flags = SURF_SLICK };
        trace->fraction = 0.0f;
        trace->plane.normal[2] = 1.0f;
        trace->surface = &slick_surface;
        trace->contents = ground_contents;
        VectorCopy(start, trace->endpos);
    }
}

void CM_TransformedBoxTrace(trace_t *trace, const vec3_t start, const vec3_t end,
    const vec3_t mins, const vec3_t maxs, const mnode_t *headnode,
    int brushmask, const vec3_t origin, const vec3_t angles, bool extended)
{
    CM_BoxTrace(trace, start, end, mins, maxs, headnode, brushmask, extended);
}

void CM_ClipEntity(trace_t *dst, const trace_t *src, struct edict_s *ent) {}

static void Setup(void)
{
    memset(&cl, 0, sizeof(cl));
    memset(&cls, 0, sizeof(cls));
    predict_var = (cvar_t) { .integer = 1 };
    paused_var = (cvar_t) { 0 };
    step_var = (cvar_t) { .string = "q2pro" };
    efficiency_var = (cvar_t) { .integer = 1 };
    cls.state = ca_active;
    cls.realtime = 100;
    cls.frametime = 0.008f;
    empty_bsp.nodes = &empty_node;
    cl.bsp = &empty_bsp;
    cl.frame.ps.pmove.pm_type = PM_NORMAL;
    cl.frame.ps.pmove.velocity[0] = 3200;
    PmoveInit(&cl.pmp);
    cl.cmdNumber = 3;
    for (unsigned i = 1; i <= cl.cmdNumber; i++) {
        cl.cmds[i].msec = 8;
        cl.cmds[i].forwardmove = 200;
    }
    cl.cmds[3].sidemove = 200;
    capturing = slick_ground = false;
    helper_have = true;
    begins = ends = samples = helper_clears = 0;
    efficiency_samples = 0;
    water_contents = ground_contents = sampled_waterlevel = 0;
    water_surface = 0;
    sampled_accel = sampled_frametime = 0;
    VectorClear(sampled_forward);
    VectorClear(sampled_velocity);
    VectorClear(sampled_wishdir);
    sampled_target = sampled_wishspeed = end_velocity = 0;
}

static void TestFinalCommand(void)
{
    Setup();
    CL_PredictMovement();
    Check("one final recorded command is captured", begins == 1 && ends == 1 && samples == 1);
    Check("historical non-strafe commands do not clear", helper_clears == 0 && helper_have);
    Check("capture closes after predicted results", !capturing && end_velocity == cl.predicted_velocity[0]
          && end_velocity != 0);
    Check("only final efficiency sample is published", efficiency_samples == 1);
    CL_PredictMovement();
    Check("replaying history still publishes once", begins == 2 && ends == 2 && samples == 2
          && helper_clears == 0);
}

static void TestPendingCommand(void)
{
    Setup();
    cl.cmds[3].sidemove = 0;
    cl.cmd.msec = 4;
    cl.localmove[0] = cl.localmove[1] = 200;
    CL_PredictMovement();
    Check("pending command owns the only capture", begins == 1 && ends == 1 && samples == 1);
    Check("replayed commands cannot clear pending data", helper_clears == 0 && helper_have);

    Setup();
    cl.cmds[2].sidemove = 200;
    cl.cmds[3].sidemove = 0;
    CL_PredictMovement();
    Check("final non-strafe command clears older data", begins == 1 && ends == 1 && samples == 0
          && helper_clears == 1 && !helper_have);
}

static void TestUnavailablePrediction(void)
{
    for (int reason = 0; reason < 7; reason++) {
        Setup();
        switch (reason) {
        case 0: cls.state = ca_disconnected; break;
        case 1: cls.demo.playback = true; break;
        case 2: paused_var.integer = 1; break;
        case 3: predict_var.integer = 0; break;
        case 4: cl.frame.ps.pmove.pm_flags |= PMF_NO_PREDICTION; break;
        case 5: cl.cmdNumber = CMD_BACKUP; break;
        case 6: cl.cmdNumber = 0; break;
        }
        CL_PredictMovement();
        Check("unavailable prediction clears helper", helper_clears == 1
              && !helper_have);
        Check("unavailable prediction never captures", begins == 0 && ends == 0 && samples == 0
              && !capturing);
    }
}

static void TestUnownedMovement(void)
{
    const pmtype_t types[] = { PM_NORMAL, PM_SPECTATOR, PM_DEAD, PM_FREEZE };
    const int flags[] = { 0, PMF_TIME_TELEPORT, PMF_TIME_WATERJUMP };
    for (size_t i = 0; i < q_countof(types); i++) {
        for (size_t j = 0; j < q_countof(flags); j++) {
            Setup();
            slick_ground = true;
            pmove_t move = { 0 };
            move.s = cl.frame.ps.pmove;
            move.s.pm_type = types[i];
            move.s.pm_flags = flags[j];
            move.s.pm_time = flags[j] ? 100 : 0;
            move.trace = CL_PMTrace;
            move.pointcontents = CL_PointContents;
            move.cmd = cl.cmds[3];
            PmoveNew(&move, &cl.pmp);
            Check("server or historical movement cannot change HUD", helper_have
                  && samples == 0 && helper_clears == 0
                  && efficiency_samples == 0);
        }
    }
}

static void TestMovementIsUnchanged(void)
{
    const unsigned durations[] = { 8, 16, 7, 25, 10, 40, 8, 9 };
    for (int mode = 0; mode < 7; mode++) {
        Setup();
        cl.pmp.airaccelerate = mode != 0;
        slick_ground = mode == 2;
        if (mode >= 3) {
            water_contents = CONTENTS_WATER;
            water_surface = mode == 3 ? -10 : mode == 4 ? 8 : 1000;
            if (mode == 6)
                water_contents |= CONTENTS_CURRENT_90;
        }
        pmove_t unowned = { 0 };
        unowned.s = cl.frame.ps.pmove;
        unowned.s.gravity = 800;
        unowned.trace = CL_PMTrace;
        unowned.pointcontents = CL_PointContents;
        pmove_t owned = unowned;
        for (unsigned step = 0; step < q_countof(durations); step++) {
            unowned.cmd = cl.cmds[3];
            unowned.cmd.msec = durations[step];
            unowned.cmd.angles[YAW] = ANGLE2SHORT(step * 11.0f);
            unowned.cmd.sidemove = step % 2 ? -200 : 200;
            if (mode >= 3) {
                unowned.cmd.angles[PITCH] = ANGLE2SHORT(step * -5.0f);
                unowned.cmd.upmove = step % 3 ? -100 : 100;
            }
            owned.cmd = unowned.cmd;
            PmoveNew(&unowned, &cl.pmp);
            StrafeHelper_BeginPrediction();
            PmoveNew(&owned, &cl.pmp);
            StrafeHelper_EndPrediction();
            Check("capturing telemetry preserves air, slick and water pmove state",
                  memcmp(&unowned.s, &owned.s, sizeof(owned.s)) == 0
                  && VectorCompare(unowned.viewangles, owned.viewangles)
                  && unowned.groundentity == owned.groundentity
                  && unowned.waterlevel == owned.waterlevel
                  && unowned.numtouch == owned.numtouch);
        }
    }
    Setup();
    slick_ground = true;
    CL_PredictMovement();
    Check("slick ground still publishes regular strafe telemetry",
          samples == 1 && begins == 1 && ends == 1 && efficiency_samples == 0);

    pmove_t coast = { 0 };
    coast.s = cl.frame.ps.pmove;
    coast.trace = CL_PMTrace;
    coast.pointcontents = CL_PointContents;
    coast.cmd.msec = 16;
    PmoveNew(&coast, &cl.pmp);
    Check("slick surface still preserves horizontal speed without input",
          coast.groundentity && coast.s.velocity[0] == cl.frame.ps.pmove.velocity[0]
          && coast.s.velocity[1] == 0);
}

static void TestAirTarget(void)
{
    Setup();
    cl.pmp.airaccelerate = true;
    CL_PredictMovement();
    Check("air cap reaches helper independently of wishspeed", samples == 1
          && sampled_target == 30.0f && sampled_wishspeed > 30.0f);
    Setup();
    CL_PredictMovement();
    Check("standard acceleration preserves wishspeed target", samples == 1
          && sampled_target == sampled_wishspeed);
}

static void SetupWater(int level)
{
    Setup();
    water_contents = CONTENTS_WATER;
    water_surface = level == 1 ? -10 : level == 2 ? 8 : 1000;
    cl.cmdNumber = 1;
    cl.cmds[1] = cl.cmds[3];
}

static void TestWaterAcceleration(void)
{
    for (int level = 1; level <= 3; level++) {
        for (int mode = 0; mode < 3; mode++) {
            SetupWater(level);
            if (mode == 1)
                PmoveEnableQW(&cl.pmp);
            else if (mode == 2) {
                cl.pmp.watermult = 0.65f;
                cl.pmp.waterfriction = 2;
                cl.pmp.maxspeed = 250;
            }
            cl.cmds[1].forwardmove = cl.cmds[1].sidemove = 400;
            CL_PredictMovement();
            float expected = cl.pmp.maxspeed;
            if (level >= 2)
                expected *= cl.pmp.watermult;
            Check("water depth fixture publishes one final sample",
                  samples == 1 && begins == 1 && ends == 1
                  && helper_have && sampled_waterlevel == level);
            Check("water uses server maxspeed and water multiplier",
                  fabsf(sampled_wishspeed - expected) < 0.001f);
            Check("water acceleration target is not the QW air cap",
                  fabsf(sampled_target - (level == 1 && mode == 1 ? 30 : expected)) < 0.001f);
            Check("water uses swimming acceleration and command duration",
                  sampled_accel == (level >= 2 || mode == 1 ? 10 : 1)
                  && fabsf(sampled_frametime - 0.008f) < 0.000001f);
            Check("water sample includes friction before acceleration",
                  fabsf(sampled_velocity[0] - 400 * (1 - cl.pmp.waterfriction * level * 0.008f)) < 0.001f);
            Check("all water depths keep efficiency disabled", efficiency_samples == 0);
        }
    }
}

static void TestWaterDirection(void)
{
    const int currents[] = { 0, 0, CONTENTS_CURRENT_UP, CONTENTS_CURRENT_DOWN };
    for (unsigned i = 0; i < q_countof(currents); i++) {
        SetupWater(3);
        water_contents |= currents[i];
        cl.pmp.waterhack = true;
        cl.frame.ps.pmove.velocity[2] = -640;
        float pitch = i % 2 ? -45 : 45;
        int upmove = i % 2 ? -80 : 80;
        cl.cmds[1].angles[PITCH] = ANGLE2SHORT(pitch);
        cl.cmds[1].forwardmove = 120;
        cl.cmds[1].sidemove = 80;
        cl.cmds[1].upmove = upmove;
        CL_PredictMovement();
        float diagonal = sqrtf(0.5f);
        float forward_z = pitch > 0 ? -diagonal : diagonal;
        float current_z = i == 2 ? 400 : i == 3 ? -400 : 0;
        vec3_t wish = { 120 * diagonal, -80, 120 * forward_z + upmove + current_z };
        float length = VectorLength(wish);
        Check("pitched swimming and vertical currents publish guidance", samples == 1 && helper_have);
        Check("swimming preserves full view pitch",
              fabsf(sampled_forward[0] - diagonal) < 0.0001f
              && fabsf(sampled_forward[2] - forward_z) < 0.0001f);
        for (int axis = 0; axis < 3; axis++)
            Check("swimming wish direction includes pitch, upmove and vertical current",
                  fabsf(sampled_wishdir[axis] - wish[axis] / length) < 0.0001f);
        Check("swimming uses full 3D wishspeed before server cap",
              fabsf(sampled_wishspeed - min(length, 300) * 0.5f) < 0.001f);
        Check("swimming captures vertical velocity after friction",
              fabsf(sampled_velocity[2] - -80 * 0.976f) < 0.001f);
    }

    SetupWater(3);
    cl.cmds[1].upmove = 100;
    CL_PredictMovement();
    Check("swim-up impulse reaches helper before water acceleration",
          samples == 1 && fabsf(sampled_velocity[2] - 100 * 0.976f) < 0.001f);
}

static void TestWaterSuppression(void)
{
    for (int input = 0; input < 3; input++) {
        SetupWater(3);
        cl.cmds[1].sidemove = 0;
        cl.cmds[1].forwardmove = input == 1 ? 200 : 0;
        cl.cmds[1].upmove = input == 2 ? 200 : 0;
        CL_PredictMovement();
        Check("idle, forward-only and vertical-only water input clears guidance",
              samples == 0 && helper_clears == 1 && !helper_have);
    }

    const int currents[] = { CONTENTS_CURRENT_0, CONTENTS_CURRENT_90,
                             CONTENTS_CURRENT_180, CONTENTS_CURRENT_270 };
    for (unsigned i = 0; i < q_countof(currents); i++) {
        for (int ground = 0; ground < 2; ground++) {
            SetupWater(3);
            if (ground) {
                slick_ground = true;
                ground_contents = currents[i];
            } else {
                water_contents |= currents[i];
            }
            CL_PredictMovement();
            Check("underwater horizontal currents suppress misleading yaw guidance",
                  samples == 0 && helper_clears == 1 && !helper_have);
        }
    }

    SetupWater(1);
    water_contents |= CONTENTS_CURRENT_0;
    CL_PredictMovement();
    Check("shallow horizontal current suppresses misleading yaw guidance",
          samples == 0 && helper_clears == 1 && !helper_have);

    SetupWater(3);
    cl.frame.ps.pmove.pm_flags = PMF_TIME_WATERJUMP;
    cl.frame.ps.pmove.pm_time = 100;
    cl.frame.ps.pmove.velocity[2] = 1600;
    CL_PredictMovement();
    Check("forced waterjump clears swimming guidance", samples == 0
          && helper_clears == 1 && !helper_have && efficiency_samples == 0);
}

static void TestHorizontalCurrents(void)
{
    const struct {
        int contents;
        bool horizontal;
    } currents[] = {
        { 0, false },
        { CONTENTS_CURRENT_UP, false },
        { CONTENTS_CURRENT_DOWN, false },
        { CONTENTS_CURRENT_UP | CONTENTS_CURRENT_DOWN, false },
        { CONTENTS_CURRENT_0 | CONTENTS_CURRENT_180, false },
        { CONTENTS_CURRENT_90 | CONTENTS_CURRENT_270, false },
        { CONTENTS_CURRENT_0 | CONTENTS_CURRENT_90 |
          CONTENTS_CURRENT_180 | CONTENTS_CURRENT_270 | CONTENTS_CURRENT_UP, false },
        { CONTENTS_CURRENT_0, true },
        { CONTENTS_CURRENT_90, true },
        { CONTENTS_CURRENT_180, true },
        { CONTENTS_CURRENT_270, true },
        { CONTENTS_CURRENT_0 | CONTENTS_CURRENT_90, true },
        { CONTENTS_CURRENT_0 | CONTENTS_CURRENT_180 | CONTENTS_CURRENT_90, true },
    };

    for (int level = 0; level <= 3; level++) {
        for (int grounded = 0; grounded < 2; grounded++) {
            for (int source = 0; source < 2; source++) {
                for (unsigned i = 0; i < q_countof(currents); i++) {
                    for (int qw = 0; qw < 2; qw++) {
                        Setup();
                        slick_ground = grounded;
                        water_contents = level ? CONTENTS_WATER : 0;
                        water_surface = level == 1 ? -10 : level == 2 ? 8 : 1000;
                        if (source == 0)
                            water_contents |= currents[i].contents;
                        else
                            ground_contents = currents[i].contents;
                        cl.pmp.airaccelerate = qw;
                        cl.cmdNumber = 1;
                        cl.cmds[1] = cl.cmds[3];

                        pmove_t unowned = { 0 };
                        unowned.s = cl.frame.ps.pmove;
                        unowned.trace = CL_PMTrace;
                        unowned.pointcontents = CL_PointContents;
                        unowned.snapinitial = true;
                        unowned.cmd = cl.cmds[1];
                        PmoveNew(&unowned, &cl.pmp);
                        Check("unowned current movement cannot publish or clear guidance",
                              helper_have && !samples && !helper_clears && !efficiency_samples);
                        CL_PredictMovement();

                        const bool blocked = currents[i].horizontal &&
                                             (source == 0 ? level != 0 : grounded != 0);
                        char name[128];
                        Q_scnprintf(name, sizeof(name),
                                    "current guidance: level %d ground %d source %d mask %x QW %d",
                                    level, grounded, source, currents[i].contents, qw);
                        Check(name, begins == 1 && ends == 1 && !capturing &&
                              samples == !blocked && helper_have == !blocked &&
                              helper_clears == blocked);
                        Check("current depth and ground fixture are as requested",
                              unowned.waterlevel == level && !!unowned.groundentity == !!grounded);
                        for (int axis = 0; axis < 3; axis++) {
                            Check("current guidance preserves predicted movement",
                                  cl.predicted_origin[axis] == unowned.s.origin[axis] * 0.125f &&
                                  cl.predicted_velocity[axis] == unowned.s.velocity[axis] * 0.125f &&
                                  cl.predicted_angles[axis] == unowned.viewangles[axis]);
                        }
                    }
                }
            }
        }
    }

    for (int level = 1; level <= 3; level++) {
        SetupWater(level);
        slick_ground = true;
        water_contents |= CONTENTS_CURRENT_0;
        ground_contents = CONTENTS_CURRENT_180;
        CL_PredictMovement();
        // Water adds 200/400 UPS and a conveyor 100, so opposite sources do not cancel.
        Check("opposing water and conveyor currents still suppress guidance",
              samples == 0 && helper_clears == 1 && !helper_have);
    }
}

static void TestWaterFreshness(void)
{
    Setup();
    water_contents = CONTENTS_WATER;
    water_surface = 1000;
    CL_PredictMovement();
    Check("only final command publishes swimming guidance", samples == 1
          && begins == 1 && ends == 1 && helper_have && efficiency_samples == 0
          && sampled_waterlevel == 3);
    water_contents = 0;
    CL_PredictMovement();
    Check("leaving water replaces swimming data with air data", samples == 2
          && begins == 2 && ends == 2 && helper_have && efficiency_samples == 1
          && sampled_waterlevel == 0 && sampled_accel == 1);
    water_contents = CONTENTS_WATER;
    cl.cmds[2].sidemove = 200;
    cl.cmds[3].sidemove = 0;
    CL_PredictMovement();
    Check("reentering water without strafe clears prior guidance", samples == 2
          && begins == 3 && ends == 3 && helper_clears == 1 && !helper_have
          && efficiency_samples == 1);
}

int main(void)
{
    TestFinalCommand();
    TestPendingCommand();
    TestUnavailablePrediction();
    TestUnownedMovement();
    TestAirTarget();
    TestMovementIsUnchanged();
    TestWaterAcceleration();
    TestWaterDirection();
    TestWaterSuppression();
    TestHorizontalCurrents();
    TestWaterFreshness();
    if (failures) {
        fprintf(stderr, "%d strafe prediction checks failed\n", failures);
        return EXIT_FAILURE;
    }
    printf("strafe prediction checks passed\n");
    return EXIT_SUCCESS;
}
