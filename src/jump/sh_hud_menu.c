#include "strafe_helper.h"
#include <src/client/client.h>
#include "strafe_helper_customization.h"
#include <math.h>

void SH_Enable_f(void)
{
    Cvar_Set("sh_draw", "1");
    Com_Printf("Strafe helper enabled.\n");
}

void SH_Disable_f(void)
{
    Cvar_Set("sh_draw", "0");
    Com_Printf("Strafe helper disabled.\n");
}

void SH_Scale_f(void)
{
    const char* scale = Cmd_ArgsFrom(3);
    float value;
    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("- Scale: %.2f\n", cl_strafeHelperScale->value);
        return;
    }
    if (sscanf(scale, "%f", &value) == 1 && value > 0.0f)
    {
        Cvar_Set("sh_scale", scale);
        Com_Printf("Strafe helper scale set to: %s\n", scale);
    }
    else
    {
        Com_EPrintf("Invalid scale value. Usage: 'sh hud scale <value>'\n");
    }
}

void SH_ypos_f(void)
{
    const char* ypos = Cmd_ArgsFrom(3);
    float value;

    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("- Y pos: %.2f\n", cl_strafeHelperY->value);
        return;
    }

    if (sscanf(ypos, "%f", &value) == 1 && value > 0.0f)
    {
        Cvar_Set("sh_y", ypos);
        Com_Printf("Strafe helper Y position set to: %s\n", ypos);
    }
    else
    {
        Com_EPrintf("Invalid Y position value. Usage: 'sh hud ypos <value>'\n");
    }
}

void SH_Height_f(void)
{
    const char* height = Cmd_ArgsFrom(3);
    int value;

    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("- Height: %d\n", cl_strafeHelperHeight->integer);
        return;
    }

    if (sscanf(height, "%d", &value) == 1 && value > 0)
    {
        Cvar_Set("sh_height", height);
        Com_Printf("Strafe helper height set to: %s\n", height);
    }
    else
    {
        Com_Printf("Invalid height value. Usage: 'sh hud height <value>' \n");
    }
}

void SH_CenterMarker_f(void)
{
    Cvar_Set("sh_centermarker", cl_strafeHelperCenterMarker->integer ? "0" : "1");
    Com_Printf("Center marker %s.\n", cl_strafeHelperCenterMarker->integer ? "disabled" : "enabled");
}

void SH_CenterWidth_f(void)
{
    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("- Current Center Width: %f\n", cl_strafehelper_center_width->value);
        return;
    }
    const char* width = Cmd_Argv(3); // Fetch the second argument
    float value;
    if (sscanf(width, "%f", &value) == 1 && value >= 0.1f && value <= 5.0f)
    {
        Cvar_Set("sh_center_width", width);
        Com_Printf("Center line width set to: %s\n", width);
    }
    else
    {
        Com_Printf("Invalid Center line width value. Usage: 'sh hud center_width <value>' (0.1-5.0)\n");
    }
}

void SH_OptimalWidth_f(void)
{
    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("- Current Center Width: %.2f\n", cl_strafehelper_optimal_width->value);
        return;
    }
    const char* width = Cmd_ArgsFrom(3);
    float value;
    if (sscanf(width, "%f", &value) == 1 && value > 0.0f)
    {
        Cvar_Set("sh_optimal_width", width);
        Com_Printf("Optimal line width set to: %s\n", width);
    }
    else
    {
        Com_Printf("Invalid optimal line width value. Usage: 'sh hud optimal_width <value>' (0.1-5.0)\n");
    }
}

void SH_Color_Accel_f(void)
{
    const char* color = Cmd_ArgsFrom(3);
    int r, g, b, a;

    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("- Accelerating color: %s\n", cl_strafehelper_color_accelerating->string);
        return;
    }

    if (sscanf(color, "%d %d %d %d", &r, &g, &b, &a) == 4 &&
        r >= 0 && r <= 255 && g >= 0 && g <= 255 &&
        b >= 0 && b <= 255 && a >= 0 && a <= 255)
    {
        Cvar_Set("sh_color_accelerating", color);
        Com_Printf("Accelerating color set to: %s\n", color);
    }
    else
    {
        Com_Printf("Wrong input. Use 'sh hud color_accelerating R G B A'\n");
    }
}

void SH_Color_Optimal_f(void)
{
    const char* color = Cmd_ArgsFrom(3);
    int r, g, b, a;

    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("- Optimal color: %s\n", cl_strafehelper_color_optimal->string);
        return;
    }

    if (sscanf(color, "%d %d %d %d", &r, &g, &b, &a) == 4 &&
        r >= 0 && r <= 255 && g >= 0 && g <= 255 &&
        b >= 0 && b <= 255 && a >= 0 && a <= 255)
    {
        Cvar_Set("sh_color_optimal", color);
        Com_Printf("Optimal color changed to: %s\n", color);
    }
    else
    {
        Com_Printf("Wrong input. Use 'sh hud color_optimal R G B A'\n");
    }
}

void SH_Color_CenterMarker_f(void)
{
    const char* color = Cmd_ArgsFrom(3);
    int r, g, b, a;

    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("- Center marker color: %s\n", cl_strafehelper_color_centermarker->string);
        return;
    }

    if (sscanf(color, "%d %d %d %d", &r, &g, &b, &a) == 4 &&
        r >= 0 && r <= 255 && g >= 0 && g <= 255 &&
        b >= 0 && b <= 255 && a >= 0 && a <= 255)
    {
        Cvar_Set("sh_color_centermarker", color);
        Com_Printf("Center marker color set to: %s\n", color);
    }
    else
    {
        Com_Printf("Wrong input. Use 'sh hud color_centermarker R G B A'\n");
    }
}

void SH_Hud_Help_f(void)
{
    Com_Printf("========================================================================================\n");
    Com_LPrintf(PRINT_WARNING, "Usage: sh <command> [options]\n");
    Com_LPrintf(PRINT_WARNING, "Commands:\n");
    Com_Printf("========================================================================================\n");
    Com_Printf("  enable                                  Enable the strafe helper.\n");
    Com_Printf("  disable                                 Disable the strafe helper.\n");
    Com_Printf("  scale <value>                           Set the scale (default: 1.5).\n");
    Com_Printf("  ypos <value>                            Set the Y position (default: 100).\n");
    Com_Printf("  height <value>                          Set the height (default: 20).\n");
    Com_Printf("  centermarker                            Toggle the center marker on/off.\n");
    Com_Printf(
        "  center_width <value>                    Set the center line width (default: 2.\n");
    Com_Printf(
        "  optimal_width <value>                   Set the optimal line width (default: 2).\n");
    Com_Printf("  color_<type> <R> <G> <B> <A>            Set color (0-255).\n");
    Com_Printf("                                          Types: optimal, centermarker, accelerating\n");
    Com_Printf("========================================================================================\n");
}
