#include "sh_menus.h"
#include <src/client/client.h>
#include "strafe_helper_customization.h"
#include "strafe_helper.h"
#include <math.h>

void SH_Help_f(void) {
    Com_Printf("========================================================================================\n");
    Com_LPrintf(PRINT_WARNING, "Strafe Helper\n");
    Com_Printf("Usage: sh <section> <command> [options]\n");
    Com_Printf("========================================================================================\n");
    Com_Printf("Sections\n");
    Com_Printf("  %-18s %s\n", "hud", "Strafe bar visibility, geometry, style, and colors.");
    Com_Printf("  %-18s %s\n", "ups", "Centered cl_ups readout and its display/color options.");
    Com_Printf("  %-18s %s\n", "status", "Show all current strafe helper settings.");
    Com_Printf("----------------------------------------------------------------------------------------\n");
    Com_Printf("Common examples\n");
    Com_Printf("  sh hud enable\n");
    Com_Printf("  sh hud bar_style gradient\n");
    Com_Printf("  sh hud alpha 0.75\n");
    Com_Printf("  sh ups enable\n");
    Com_Printf("  sh ups color_mode dynamic\n");
    Com_Printf("----------------------------------------------------------------------------------------\n");
    Com_Printf("Related cvars\n");
    Com_Printf("  %-18s %s\n", "sh_nerdstats", "1 left, 2 right.");
    Com_Printf("  %-18s %s\n", "race_width", "Race line thickness, 1 to 20.");
    Com_Printf("  %-18s %s\n", "race_color", "Race line RGB color.");
    Com_Printf("  %-18s %s\n", "race_alpha", "Race line opacity, 0 to 1.");
    Com_Printf("  %-18s %s\n", "race_life", "Race line lifetime, 100 to 5000 ms.");
    Com_Printf("========================================================================================\n");
}


void SH_Cmd_g(genctx_t *ctx, int argnum) {
    R_SetColor(U32_RED);
    if (argnum == 1) {
        Prompt_AddMatch(ctx, "hud");
        Prompt_AddMatch(ctx, "ups");
        Prompt_AddMatch(ctx, "status");
        Prompt_AddMatch(ctx, "help");
    } else if (argnum == 2) {
        const char *subcmd = Cmd_Argv(1);
        if (!strcmp(subcmd, "hud")) {
            Prompt_AddMatch(ctx, "enable");
            Prompt_AddMatch(ctx, "disable");
            Prompt_AddMatch(ctx, "center_marker");
            Prompt_AddMatch(ctx, "scale");
            Prompt_AddMatch(ctx, "ypos");
            Prompt_AddMatch(ctx, "height");
            Prompt_AddMatch(ctx, "alpha");
            Prompt_AddMatch(ctx, "fade_inactive");
            Prompt_AddMatch(ctx, "bar_style");
            Prompt_AddMatch(ctx, "smoothing");
            Prompt_AddMatch(ctx, "smoothing_mode");
            Prompt_AddMatch(ctx, "center_width");
            Prompt_AddMatch(ctx, "optimal_width");
            Prompt_AddMatch(ctx, "color_accelerating");
            Prompt_AddMatch(ctx, "color_optimal");
            Prompt_AddMatch(ctx, "color_centermarker");
            Prompt_AddMatch(ctx, "preset");
            Prompt_AddMatch(ctx, "status");
            Prompt_AddMatch(ctx, "help");
        } else if (!strcmp(subcmd, "ups")) {
            Prompt_AddMatch(ctx, "enable");
            Prompt_AddMatch(ctx, "disable");
            Prompt_AddMatch(ctx, "toggle");
            Prompt_AddMatch(ctx, "status");
            Prompt_AddMatch(ctx, "scale");
            Prompt_AddMatch(ctx, "ypos");
            Prompt_AddMatch(ctx, "shadow");
            Prompt_AddMatch(ctx, "hide_zero");
            Prompt_AddMatch(ctx, "color_mode");
            Prompt_AddMatch(ctx, "color_gain");
            Prompt_AddMatch(ctx, "color_loss");
            Prompt_AddMatch(ctx, "color_neutral");
            Prompt_AddMatch(ctx, "format");
            Prompt_AddMatch(ctx, "help");
        } else if (!strcmp(subcmd, "status")) {
            Prompt_AddMatch(ctx, "status");
        }
    } else if (argnum == 3) {
        const char *subcmd = Cmd_Argv(1);
        const char *cmd = Cmd_Argv(2);

        if (!strcmp(subcmd, "hud")) {
            if (!strcmp(cmd, "fade_inactive")) {
                Prompt_AddMatch(ctx, "0");
                Prompt_AddMatch(ctx, "1");
            } else if (!strcmp(cmd, "bar_style")) {
                Prompt_AddMatch(ctx, "gradient");
                Prompt_AddMatch(ctx, "solid");
                Prompt_AddMatch(ctx, "outline");
                Prompt_AddMatch(ctx, "minimal");
            } else if (!strcmp(cmd, "smoothing_mode")) {
                Prompt_AddMatch(ctx, "linear");
                Prompt_AddMatch(ctx, "quadratic");
                Prompt_AddMatch(ctx, "cubic");
                Prompt_AddMatch(ctx, "sine");
                Prompt_AddMatch(ctx, "exponential");
            }
        } else if (!strcmp(subcmd, "ups")) {
            if (!strcmp(cmd, "shadow") || !strcmp(cmd, "hide_zero")) {
                Prompt_AddMatch(ctx, "0");
                Prompt_AddMatch(ctx, "1");
            } else if (!strcmp(cmd, "color_mode")) {
                Prompt_AddMatch(ctx, "dynamic");
                Prompt_AddMatch(ctx, "static");
                Prompt_AddMatch(ctx, "threshold");
                Prompt_AddMatch(ctx, "rainbow");
                Prompt_AddMatch(ctx, "gradient");
                Prompt_AddMatch(ctx, "strafing");
            } else if (!strcmp(cmd, "format")) {
                Prompt_AddMatch(ctx, "plain");
                Prompt_AddMatch(ctx, "suffix");
                Prompt_AddMatch(ctx, "prefix");
            }
        }
    }
}

void SH_Cmd_f(void) {
    const char *subcmd = Cmd_Argv(1); // First argument after "sh"

    if (!subcmd || !subcmd[0]) {
        SH_Help_f();
        return;
    }

    if (!strcmp(subcmd, "hud")) {
        // Forward to strafe_helper commands
        const char *cmd = Cmd_Argv(2); // Command after "sh hud"
        if (!cmd || !cmd[0]) {
            SH_Hud_Help_f();
            return;
        }

        if (!strcmp(cmd, "enable"))
            SH_Enable_f();
        else if (!strcmp(cmd, "disable"))
            SH_Disable_f();
        else if (!strcmp(cmd, "scale"))
            SH_Scale_f();
        else if (!strcmp(cmd, "ypos"))
            SH_ypos_f();
        else if (!strcmp(cmd, "height"))
            SH_Height_f();
        else if (!strcmp(cmd, "alpha"))
            SH_Alpha_f();
        else if (!strcmp(cmd, "bar_style"))
            SH_BarStyle_f();
        else if (!strcmp(cmd, "smoothing"))
            SH_Smoothing_f();
        else if (!strcmp(cmd, "smoothing_mode"))
            SH_SmoothingMode_f();
        else if (!strcmp(cmd, "center_marker"))
            SH_CenterMarker_f();
        else if (!strcmp(cmd, "center_width"))
            SH_CenterWidth_f();
        else if (!strcmp(cmd, "optimal_width"))
            SH_OptimalWidth_f();
        else if (!strcmp(cmd, "color_accelerating"))
            SH_Color_Accel_f();
        else if (!strcmp(cmd, "color_optimal"))
            SH_Color_Optimal_f();
        else if (!strcmp(cmd, "color_centermarker"))
            SH_Color_CenterMarker_f();
        else if (!strcmp(cmd, "preset"))
            SH_SetPreset_f();
        else if (!strcmp(cmd, "status"))
            SH_Status_f();
        else if (!strcmp(cmd, "help"))
            SH_Hud_Help_f();
        else
            Com_Printf("Unknown hud command. Use 'sh hud' for a list of commands.\n");
    } else if (!strcmp(subcmd, "ups")) {
        const char *cmd = Cmd_Argv(2);
        if (!cmd || !cmd[0]) {
            SH_Ups_Help_f();
            return;
        }
        if (!strcmp(cmd, "enable"))
            SH_Ups_Enable_f();
        else if (!strcmp(cmd, "disable"))
            SH_Ups_Disable_f();
        else if (!strcmp(cmd, "toggle"))
            SH_Ups_Toggle_f();
        else if (!strcmp(cmd, "status"))
            SH_Ups_Status_f();
        else if (!strcmp(cmd, "scale"))
            SH_Ups_Scale_f();
        else if (!strcmp(cmd, "ypos"))
            SH_Ups_Ypos_f();
        else if (!strcmp(cmd, "shadow"))
            SH_Ups_Shadow_f();
        else if (!strcmp(cmd, "hide_zero"))
            SH_Ups_HideZero_f();
        else if (!strcmp(cmd, "color_mode"))
            SH_Ups_ColorMode_f();
        else if (!strcmp(cmd, "color_gain"))
            SH_Ups_ColorGain_f();
        else if (!strcmp(cmd, "color_loss"))
            SH_Ups_ColorLoss_f();
        else if (!strcmp(cmd, "color_neutral"))
            SH_Ups_ColorNeutral_f();
        else if (!strcmp(cmd, "format"))
            SH_Ups_Format_f();
        else if (!strcmp(cmd, "help"))
            SH_Ups_Help_f();
        else
            Com_Printf("Unknown ups command. Use 'sh ups' for a list of commands.\n");
    } else if (!strcmp(subcmd, "status")) {
        SH_Status_f();
    } else {
        SH_Help_f();
    }
}

static const char *SH_DefaultString(cvar_t *var)
{
    return var && var->default_string ? var->default_string : "?";
}

static void SH_PrintStatusInt(const char *label, cvar_t *var)
{
    Com_Printf("  %-20s : %-20d : def: %s\n",
               label, var ? var->integer : 0, SH_DefaultString(var));
}

static void SH_PrintStatusFloat(const char *label, cvar_t *var)
{
    Com_Printf("  %-20s : %-20.2f : def: %s\n",
               label, var ? var->value : 0.0f, SH_DefaultString(var));
}

static void SH_PrintStatusString(const char *label, cvar_t *var)
{
    Com_Printf("  %-20s : %-20s : def: %s\n",
               label, var ? var->string : "?", SH_DefaultString(var));
}

void SH_Status_f(void) {
    Com_Printf("------------------------------------------------------------------\n");
    Com_LPrintf(PRINT_WARNING, "                        Strafe Helper Status:\n");
    Com_Printf("------------------------------------------------------------------\n");
    SH_PrintStatusInt("Enabled", cl_drawStrafeHelper);
    SH_PrintStatusInt("Center marker", cl_strafeHelperCenterMarker);
    SH_PrintStatusFloat("Y pos", cl_strafeHelperY);
    SH_PrintStatusFloat("Scale", cl_strafeHelperScale);
    SH_PrintStatusInt("Height", cl_strafeHelperHeight);
    SH_PrintStatusFloat("Alpha", cl_strafehelperAlpha);
    SH_PrintStatusString("Bar style", cl_strafehelperBarStyle);
    SH_PrintStatusFloat("Smoothing", cl_strafehelperSmoothing);
    SH_PrintStatusInt("Smoothing mode", cl_strafehelperSmoothingMode);
    SH_PrintStatusFloat("Center width", cl_strafehelper_center_width);
    SH_PrintStatusFloat("Optimal width", cl_strafehelper_optimal_width);
    SH_PrintStatusInt("Optimal outline", cl_strafehelper_optimal_outline);
    SH_PrintStatusString("Accelerating color", cl_strafehelper_color_accelerating);
    SH_PrintStatusString("Optimal color", cl_strafehelper_color_optimal);
    SH_PrintStatusString("Center marker color", cl_strafehelper_color_centermarker);
    Com_Printf("------------------------------------------------------------------\n");
    Com_LPrintf(PRINT_WARNING, "                        Center UPS Status:\n");
    Com_Printf("------------------------------------------------------------------\n");
    SH_PrintStatusInt("Enabled", cl_strafehelperUps);
    SH_PrintStatusFloat("Scale", cl_strafehelperUpsScale);
    SH_PrintStatusFloat("Y pos", cl_strafehelperUpsY);
    SH_PrintStatusInt("Shadow", cl_strafehelperUpsShadow);
    SH_PrintStatusInt("Hide zero", cl_strafehelperUpsHideZero);
    SH_PrintStatusString("Color mode", cl_strafehelperUpsColorMode);
    SH_PrintStatusString("Format", cl_strafehelperUpsFormat);
    SH_PrintStatusString("Gain color", cl_strafehelperUpsColorGain);
    SH_PrintStatusString("Loss color", cl_strafehelperUpsColorLoss);
    SH_PrintStatusString("Neutral color", cl_strafehelperUpsColorNeutral);
    Com_Printf("------------------------------------------------------------------\n");
}
