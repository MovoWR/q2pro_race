#include "sh_menus.h"
#include <src/client/client.h>
#include "strafe_helper_customization.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>


typedef struct {
    const char* name;
    const char* color_optimal;
    const char* color_accelerating;
    const char* color_centermarker;
    const char* description;
} ColorPreset;

static const ColorPreset presets[] = {
    {"Vanilla", "0 255 64 192", "0 128 32 96", "255 255 255 192", "Classic"},
    {"Gold", "255 215 0 255", "184 134 11 90", "0 0 255 255", "Rich gold with deep blue contrasts"},
    {"Rainbow", "255 0 0 255", "0 255 0 80", "0 0 255 255", "Vivid RGB rainbow colors"},
    {"FeverDream", "255 0 255 255", "0 255 255 80", "255 255 0 255", "Magenta, cyan, and yellow overload"},
    {"Nightmare", "255 0 128 255", "128 0 255 90", "0 0 0 255", "Bold magenta and purple on black"},
    {"Ocean", "0 128 255 255", "0 64 128 80", "128 192 255 255", "Ocean blue with soft highlights"},
    {"Retro", "0 255 128 255", "255 128 0 80", "255 0 128 255", "Neon green, orange, and pink"},
    {"Amber", "255 128 64 255", "255 64 128 90", "255 255 192 255", "Warm amber tones with subtle highlights"},
    {"SkyBlue", "135 206 235 255", "70 130 180 100", "255 0 128 255", "Cool sky blue with contrasting pink highlights"},
    {"Candy", "255 182 193 255", "255 105 180 90", "255 255 0 255", "Bright pinks with yellow"},
    {"Cyberpunk", "255 20 147 255", "0 255 255 80", "0 0 255 255", "Bright pink and cyan for a futuristic feel"},
    {"Party", "255 255 0 255", "0 255 255 80", "255 0 255 255", "Yellow, cyan, and magenta"},
    {"Aussie", "255 215 0 255", "0 128 0 128", "255 255 255 255", "Golden wattle yellow, green, and white"},
    {"Pepega", "0 255 0 255", "128 128 128 128", "255 0 255 255", "Bright green with muted gray and magenta"},
    {"Toxic", "0 255 0 255", "255 0 255 80", "0 0 0 255", "Toxic green with magenta and black"},
    {"Pixelated", "0 0 0 255", "255 255 255 90", "0 255 255 255", "Black, white, and cyan blocks"},
    {"Forest", "0 128 0 255", "0 64 0 80", "128 192 0 255", "Earthy green tones"},
    {"Radioactive", "0 255 0 255", "0 128 0 90", "255 0 255 255", "Neon green, dark green, and magenta"},
    {"Coral", "255 127 80 255", "255 69 0 80", "0 0 255 255", "Warm coral tones with vivid blue accents"},
    {"Funky", "255 0 255 255", "0 255 0 80", "255 165 0 255", "Magenta, lime green, and orange"},
    {"Electric", "0 0 255 255", "255 255 0 80", "255 0 255 255", "Blue, yellow, and magenta sparks"},
    {"Hudslut", "255 0 0 255", "0 255 0 255", "0 0 255 255", "Bold red, green, and blue for maximum contrast"},
    {"Sand", "194 178 128 255", "153 138 102 90", "255 244 224 255", "Muted sandy tones with soft highlights"},
    {"Fire", "255 69 0 255", "128 32 0 90", "255 255 0 255", "Fiery red and orange with a yellow highlight"},
    {"Mint", "152 255 152 255", "0 204 153 80", "255 0 0 255", "Refreshing mint green with bold red highlights"},
    {"Karen", "255 0 255 255", "255 255 255 128", "0 0 0 255", "Magenta, bright white, and black"},
    {"Nuclear", "0 255 0 255", "255 255 0 90", "255 0 0 255", "Toxic green, yellow, and red explosion"},
    {"Highlighter", "255 255 0 255", "255 0 255 90", "0 255 255 255", "Neon yellow, magenta, and cyan"},
    {"Cursed", "0 0 0 255", "255 0 255 255", "255 255 255 255", "Dark black, spooky magenta, and haunting white"},
    {"Zebra", "255 255 255 255", "0 0 0 80", "128 128 128 255", "Black, white, and gray"},
    {"Ice", "0 255 255 255", "0 128 255 80", "255 255 255 255", "Cool icy cyan and white"},
    {"Imposs", "0 0 0 0", "0 0 0 0", "0 0 0 0", "?"},
    {"Clown", "255 0 0 255", "0 0 255 80", "255 255 0 255", "Red, blue, and yellow for a playful feel"},
    {"Acid", "102 255 0 255", "255 255 0 90", "0 255 128 255", "Acidic green and yellow with teal"},
    {"Cherry", "255 0 0 255", "128 0 0 80", "255 128 128 255", "Deep cherry red with soft pink accents"},
    {"Venom", "0 0 0 255", "0 255 0 80", "255 0 0 255", "Black, neon green, and blood red"},
    {"Psycho", "255 0 0 255", "255 255 0 90", "0 0 255 255", "Red, yellow, and blue for mind-bending contrast"},
    {"Doge", "255 223 0 255", "0 191 255 255", "255 0 128 255", "Golden yellow, sky blue, and hot pink for much wow"},
    {"Lava", "255 69 0 255", "255 140 0 80", "255 0 255 255", "Fiery red and orange with magenta"},
    {"ToxicSunset", "255 128 0 255", "255 0 128 90", "0 255 0 255", "Orange, pink, and green madness"},
    {"Shadow", "64 64 64 255", "32 32 32 80", "192 192 192 255", "Neutral gray with shadowy tones"},
    {"Lavender", "230 230 250 255", "216 191 216 90", "0 255 0 255", "Soft lavender tones with bright green"},
    {"Emerald", "80 200 120 255", "40 100 60 80", "200 255 200 255", "Lush emerald green with light highlights"},
    {"FeverDream", "255 0 255 255", "0 255 255 80", "255 255 0 255", "Magenta, cyan, and yellow overload"}
};

void SH_SetPreset_f(void)
{
    if (Cmd_Argc() < 4)
    {
        Com_Printf("Available presets:\n");
        Com_Printf("===============================================================================\n");
        Com_Printf("| %-5s | %-15s | %-50s |\n", "Num", "Name", "Description");
        Com_Printf("===============================================================================\n");

        for (int i = 0; i < sizeof(presets) / sizeof(presets[0]); i++)
        {
            Com_Printf("| %-5d | %-15s | %-50s |\n", i + 1, presets[i].name, presets[i].description);
            Com_Printf("-------------------------------------------------------------------------------\n");
        }

        Com_Printf("| %-5s | %-15s | %-50s |\n", "N/A", "random", "Apply a random preset.");
        Com_Printf("===============================================================================\n");
        Com_Printf("Usage: 'sh hud preset <name_or_number>'\n");

        return;
    }


    const char* selectedpreset = Cmd_Argv(3);

    // Handle random preset option
    if (!_stricmp(selectedpreset, "random"))
    {
        srand((unsigned int)time(NULL));
        int randomIndex = rand() % (sizeof(presets) / sizeof(presets[0])); // Get random index
        Cvar_Set("sh_color_optimal", presets[randomIndex].color_optimal);
        Cvar_Set("sh_color_accelerating", presets[randomIndex].color_accelerating);
        Cvar_Set("sh_color_centermarker", presets[randomIndex].color_centermarker);
        Com_Printf("Random preset applied: '%s' - %s\n", presets[randomIndex].name, presets[randomIndex].description);
        return;
        }

    // Check if input is a number
    int presetNumber = -1;
    if (sscanf(selectedpreset, "%d", &presetNumber) == 1)
    {
        if (presetNumber >= 1 && presetNumber <= sizeof(presets) / sizeof(presets[0]))
        {
            int index = presetNumber - 1; // Convert to 0-based index
            Cvar_Set("sh_color_optimal", presets[index].color_optimal);
            Cvar_Set("sh_color_accelerating", presets[index].color_accelerating);
            Cvar_Set("sh_color_centermarker", presets[index].color_centermarker);
            Com_Printf("Preset '%s' applied: %s\n", presets[index].name, presets[index].description);
            return;
        }
        else
        {
            Com_Printf("Invalid preset number. Use 'sh hud preset' to see available options.\n");
        return;
    }
    }

    // Check if input matches a preset name
    for (int i = 0; i < sizeof(presets) / sizeof(presets[0]); i++)
    {
        if (!_stricmp(selectedpreset, presets[i].name))
        {
            Cvar_Set("sh_color_optimal", presets[i].color_optimal);
            Cvar_Set("sh_color_accelerating", presets[i].color_accelerating);
            Cvar_Set("sh_color_centermarker", presets[i].color_centermarker);
            Com_Printf("Preset '%s' applied: %s\n", presets[i].name, presets[i].description);
            return;
        }
    }

    Com_Printf("Invalid preset name or number. Use 'sh hud preset' to see available options.\n");
}


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
    Com_Printf("  palette <name>                           Apply a predefined color palette.\n");
    Com_Printf("========================================================================================\n");
}
