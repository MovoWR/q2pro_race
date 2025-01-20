//
// Created by stasi on 20.01.2025.
//
#include <math.h>
#include "src/client/client.h"
#ifndef SH_MENUS_H
#define SH_MENUS_H
void SH_Indicator_Cmd_g(genctx_t* ctx, int argnum);
void SH_Cmd_g(genctx_t* ctx, int argnum);

void SH_Indicator_Cmd_f(void);
void SH_Indicator_Help(void);
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
void SH_Color_Accel_f(void);
void SH_Color_Optimal_f(void);
void SH_Color_CenterMarker_f(void);
void SH_Status_f(void);
void SH_Hud_Help_f(void);
void SH_Indicator_Enable_f(void);
void SH_Indicator_Disable_f(void);
void SH_Indicator_SetPos_f(void);
void SH_Indicator_SetSize_f(void);
void SH_Indicator_SetColor_f(void);
void SH_Indicator_SetPic_f(void);
void SH_Indicator_Help(void);
void SH_Indicator_GetStatus_f(void);
void SH_Indicator_Tolerance_f(void);
void DebugTracking(void);
void GrenadeTimer_Update(void);
void GrenadeTimer_Add(float firing_time);

#endif //SH_MENUS_H
