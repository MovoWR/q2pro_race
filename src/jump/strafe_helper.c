#include "strafe_helper.h"
#include <src/client/client.h>
#include "strafe_helper_customization.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
struct StrafeHelper sh;


static float sign(const float value)
{
	if (value < 0.0f)
	{
		return -1.0f;
	}
	else if (value > 0.0f)
	{
		return 1.0f;
	}
	return 0.0f;
}
static float crossProduct(const float v[2], const float w[2])
{
	return v[0] * w[1] - v[1] * w[0];
}
static float dotProduct(const float v[2], const float w[2])
{
	float dot_product = 0.0f;
    for (int i = 0; i < 2; i++) {
		dot_product += v[i] * w[i];
	}
	return dot_product;
}
static float angleBetweenVectors(const float v[2], const float w[2]) {
	return atan2f(crossProduct(v, w), dotProduct(v, w));
}
static float vectorAngleSign(const float v[2], const float w[2]) {
	return sign(crossProduct(v, w));
}
static float vectorNorm(const float v[2])
{
	return sqrtf(dotProduct(v, v));
}



// Strafe Calculations
void StrafeHelper_SetAccelerationValues(const float forward[3],
                                        const float velocity[3],
                                        const float wishdir[3],
                                        const float wishspeed,
                                        const float accel,
                                        const float frametime)
{

	const float v_z = velocity[2];
	const float w_z = wishdir[2];
	const float wishdir_norm = vectorNorm(wishdir);
	const float forward_velocity_angle = angleBetweenVectors(wishdir, forward);
	const float angle_sign = vectorAngleSign(wishdir, velocity);
	const float two_pi = 2.0f * (float)M_PI;
    sh.velocity_norm = vectorNorm(velocity);
    sh.angle_optimal = (wishspeed * (1.0f - accel * frametime) - v_z * w_z) / (sh.velocity_norm * wishdir_norm);
    sh.angle_optimal = acosf(sh.angle_optimal);
    sh.angle_optimal = angle_sign * sh.angle_optimal - forward_velocity_angle;

    sh.angle_minimum = (wishspeed - v_z * w_z) /
                       (2.0f - wishdir_norm * wishdir_norm) * wishdir_norm / sh.velocity_norm;
    sh.angle_minimum = acosf(sh.angle_minimum < 1.0f ? sh.angle_minimum : 1.0f);
    sh.angle_minimum = angle_sign * sh.angle_minimum - forward_velocity_angle;

    sh.angle_maximum = -0.5f * accel * frametime * wishspeed * wishdir_norm / sh.velocity_norm;
    sh.angle_maximum = acosf(sh.angle_maximum);
    sh.angle_maximum = angle_sign * sh.angle_maximum - forward_velocity_angle;

	sh.angle_current = angleBetweenVectors(forward, velocity);
	sh.angle_current += truncf((sh.angle_minimum - sh.angle_current) / two_pi) * two_pi;
	sh.angle_current += truncf((sh.angle_maximum - sh.angle_current) / two_pi) * two_pi;
	}


static float angleDiffToPixelDiff(const float angle_difference, const float scale,
                                  const float hud_width)
{
	return angle_difference * (hud_width / 2.0f) * scale / (float)M_PI;
}
static float angleToPixel(const float angle, const float scale,
                          const float hud_width)
{
	return (hud_width / 2.0f) - 0.5f +
	       angleDiffToPixelDiff(angle, scale, hud_width);
}




void StrafeHelper_Draw(const struct StrafeHelperParams *params,
                       const float hud_width, const float hud_height) {
	static float previous_velocity_norm = 0.0f;
	bool isOptimal = fabsf(sh.angle_current - sh.angle_optimal) <= OPTIMAL_ANGLE_TOLERANCE;
	bool insideAccelerationZone = (sh.angle_current >= sh.angle_minimum && sh.angle_current <= sh.angle_maximum) ||
								  (sh.angle_current >= sh.angle_maximum && sh.angle_current <= sh.angle_minimum);
	bool speedIncreased = sh.velocity_norm > previous_velocity_norm;
	float angle_x, angle_width;
	const float upper_y = (hud_height - params->height) / 2.0f + params->y;
    const float center_width = CLAMP(cl_strafehelper_center_width->value, 0.1f, 5.0f);
    const float optimal_width = CLAMP(cl_strafehelper_optimal_width->value, 0.1f, 5.0f);

	float offset = 0.0f;
	if (params->center)	{
		offset = -sh.angle_current;
	}
	if (sh.angle_minimum < sh.angle_maximum) {
		angle_x = sh.angle_minimum + offset;
		angle_width = sh.angle_maximum - sh.angle_minimum;
	} else {
		angle_x = sh.angle_maximum + offset;
		angle_width = sh.angle_minimum - sh.angle_maximum;
	}

	// Draw accel range
	shc_drawFilledRectangle(
		angleToPixel(angle_x, params->scale, hud_width),
		upper_y,
		angleDiffToPixelDiff(angle_width, params->scale, hud_width),
		params->height,
		shc_ElementId_AcceleratingAngles);

	// Draw optimal angle
	shc_drawFilledRectangle(
		angleToPixel(sh.angle_optimal + offset, params->scale, hud_width) - (optimal_width / 2.0f),
		upper_y,
		optimal_width,
		params->height,
		shc_ElementId_OptimalAngle);
	if (params->center_marker) {
		// Draw center marker
		shc_drawFilledRectangle(
			angleToPixel(sh.angle_current + offset, params->scale, hud_width) - (center_width / 2.0f),
			upper_y + params->height / 2.0f,
			center_width,
			params->height / 2.0f,
			shc_ElementId_CenterMarker);
	}

	// Draw Indicator
	if (cl_strafehelperIndicator->value && isOptimal && insideAccelerationZone && speedIncreased) {
		const char *highlight_pos = cl_strafehelper_indicator_pos->string;
		const char *highlight_size = cl_strafehelper_indicator_size->string;
		float custom_x = hud_width / 2.0f;
		float custom_y = hud_height / 2.0f;
		float custom_width = 10.0f;
		float custom_height = params->height / 2.0f;
		sscanf(highlight_pos, "%f %f", &custom_x, &custom_y);
		sscanf(highlight_size, "%f %f", &custom_width, &custom_height);
		custom_x = CLAMP(custom_x, 0.0f, hud_width);
		custom_y = CLAMP(custom_y, 0.0f, hud_height);
		custom_width = CLAMP(custom_width, 1.0f, hud_width);
		custom_height = CLAMP(custom_height, 1.0f, hud_height);
		shc_drawFilledRectangle(
			custom_x - (custom_width / 2.0f),
			custom_y - custom_height,
			custom_width,
			custom_height,
			shc_ElementId_Indicator);
	}

	previous_velocity_norm = sh.velocity_norm;
    }









