#include "strafe_helper_customization.h"
#include "strafe_helper.h"
#include "shared/shared.h"
#include "refresh/refresh.h"
#include "src/client/client.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "src/refresh/gl.h"


uint32_t ParseColorCvar(cvar_t *cvar)
{
	int r, g, b, a;
	sscanf(cvar->string, "%d %d %d %d", &r, &g, &b, &a);

	return MakeColor(r, g, b, a);
}

uint32_t getColorForElement(const enum shc_ElementId element_id)
{
	switch (element_id) {
		case shc_ElementId_AcceleratingAngles:
			return ParseColorCvar(cl_strafehelper_color_accelerating);
		case shc_ElementId_OptimalAngle:
			return ParseColorCvar(cl_strafehelper_color_optimal);
		case shc_ElementId_CenterMarker:
			return ParseColorCvar(cl_strafehelper_color_centermarker);
       	case shc_ElementId_HighlightOptimal:
       		return ParseColorCvar(cl_strafehelper_color_highlight);
		default:
			assert(0);
			return MakeColor(255, 255, 255, 255);
    }
}

void shc_drawFilledRectangle(const float x, const float y,
							 const float w, const float h,
                             enum shc_ElementId element_id) {
	const uint32_t color = getColorForElement(element_id);
	R_DrawFill32(roundf(x), roundf(y), roundf(w), roundf(h), color);
}
//

