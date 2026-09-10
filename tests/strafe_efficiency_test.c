#include "strafe_efficiency.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int failures;

static void CheckTrue(const char *name, const bool condition)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", name);
        failures++;
    }
}

static void CheckNear(const char *name, const float actual,
                      const float expected, const float tolerance)
{
    if (!isfinite(actual) || fabsf(actual - expected) > tolerance) {
        fprintf(stderr, "FAIL: %s (got %.7f, expected %.7f)\n",
                name, actual, expected);
        failures++;
    }
}

static void TestZeroEfficiency(void)
{
    const float velocity[2] = { 400.0f, 0.0f };
    const float wishdir[2] = { 1.0f, 0.0f };
    const StrafeEfficiency result = StrafeEfficiency_Calculate(
        velocity, wishdir, 300.0f, 7.5f);

    CheckTrue("aligned high-speed sample is valid", result.valid);
    CheckNear("aligned high-speed gain", result.gain, 0.0f, 0.0001f);
    CheckNear("aligned high-speed maximum", result.maximum_gain,
              5.5166f, 0.001f);
    CheckNear("aligned high-speed efficiency", result.efficiency,
              0.0f, 0.0001f);
}

static void TestPerfectEfficiency(void)
{
    const float slow_velocity[2] = { 100.0f, 0.0f };
    const float forward[2] = { 1.0f, 0.0f };
    StrafeEfficiency result = StrafeEfficiency_Calculate(
        slow_velocity, forward, 300.0f, 7.5f);

    CheckTrue("slow aligned sample is valid", result.valid);
    CheckNear("slow aligned gain", result.gain, 7.5f, 0.0001f);
    CheckNear("slow aligned maximum", result.maximum_gain, 7.5f, 0.0001f);
    CheckNear("slow aligned efficiency", result.efficiency, 1.0f, 0.0001f);

    const float fast_velocity[2] = { 400.0f, 0.0f };
    const float sideways[2] = { 0.0f, 2.0f };
    result = StrafeEfficiency_Calculate(fast_velocity, sideways, 30.0f, 75.0f);

    CheckTrue("air-clamp sample is valid", result.valid);
    CheckNear("air-clamp perpendicular gain", result.gain, 1.1234f, 0.001f);
    CheckNear("air-clamp perpendicular efficiency", result.efficiency,
              1.0f, 0.0001f);
}

static void TestNegativeGainScoresZero(void)
{
    const float velocity[2] = { 100.0f, 0.0f };
    const float opposite[2] = { -1.0f, 0.0f };
    const StrafeEfficiency result = StrafeEfficiency_Calculate(
        velocity, opposite, 300.0f, 7.5f);

    CheckTrue("negative-gain sample is valid", result.valid);
    CheckTrue("opposite acceleration loses speed", result.gain < 0.0f);
    CheckNear("negative gain scores zero", result.efficiency, 0.0f, 0.0001f);
}

static void TestNearStopBraking(void)
{
    const float velocity[2] = { 1.0f, 5.0f };
    const float opposite[2] = { -1.0f, -5.0f };
    const double speed = sqrt(26.0);
    const float budgets[] = {
        (float)speed - 0.01f, (float)speed, (float)speed + 0.01f,
    };

    for (size_t i = 0; i < sizeof(budgets) / sizeof(budgets[0]); i++) {
        const StrafeEfficiency result = StrafeEfficiency_Calculate(
            velocity, opposite, 300.0f, budgets[i]);
        // Collinear braking leaves abs(speed - budget) speed, including reversal.
        const float expected_gain = (float)(fabs(speed - budgets[i]) - speed);

        CheckTrue("near-stop sample is valid", result.valid);
        CheckNear("near-stop braking preserves speed loss", result.gain,
                  expected_gain, 0.000001f);
        CheckNear("near-stop braking scores zero", result.efficiency,
                  0.0f, 0.0001f);
    }
}

static void TestStableHighSpeedGain(void)
{
    const float velocity[2] = { 1500.0f, 0.0f };
    const float projection = 299.0f / 1500.0f;
    const float wishdir[2] = {
        projection,
        sqrtf(1.0f - projection * projection),
    };
    const StrafeEfficiency result = StrafeEfficiency_Calculate(
        velocity, wishdir, 300.0f, 2.4f);

    CheckTrue("high-speed sample is valid", result.valid);
    CheckNear("high-speed stable gain", result.gain, 0.1996534f, 0.00001f);
    CheckTrue("high-speed efficiency is bounded",
              result.efficiency > 0.0f && result.efficiency < 1.0f);
}

static void TestAirClampAtEightMilliseconds(void)
{
    const float velocity[2] = { 400.0f, 0.0f };
    const float optimal_projection = 6.0f / 400.0f;
    const float optimal[2] = {
        optimal_projection,
        sqrtf(1.0f - optimal_projection * optimal_projection),
    };
    const float perpendicular[2] = { 0.0f, 1.0f };
    StrafeEfficiency result = StrafeEfficiency_Calculate(
        velocity, optimal, 30.0f, 24.0f);

    CheckTrue("8 ms air-clamp optimum is valid", result.valid);
    CheckNear("8 ms air-clamp optimum scores perfectly",
              result.efficiency, 1.0f, 0.0001f);

    result = StrafeEfficiency_Calculate(
        velocity, perpendicular, 30.0f, 24.0f);
    CheckTrue("8 ms perpendicular sample is valid", result.valid);
    CheckTrue("8 ms perpendicular scores below optimum",
              result.efficiency > 0.0f && result.efficiency < 1.0f);
}

static void TestInvalidInputs(void)
{
    const float velocity[2] = { 400.0f, 0.0f };
    const float direction[2] = { 1.0f, 0.0f };
    const float no_direction[2] = { 0.0f, 0.0f };

    CheckTrue("zero wish direction is invalid",
              !StrafeEfficiency_Calculate(velocity, no_direction,
                                          300.0f, 7.5f).valid);
    CheckTrue("zero target is invalid",
              !StrafeEfficiency_Calculate(velocity, direction,
                                          0.0f, 7.5f).valid);
    CheckTrue("zero budget is invalid",
              !StrafeEfficiency_Calculate(velocity, direction,
                                          300.0f, 0.0f).valid);
    CheckTrue("non-finite input is invalid",
              !StrafeEfficiency_Calculate(velocity, direction,
                                          NAN, 7.5f).valid);
    CheckTrue("null velocity is invalid",
              !StrafeEfficiency_Calculate(NULL, direction,
                                          300.0f, 7.5f).valid);
    CheckTrue("null wish direction is invalid",
              !StrafeEfficiency_Calculate(velocity, NULL,
                                          300.0f, 7.5f).valid);
    CheckTrue("negative target is invalid",
              !StrafeEfficiency_Calculate(velocity, direction,
                                          -300.0f, 7.5f).valid);
    CheckTrue("negative budget is invalid",
              !StrafeEfficiency_Calculate(velocity, direction,
                                          300.0f, -7.5f).valid);
}

static const StrafeEfficiencyColor test_bad = { 235, 70, 60, 255 };
static const StrafeEfficiencyColor test_mid = { 235, 200, 60, 255 };
static const StrafeEfficiencyColor test_good = { 80, 220, 90, 255 };

static bool ColorEquals(const StrafeEfficiencyColor a,
                        const StrafeEfficiencyColor b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

static void TestMapColorEndpoints(void)
{
    CheckTrue("value 0 maps to bad color",
              ColorEquals(StrafeEfficiency_MapColor(0.0f, 0.5f, test_bad,
                                                    test_mid, test_good),
                          test_bad));
    CheckTrue("value 1 maps to good color",
              ColorEquals(StrafeEfficiency_MapColor(1.0f, 0.5f, test_bad,
                                                    test_mid, test_good),
                          test_good));
    CheckTrue("value at midpoint maps to mid color",
              ColorEquals(StrafeEfficiency_MapColor(0.5f, 0.5f, test_bad,
                                                    test_mid, test_good),
                          test_mid));
    CheckTrue("shifted midpoint maps to mid color",
              ColorEquals(StrafeEfficiency_MapColor(0.85f, 0.85f, test_bad,
                                                    test_mid, test_good),
                          test_mid));
}

static void TestMapColorInterpolatesAndClamps(void)
{
    const StrafeEfficiencyColor quarter = StrafeEfficiency_MapColor(
        0.25f, 0.5f, test_bad, test_mid, test_good);
    CheckNear("quarter value interpolates green", quarter.g, 135.0f, 1.0f);
    CheckNear("quarter value keeps red", quarter.r, 235.0f, 1.0f);

    CheckTrue("value above 1 clamps to good",
              ColorEquals(StrafeEfficiency_MapColor(2.0f, 0.5f, test_bad,
                                                    test_mid, test_good),
                          test_good));
    CheckTrue("negative value clamps to bad",
              ColorEquals(StrafeEfficiency_MapColor(-1.0f, 0.5f, test_bad,
                                                    test_mid, test_good),
                          test_bad));
    CheckTrue("NaN value clamps to bad",
              ColorEquals(StrafeEfficiency_MapColor(NAN, 0.5f, test_bad,
                                                    test_mid, test_good),
                          test_bad));
    CheckTrue("out-of-range midpoint is clamped, not divided by zero",
              ColorEquals(StrafeEfficiency_MapColor(1.0f, 2.0f, test_bad,
                                                    test_mid, test_good),
                          test_good));
}

static void TestBlendColor(void)
{
    const StrafeEfficiencyColor base = { 0, 128, 0, 128 };
    const StrafeEfficiencyColor tint = { 200, 0, 100, 255 };

    CheckTrue("zero strength keeps base color",
              ColorEquals(StrafeEfficiency_BlendColor(base, tint, 0.0f), base));

    const StrafeEfficiencyColor full =
        StrafeEfficiency_BlendColor(base, tint, 1.0f);
    CheckNear("full strength takes tint red", full.r, 200.0f, 0.5f);
    CheckNear("full strength takes tint green", full.g, 0.0f, 0.5f);
    CheckNear("full strength keeps base alpha", full.a, 128.0f, 0.5f);

    const StrafeEfficiencyColor half =
        StrafeEfficiency_BlendColor(base, tint, 0.5f);
    CheckNear("half strength blends red", half.r, 100.0f, 1.0f);
    CheckNear("half strength blends green", half.g, 64.0f, 1.0f);
    CheckNear("half strength keeps base alpha", half.a, 128.0f, 0.5f);

    CheckTrue("over-range strength clamps to tint RGB",
              StrafeEfficiency_BlendColor(base, tint, 5.0f).r == full.r);
    CheckTrue("NaN strength keeps base color",
              ColorEquals(StrafeEfficiency_BlendColor(base, tint, NAN), base));
}

static void TestSmoothStep(void)
{
    CheckNear("zero smoothing snaps to target",
              StrafeEfficiency_SmoothStep(0.0f, 1.0f, 0.0f, 0.016f),
              1.0f, 0.0001f);
    CheckNear("zero elapsed time preserves the filtered value",
              StrafeEfficiency_SmoothStep(0.0f, 1.0f, 5.0f, 0.0f),
              0.0f, 0.0001f);

    const float step = StrafeEfficiency_SmoothStep(0.0f, 1.0f, 2.0f, 0.016f);
    CheckTrue("smoothing moves toward target", step > 0.0f && step < 1.0f);
    CheckNear("smoothing matches exponential factor", step,
              1.0f - expf(-0.016f / 0.1f), 0.001f);

    // The same 100 ms of a constant input must have the same response,
    // irrespective of how many display or command updates divide it.
    const int intervals[][5] = {
        { 1, 1, 1, 1, 1 },
        { 10, 10, 10, 10, 10 },
        { 1, 7, 19, 23, 50 },
    };
    for (int cadence = 0; cadence < 3; cadence++) {
        float filtered = 0.0f;
        int elapsed = 0, sample = 0;
        while (elapsed < 100) {
            const int ms = intervals[cadence][sample++ % 5];
            const float previous = filtered;
            filtered = StrafeEfficiency_SmoothStep(filtered, 1.0f, 10.0f,
                                                   ms * 0.001f);
            CheckTrue("filter stays monotone and bounded",
                      filtered >= previous && filtered <= 1.0f);
            CheckNear("repeated timestamp does not advance the filter",
                      StrafeEfficiency_SmoothStep(filtered, 1.0f, 10.0f, 0.0f),
                      filtered, 0.000001f);
            elapsed += ms;
        }
        CheckNear("500-ms time constant after 100 ms",
                  filtered, 0.18126925f, 0.000002f);
    }
    CheckNear("long interval reaches target without overshooting",
              StrafeEfficiency_SmoothStep(0.0f, 1.0f, 0.5f, 1.0f),
              1.0f, 0.000001f);
    CheckNear("already at target stays put",
              StrafeEfficiency_SmoothStep(0.7f, 0.7f, 5.0f, 0.016f),
              0.7f, 0.0001f);
}

int main(void)
{
    TestZeroEfficiency();
    TestPerfectEfficiency();
    TestNegativeGainScoresZero();
    TestNearStopBraking();
    TestStableHighSpeedGain();
    TestAirClampAtEightMilliseconds();
    TestInvalidInputs();
    TestMapColorEndpoints();
    TestMapColorInterpolatesAndClamps();
    TestBlendColor();
    TestSmoothStep();

    if (failures) {
        fprintf(stderr, "%d strafe-efficiency test(s) failed\n", failures);
        return EXIT_FAILURE;
    }

    puts("strafe-efficiency tests passed");
    return EXIT_SUCCESS;
}
