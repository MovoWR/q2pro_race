#include "strafe_helper_customization.h"
#include "shared/shared.h"
#include "refresh/refresh.h"
#include "src/client/client.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "src/refresh/gl.h"


uint32_t shc_ParseColorString(const char *colorStr, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a) {
	if (!colorStr) {
		return MakeColor(255, 255, 255, 255);
	}

	int ri = 255, gi = 255, bi = 255, ai = 255;
	int components = sscanf(colorStr, "%d %d %d %d", &ri, &gi, &bi, &ai);

	if (components < 3) {
		return MakeColor(255, 255, 255, 255);
	}

	uint8_t rr = (uint8_t)(ri < 0 ? 0 : (ri > 255 ? 255 : ri));
	uint8_t gg = (uint8_t)(gi < 0 ? 0 : (gi > 255 ? 255 : gi));
	uint8_t bb = (uint8_t)(bi < 0 ? 0 : (bi > 255 ? 255 : bi));
	uint8_t aa = (uint8_t)(ai < 0 ? 0 : (ai > 255 ? 255 : ai));

	if (r) *r = rr;
	if (g) *g = gg;
	if (b) *b = bb;
	if (a) *a = aa;
	return MakeColor(rr, gg, bb, aa);
}


bool shc_ParseColorCvar(const char *cvarValue, uint32_t *outUint32,
                        color_t *outColor) {
  uint8_t r, g, b, a;
  if (!shc_ParseColorString(cvarValue, &r, &g, &b, &a)) {
    return false;
  }

  if (outUint32) {
    *outUint32 = (r << 24) | (g << 16) | (b << 8) | a;
  }

  if (outColor) {
    outColor->u8[0] = r;
    outColor->u8[1] = g;
    outColor->u8[2] = b;
    outColor->u8[3] = a;
  }
  return true;
}

uint32_t getColorForElement(const enum shc_ElementId element_id) {
	const char *colorString = NULL;

	switch (element_id) {
	case shc_ElementId_AcceleratingAngles:
		colorString = cl_strafehelper_color_accelerating->string;
		break;
	case shc_ElementId_OptimalAngle:
		colorString = cl_strafehelper_color_optimal->string;
		break;
	case shc_ElementId_CenterMarker:
		colorString = cl_strafehelper_color_centermarker->string;
		break;
	case shc_ElementId_Indicator:
		colorString = cl_strafehelper_color_indicator->string;
		break;
    case shc_ElementId_NerdStats:
        colorString = cl_strafehelper_color_nerdstats->string;
        break;
    default: ;
	}
	return shc_ParseColorString(colorString, NULL, NULL, NULL, NULL);
}

void shc_drawFilledRectangle(const float x, const float y,
							 const float w, const float h,
                             enum shc_ElementId element_id) {
	const uint32_t color = getColorForElement(element_id);
	R_DrawFill32(roundf(x), roundf(y), roundf(w), roundf(h), color);
}


