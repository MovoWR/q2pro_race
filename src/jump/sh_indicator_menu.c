#include "strafe_helper.h"
#include <src/client/client.h>
#include "strafe_helper_customization.h"
#include <math.h>

void SH_Indicator_Enable_f(void)
{
    Cvar_Set("sh_indicator", "1");
    Com_Printf("Indicator enabled.\n");
}

void SH_Indicator_Disable_f(void)
{
    Cvar_Set("sh_indicator", "0");
    Com_Printf("Indicator disabled.\n");
}

void SH_Indicator_SetPos_f(void)
{
    const char* pos = Cmd_ArgsFrom(3);
    float x, y;
    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("%s : %s \n", " Indicator Position", cl_strafehelper_indicator_pos->string);
        return;
    }
    if (sscanf(pos, "%f %f", &x, &y) == 2)
    {
        Cvar_Set("sh_indicator_pos", pos);
        Com_Printf("Indicator position set to: %s\n", pos);
    }
    else
    {
        Com_Printf("Invalid position format. Usage: sh indicator pos <X> <Y>\n");
    }
}

void SH_Indicator_SetSize_f(void)
{
    const char* size = Cmd_ArgsFrom(3);
    float x, y;
    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("%s : %s \n", " Indicator Size", cl_strafehelper_indicator_size->string);
        return;
    }

    if (sscanf(size, "%f %f", &x, &y) == 2)
    {
        Cvar_Set("sh_indicator_size", size);
        Com_Printf("Indicator size set to: %s\n", size);
    }
    else
    {
        Com_Printf("Invalid size format. Usage: sh indicator size <X> <Y>\n");
    }
}

void SH_Indicator_SetColor_f(void)
{
    const char* color = Cmd_ArgsFrom(3);
    int r, g, b, a;

    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("- Current Color: %s\n", cl_strafehelper_color_indicator->string);
        return;
    }

    // Parse the color arguments
    if (sscanf(color, "%d %d %d %d", &r, &g, &b, &a) == 4 &&
        r >= 0 && r <= 255 && g >= 0 && g <= 255 &&
        b >= 0 && b <= 255 && a >= 0 && a <= 255)
    {
        Cvar_Set("sh_color_indicator", color);
        Com_Printf("Indicator color set to: %s\n", color);
    }
    else
    {
        Com_Printf("Invalid color format. Use 'sh indicator color <R> <G> <B> <A>' (0-255 each)\n");
    }
}

void SH_Indicator_SetPic_f(void)
{
    if (Cmd_Argc() <= 3)
    {
        Com_Printf("[ERROR] Not enough arguments. Usage: sh indicator pic <1-9|off>\n");
        return;
    }
    const char* arg = Cmd_Argv(3); // Corrected to Cmd_Argv(3)
    if (!strcmp(arg, "off"))
    {
        Cvar_Set("sh_indicator", "1");
        Com_Printf("Indicator picture mode turned off. Reverting to default drawing.\n");
    }
    else
    {
        int pic = Q_atoi(arg);
        if (pic < 1 || pic > 9)
        {
            Com_Printf("[ERROR] Invalid picture index. Usage: sh indicator pic <1-9|off>\n");
            return;
        }

        Cvar_Set("sh_indicator", "2");
        char picStr[8];
        snprintf(picStr, sizeof(picStr), "%d", pic);
        Cvar_Set("scr_indicator", picStr);
        Com_Printf("Indicator picture set to %d.\n", pic);
    }
}


void SH_Indicator_Tolerance_f(void)
{
    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("- Indicator tolerance: %.2f\n", cl_strafehelper_tolerance->value);
        return;
    }
    const char* arg = Cmd_Argv(3);
    float tolerance = 0.0f;
    int parsed = sscanf(arg, "%f", &tolerance);

    if (parsed != 1)
    {
        return;
    }
    // Validate the tolerance range
    if (tolerance < 0.0f || tolerance > 10.0f)
    {
        return;
    }
    // Set the cvar to the provided tolerance value
    char toleranceStr[16];
    snprintf(toleranceStr, sizeof(toleranceStr), "%.2f", tolerance);
    Cvar_Set("sh_indicator_tolerance", toleranceStr);

    Com_Printf("Indicator tolerance set to %.2f degrees.\n", tolerance);
}


void SH_Indicator_Help(void)
{
    Com_Printf("========================================================================================\n");
    Com_LPrintf(PRINT_WARNING, "Usage: sh indicator <command> [options]\n");
    Com_LPrintf(PRINT_WARNING, "Commands:\n");
    Com_Printf("========================================================================================\n");
    Com_Printf("  enable           Enable the indicator.\n");
    Com_Printf("  disable          Disable the indicator.\n");
    Com_Printf("  tolerance        Set tolerance (degrees) from optimal angle .\n");
    Com_Printf("  pos X Y          Set position. Example: pos 100 200\n");
    Com_Printf("  size W H         Set size. Example: size 50 50\n");
    Com_Printf("  color R G B A    Set color (0-255). Example: color 255 255 255 128\n");
    Com_Printf("  pic <1-9|off>    Set picture index or turn off. Example: pic 2\n");
    Com_Printf("========================================================================================\n");
}

void SH_Indicator_Cmd_f(void)
{
    const char* cmd = Cmd_Argv(1);

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
}
