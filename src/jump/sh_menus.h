#include <math.h>
#include "src/client/client.h"

void SH_Cmd_g(genctx_t *ctx, int argnum);

void SH_Cmd_f(void);

void SH_Help_f(void);

void SH_Enable_f(void);

void SH_Disable_f(void);

void SH_Scale_f(void);

void SH_ypos_f(void);

void SH_Height_f(void);

void SH_CenterMarker_f(void);

void SH_CenterWidth_f(void);

void SH_OptimalWidth_f(void);

void SH_Alpha_f(void);

void SH_BarStyle_f(void);

void SH_Smoothing_f(void);

void SH_SmoothingMode_f(void);

void SH_Ups_Enable_f(void);

void SH_Ups_Disable_f(void);

void SH_Ups_Toggle_f(void);

void SH_Ups_Status_f(void);

void SH_Ups_Help_f(void);

void SH_Ups_Scale_f(void);

void SH_Ups_Ypos_f(void);

void SH_Ups_Shadow_f(void);

void SH_Ups_HideZero_f(void);

void SH_Ups_ColorMode_f(void);

void SH_Ups_ColorGain_f(void);

void SH_Ups_ColorLoss_f(void);

void SH_Ups_ColorNeutral_f(void);

void SH_Ups_Format_f(void);

void SH_Color_Accel_f(void);

void SH_Color_Optimal_f(void);

void SH_Color_CenterMarker_f(void);

void SH_Status_f(void);

void SH_Hud_Help_f(void);

void SH_SetPreset_f(void);

bool SH_GetPresetColors(const char *name, const char **accelerating,
                        const char **optimal, const char **centermarker);
