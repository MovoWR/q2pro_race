#include "sh_menus.h"
#include <src/client/client.h>
#include "strafe_helper_customization.h"
#include <math.h>

void SH_Help_f(void)
{
    Com_Printf("========================================================================================\n");
    Com_LPrintf(PRINT_WARNING,"Usage: sh <subcommand> <command> [options]\n");
    Com_LPrintf(PRINT_WARNING,"Subcommands:\n");
    Com_Printf("========================================================================================\n");
    Com_Printf("  hud                  Commands for the strafe helper.\n");
    Com_Printf("                       Use 'sh hud' for more details.\n");
    Com_Printf("  indicator            Commands for the indicator.\n");
    Com_Printf("                       Use 'sh indicator' for more details.\n");
    Com_Printf("  status                Show all set  parem\n");
    Com_Printf("========================================================================================\n");
    Com_LPrintf(PRINT_WARNING,"Other commands:\n");
    Com_Printf("========================================================================================\n");
    Com_Printf("  sh_nerdstats         1 - display on left, 2 - display on right \n");
    Com_Printf("  race_width           Set the thickness of the race line. Values: 1 to 20 \n");
    Com_Printf("  race_color           <R> <G> <B> (0-255 each)\n");
    Com_Printf("  race_alpha           Set transparency level. Values: 0 to 1.\n");
    Com_Printf("  race_life            Define how long the race line remains visible.Values: 100 to 5000.\n");
    Com_Printf("  gl_beamstyle         0/1 - 1 new beam style.\n");
}


void SH_Cmd_g(genctx_t* ctx, int argnum)
{
    R_SetColor(U32_RED);
    if (argnum == 1)
    {
        Prompt_AddMatch(ctx, "hud");
        Prompt_AddMatch(ctx, "indicator");
    }
    else if (argnum == 2)
    {
        const char* subcmd = Cmd_Argv(1);
        if (!strcmp(subcmd, "hud"))
        {
            Prompt_AddMatch(ctx, "enable");
            Prompt_AddMatch(ctx, "disable");
            Prompt_AddMatch(ctx, "center_marker");
            Prompt_AddMatch(ctx, "scale");
            Prompt_AddMatch(ctx, "ypos");
            Prompt_AddMatch(ctx, "height");
            Prompt_AddMatch(ctx, "center_width");
            Prompt_AddMatch(ctx, "optimal_width");
            Prompt_AddMatch(ctx, "color_accelerating");
            Prompt_AddMatch(ctx, "color_optimal");
            Prompt_AddMatch(ctx, "preset");
            Prompt_AddMatch(ctx, "status");
            Prompt_AddMatch(ctx, "help");
        }
        else if (!strcmp(subcmd, "indicator"))
        {
            Prompt_AddMatch(ctx, "enable");
            Prompt_AddMatch(ctx, "disable");
            Prompt_AddMatch(ctx, "tolerance");
            Prompt_AddMatch(ctx, "pos");
            Prompt_AddMatch(ctx, "size");
            Prompt_AddMatch(ctx, "color");
            Prompt_AddMatch(ctx, "pic");
            Prompt_AddMatch(ctx, "help");
        }
        else if (!strcmp(subcmd, "status"))
        {
            Prompt_AddMatch(ctx, "status");
        }
    }
}

void SH_Cmd_f(void)
{
    const char* subcmd = Cmd_Argv(1); // First argument after "sh"

    if (!subcmd || !subcmd[0])
    {
        SH_Help_f();
        return;
    }

    if (!strcmp(subcmd, "hud"))
    {
        // Forward to strafe_helper commands
        const char* cmd = Cmd_Argv(2); // Command after "sh hud"
        if (!cmd || !cmd[0])
        {
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
        else if (!strcmp(cmd, "help"))
            SH_Hud_Help_f();
        else
            Com_Printf("Unknown hud command. Use 'sh hud' for a list of commands.\n");
    }
    else if
    (!strcmp(subcmd, "indicator"))
    {
        // Forward to indicator commands
        const char* cmd = Cmd_Argv(2); // Command after "sh indicator"
        if (!cmd || !cmd[0])
        {
            SH_Indicator_Help();
            return;
        }
        if (!strcmp(cmd, "enable"))
            SH_Indicator_Enable_f();
        else if (!strcmp(cmd, "disable"))
            SH_Indicator_Disable_f();
        else if (!strcmp(cmd, "tolerance"))
            SH_Indicator_Tolerance_f();
        else if (!strcmp(cmd, "pos"))
            SH_Indicator_SetPos_f();
        else if (!strcmp(cmd, "size"))
            SH_Indicator_SetSize_f();
        else if (!strcmp(cmd, "color"))
            SH_Indicator_SetColor_f();
        else if (!strcmp(cmd, "pic"))
            SH_Indicator_SetPic_f();
        else if (!strcmp(cmd, "help"))
            SH_Indicator_Help();
        else
            Com_Printf("Unknown indicator command. Use 'sh indicator' for a list of commands.\n");
    }
    else if
    (!strcmp(subcmd, "status"))
        {
        SH_Status_f();
        }
    else
    {
        SH_Help_f();
    }
}

void SH_Status_f(void)
{

    Com_Printf("------------------------------------------------------------------\n");
    Com_LPrintf(PRINT_WARNING, "                        Strafe Helper Status:\n");
    Com_Printf("------------------------------------------------------------------\n");
    Com_Printf("  %-20s : %-20d : def: %d\n", "Enabled", cl_drawStrafeHelper->integer, 1);
    Com_Printf("  %-20s : %-20d : def: %d\n", "Center marker", cl_strafeHelperCenterMarker->integer, 1);
    Com_Printf("  %-20s : %-20.2f : def: %.2f\n", "Y pos", cl_strafeHelperY->value, 100.0f);
    Com_Printf("  %-20s : %-20.2f : def: %.2f\n", "Scale", cl_strafeHelperScale->value, 1.5f);
    Com_Printf("  %-20s : %-20d : def: %d\n", "Height", cl_strafeHelperHeight->integer, 25);
    Com_Printf("  %-20s : %-20.2f : def: %.2f\n", "Center width", cl_strafehelper_center_width->value, 1.5f);
    Com_Printf("  %-20s : %-20.2f : def: %.2f\n", "Optimal width", cl_strafehelper_optimal_width->value, 1.5f);
    Com_Printf("  %-20s : %-20s : def: %s\n", "Accelerating color", cl_strafehelper_color_accelerating->string, "115 170 255 120");
    Com_Printf("  %-20s : %-20s : def: %s\n", "Optimal color", cl_strafehelper_color_optimal->string, "0 255 64 192");
    Com_Printf("  %-20s : %-20s : def: %s\n", "Center marker color", cl_strafehelper_color_centermarker->string, "255 255 255 192");
    Com_Printf("------------------------------------------------------------------\n");
    Com_LPrintf(PRINT_WARNING, "                        Indicator Status:\n");
    Com_Printf("------------------------------------------------------------------\n");
    Com_Printf("  %-20s : %-20d : def: %d\n", "Enabled", cl_strafehelperIndicator->integer, 0);
    Com_Printf("  %-20s : %-20.2f : def: %.2f\n", "Tolerance", cl_strafehelper_tolerance->value, 0.20f);
    Com_Printf("  %-20s : %-20s : def: %s\n", "Position", cl_strafehelper_indicator_pos->string, "0 0");
    Com_Printf("  %-20s : %-20s : def: %s\n", "Size", cl_strafehelper_indicator_size->string, "10 5");
    Com_Printf("  %-20s : %-20s : def: %s\n", "Color", cl_strafehelper_color_indicator->string, "255 255 255 255");
    if (cl_strafehelperIndicator->integer == 1)
    {
        Com_Printf("  %-20s : %-20s : def: %s\n", "Indicator pic", "off", "1");
    }
    else if (cl_strafehelperIndicator->integer == 2)
    {
        Com_Printf("  %-20s : %-20d : def: %d\n", "Indicator pic", scr_indicator ? scr_indicator->integer : 0, 1);
    }
    Com_Printf("------------------------------------------------------------------\n");
}

/* Palletes
{ //Citrus Fresh Palette
sh hud color_optimal 255 255 0 255;
sh hud color_accelerating 255 128 0 80;
sh hud color_centermarker 128 255 0 255;
}
{
sh hud color_optimal 255 128 0 255;
sh hud color_accelerating 0 255 128 80;
sh hud color_centermarker 255 255 255 255;
}
{//sunset
sh hud color_optimal 255 128 64 255;
sh hud color_accelerating 255 64 128 90;
sh hud color_centermarker 255 255 192 255;
}
{ //Tech glow
sh hud color_optimal 0 255 0 255;
sh hud color_accelerating 0 128 255 80;
sh hud color_centermarker 255 255 255 255;
}
*/

