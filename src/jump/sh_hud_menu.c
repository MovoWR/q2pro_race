#include "sh_menus.h"
#include <src/client/client.h>
#include "strafe_helper_customization.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>


typedef struct {
    const char *name;
    const char *color_optimal;
    const char *color_accelerating;
    const char *color_centermarker;
    const char *description;
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
};

void SH_SetPreset_f(void) {
    if (Cmd_Argc() < 4) {
        Com_Printf("Available presets:\n");
        Com_Printf("===============================================================================\n");
        Com_Printf("| %-5s | %-15s | %-50s |\n", "Num", "Name", "Description");
        Com_Printf("===============================================================================\n");

        for (int i = 0; i < sizeof(presets) / sizeof(presets[0]); i++) {
            Com_Printf("| %-5d | %-15s | %-50s |\n", i + 1, presets[i].name, presets[i].description);
            Com_Printf("-------------------------------------------------------------------------------\n");
        }

        Com_Printf("| %-5s | %-15s | %-50s |\n", "N/A", "random", "Apply a random preset.");
        Com_Printf("===============================================================================\n");
        Com_Printf("Usage: 'sh hud preset <name_or_number>'\n");

        return;
    }


    const char *selectedpreset = Cmd_Argv(3);

    // Handle random preset option
    if (!_stricmp(selectedpreset, "random")) {
        srand((unsigned int) time(NULL));
        int randomIndex = rand() % (sizeof(presets) / sizeof(presets[0])); // Get random index
        Cvar_Set("sh_color_optimal", presets[randomIndex].color_optimal);
        Cvar_Set("sh_color_accelerating", presets[randomIndex].color_accelerating);
        Cvar_Set("sh_color_centermarker", presets[randomIndex].color_centermarker);
        Com_Printf("Random preset applied: '%s' - %s\n", presets[randomIndex].name, presets[randomIndex].description);
        return;
    }

    // Check if input is a number
    int presetNumber = -1;
    if (sscanf(selectedpreset, "%d", &presetNumber) == 1) {
        if (presetNumber >= 1 && presetNumber <= sizeof(presets) / sizeof(presets[0])) {
            int index = presetNumber - 1; // Convert to 0-based index
            Cvar_Set("sh_color_optimal", presets[index].color_optimal);
            Cvar_Set("sh_color_accelerating", presets[index].color_accelerating);
            Cvar_Set("sh_color_centermarker", presets[index].color_centermarker);
            Com_Printf("Preset '%s' applied: %s\n", presets[index].name, presets[index].description);
            return;
        } else {
            Com_Printf("Invalid preset number. Use 'sh hud preset' to see available options.\n");
            return;
        }
    }

    // Check if input matches a preset name
    for (int i = 0; i < sizeof(presets) / sizeof(presets[0]); i++) {
        if (!_stricmp(selectedpreset, presets[i].name)) {
            Cvar_Set("sh_color_optimal", presets[i].color_optimal);
            Cvar_Set("sh_color_accelerating", presets[i].color_accelerating);
            Cvar_Set("sh_color_centermarker", presets[i].color_centermarker);
            Com_Printf("Preset '%s' applied: %s\n", presets[i].name, presets[i].description);
            return;
        }
    }

    Com_Printf("Invalid preset name or number. Use 'sh hud preset' to see available options.\n");
}


void SH_Enable_f(void) {
    Cvar_Set("sh_draw", "1");
    Com_Printf("Strafe helper enabled.\n");
}

void SH_Disable_f(void) {
    Cvar_Set("sh_draw", "0");
    Com_Printf("Strafe helper disabled.\n");
}

void SH_Scale_f(void) {
    const char *scale = Cmd_ArgsFrom(3);
    float value;
    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("- Scale: %.2f\n", cl_strafeHelperScale->value);
        return;
    }
    if (sscanf(scale, "%f", &value) == 1 && value > 0.0f) {
        Cvar_Set("sh_scale", scale);
        Com_Printf("Strafe helper scale set to: %s\n", scale);
    } else {
        Com_EPrintf("Invalid scale value. Usage: 'sh hud scale <value>'\n");
    }
}

void SH_ypos_f(void) {
    const char *ypos = Cmd_ArgsFrom(3);
    float value;

    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("- Y pos: %.2f\n", cl_strafeHelperY->value);
        return;
    }

    if (sscanf(ypos, "%f", &value) == 1 && value > 0.0f) {
        Cvar_Set("sh_y", ypos);
        Com_Printf("Strafe helper Y position set to: %s\n", ypos);
    } else {
        Com_EPrintf("Invalid Y position value. Usage: 'sh hud ypos <value>'\n");
    }
}

void SH_Height_f(void) {
    const char *height = Cmd_ArgsFrom(3);
    int value;

    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("- Height: %d\n", cl_strafeHelperHeight->integer);
        return;
    }

    if (sscanf(height, "%d", &value) == 1 && value > 0) {
        Cvar_Set("sh_height", height);
        Com_Printf("Strafe helper height set to: %s\n", height);
    } else {
        Com_Printf("Invalid height value. Usage: 'sh hud height <value>' \n");
    }
}

void SH_CenterMarker_f(void) {
    Cvar_Set("sh_centermarker", cl_strafeHelperCenterMarker->integer ? "0" : "1");
    Com_Printf("Center marker %s.\n", cl_strafeHelperCenterMarker->integer ? "disabled" : "enabled");
}

void SH_CenterWidth_f(void) {
    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("- Current Center Width: %f\n", cl_strafehelper_center_width->value);
        return;
    }
    const char *width = Cmd_Argv(3); // Fetch the second argument
    float value;
    if (sscanf(width, "%f", &value) == 1 && value >= 0.1f && value <= 5.0f) {
        Cvar_Set("sh_center_width", width);
        Com_Printf("Center line width set to: %s\n", width);
    } else {
        Com_Printf("Invalid Center line width value. Usage: 'sh hud center_width <value>' (0.1-5.0)\n");
    }
}

void SH_OptimalWidth_f(void) {
    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("- Current Center Width: %.2f\n", cl_strafehelper_optimal_width->value);
        return;
    }
    const char *width = Cmd_ArgsFrom(3);
    float value;
    if (sscanf(width, "%f", &value) == 1 && value > 0.0f) {
        Cvar_Set("sh_optimal_width", width);
        Com_Printf("Optimal line width set to: %s\n", width);
    } else {
        Com_Printf("Invalid optimal line width value. Usage: 'sh hud optimal_width <value>' (0.1-5.0)\n");
    }
}

static bool SH_IsOnOffValue(const char *value) {
    return value && (!strcmp(value, "0") || !strcmp(value, "1"));
}

static bool SH_IsColorString(const char *color) {
    int r, g, b, a;
    return sscanf(color, "%d %d %d %d", &r, &g, &b, &a) == 4 &&
           r >= 0 && r <= 255 && g >= 0 && g <= 255 &&
           b >= 0 && b <= 255 && a >= 0 && a <= 255;
}

static void SH_SetColorCvar(const char *cvar_name, const char *label, const char *usage) {
    const char *color = Cmd_ArgsFrom(3);

    if (Cmd_Argc() < 4) {
        Com_Printf("- %s: %s\n", label, Cvar_VariableString(cvar_name));
        return;
    }

    if (SH_IsColorString(color)) {
        Cvar_Set(cvar_name, color);
        Com_Printf("%s set to: %s\n", label, color);
    } else {
        Com_Printf("Wrong input. Use '%s'\n", usage);
    }
}

void SH_Alpha_f(void) {
    const char *alpha = Cmd_Argv(3);
    float value;

    if (Cmd_Argc() < 4) {
        Com_Printf("- Alpha: %.2f\n", cl_strafehelperAlpha->value);
        return;
    }

    if (sscanf(alpha, "%f", &value) == 1 && value >= 0.0f && value <= 1.0f) {
        Cvar_Set("sh_alpha", alpha);
        Com_Printf("Strafe helper alpha set to: %s\n", alpha);
    } else {
        Com_Printf("Invalid alpha value. Usage: 'sh hud alpha <0.0-1.0>'\n");
    }
}

void SH_FadeInactive_f(void) {
    const char *value = Cmd_Argv(3);

    if (Cmd_Argc() < 4) {
        Com_Printf("- Fade inactive: %d\n", cl_strafehelperFadeInactive->integer);
        return;
    }

    if (SH_IsOnOffValue(value)) {
        Cvar_Set("sh_fade_inactive", value);
        Com_Printf("Fade inactive %s.\n", cl_strafehelperFadeInactive->integer ? "enabled" : "disabled");
    } else {
        Com_Printf("Invalid value. Usage: 'sh hud fade_inactive <0|1>'\n");
    }
}

void SH_BarStyle_f(void) {
    const char *style = Cmd_Argv(3);

    if (Cmd_Argc() < 4) {
        Com_Printf("- Bar style: %s\n", cl_strafehelperBarStyle->string);
        return;
    }

    if (!Q_stricmp(style, "solid") || !Q_stricmp(style, "gradient") ||
        !Q_stricmp(style, "outline") || !Q_stricmp(style, "minimal")) {
        Cvar_Set("sh_bar_style", style);
        Com_Printf("Bar style set to: %s\n", style);
    } else {
        Com_Printf("Invalid bar style. Usage: 'sh hud bar_style <solid|gradient|outline|minimal>'\n");
    }
}

void SH_Smoothing_f(void) {
    const char *value = Cmd_Argv(3);
    float smoothing;

    if (Cmd_Argc() < 4) {
        Com_Printf("- Smoothing: %.2f\n", cl_strafehelperSmoothing->value);
        return;
    }

    if (sscanf(value, "%f", &smoothing) == 1 && smoothing >= 0.0f && smoothing <= 10.0f) {
        Cvar_Set("sh_smoothing", value);
        Com_Printf("Strafe helper smoothing set to: %s\n", value);
    } else {
        Com_Printf("Invalid smoothing value. Usage: 'sh hud smoothing <0.0-10.0>'\n");
    }
}

void SH_SmoothingMode_f(void) {
    const char *mode = Cmd_Argv(3);
    const char *mode_value = NULL;

    if (Cmd_Argc() < 4) {
        Com_Printf("- Smoothing mode: %s\n", cl_strafehelperSmoothingMode->string);
        return;
    }

    if (!Q_stricmp(mode, "linear") || !strcmp(mode, "1")) {
        mode_value = "1";
    } else if (!Q_stricmp(mode, "quadratic") || !strcmp(mode, "2")) {
        mode_value = "2";
    } else if (!Q_stricmp(mode, "cubic") || !strcmp(mode, "3")) {
        mode_value = "3";
    } else if (!Q_stricmp(mode, "sine") || !strcmp(mode, "4")) {
        mode_value = "4";
    } else if (!Q_stricmp(mode, "exponential") || !strcmp(mode, "5")) {
        mode_value = "5";
    }

    if (mode_value) {
        Cvar_Set("sh_smoothing_mode", mode_value);
        Com_Printf("Smoothing mode set to: %s\n", mode);
    } else {
        Com_Printf("Invalid smoothing mode. Usage: 'sh hud smoothing_mode <linear|quadratic|cubic|sine|exponential>'\n");
    }
}

void SH_Ups_Enable_f(void) {
    Cvar_Set("sh_ups", "1");
    Com_Printf("Center UPS enabled.\n");
}

void SH_Ups_Disable_f(void) {
    Cvar_Set("sh_ups", "0");
    Com_Printf("Center UPS disabled.\n");
}

void SH_Ups_Toggle_f(void) {
    const bool enable = !cl_strafehelperUps->integer;
    Cvar_Set("sh_ups", enable ? "1" : "0");
    Com_Printf("Center UPS %s.\n", enable ? "enabled" : "disabled");
}

void SH_Ups_Status_f(void) {
    Com_Printf("- Center UPS: %s\n", cl_strafehelperUps->integer ? "enabled" : "disabled");
    Com_Printf("- Scale: %.2f\n", cl_strafehelperUpsScale->value);
    Com_Printf("- Y pos: %.2f\n", cl_strafehelperUpsY->value);
    Com_Printf("- Shadow: %d\n", cl_strafehelperUpsShadow->integer);
    Com_Printf("- Hide zero: %d\n", cl_strafehelperUpsHideZero->integer);
    Com_Printf("- Color mode: %s\n", cl_strafehelperUpsColorMode->string);
    Com_Printf("- Format: %s\n", cl_strafehelperUpsFormat->string);
    Com_Printf("- Gain color: %s\n", cl_strafehelperUpsColorGain->string);
    Com_Printf("- Loss color: %s\n", cl_strafehelperUpsColorLoss->string);
    Com_Printf("- Neutral color: %s\n", cl_strafehelperUpsColorNeutral->string);
}

void SH_Ups_Ypos_f(void) {
    const char *ypos = Cmd_Argv(3);
    float value;

    if (Cmd_Argc() < 4) {
        Com_Printf("- Center UPS Y pos: %.2f\n", cl_strafehelperUpsY->value);
        return;
    }

    if (sscanf(ypos, "%f", &value) == 1 && value >= -1000.0f && value <= 1000.0f) {
        Cvar_Set("sh_ups_y", ypos);
        Com_Printf("Center UPS Y pos set to: %s\n", ypos);
    } else {
        Com_Printf("Invalid Y position value. Usage: 'sh ups ypos <-1000-1000>'\n");
    }
}

void SH_Ups_Scale_f(void) {
    const char *scale = Cmd_Argv(3);
    float value;

    if (Cmd_Argc() < 4) {
        Com_Printf("- Center UPS scale: %.2f\n", cl_strafehelperUpsScale->value);
        return;
    }

    if (sscanf(scale, "%f", &value) == 1 && value >= 0.25f && value <= 8.0f) {
        Cvar_Set("sh_ups_scale", scale);
        Com_Printf("Center UPS scale set to: %s\n", scale);
    } else {
        Com_Printf("Invalid scale value. Usage: 'sh ups scale <0.25-8.0>'\n");
    }
}

void SH_Ups_Shadow_f(void) {
    const char *value = Cmd_Argv(3);

    if (Cmd_Argc() < 4) {
        Com_Printf("- Center UPS shadow: %d\n", cl_strafehelperUpsShadow->integer);
        return;
    }

    if (SH_IsOnOffValue(value)) {
        Cvar_Set("sh_ups_shadow", value);
        Com_Printf("Center UPS shadow %s.\n", cl_strafehelperUpsShadow->integer ? "enabled" : "disabled");
    } else {
        Com_Printf("Invalid value. Usage: 'sh ups shadow <0|1>'\n");
    }
}

void SH_Ups_HideZero_f(void) {
    const char *value = Cmd_Argv(3);

    if (Cmd_Argc() < 4) {
        Com_Printf("- Center UPS hide zero: %d\n", cl_strafehelperUpsHideZero->integer);
        return;
    }

    if (SH_IsOnOffValue(value)) {
        Cvar_Set("sh_ups_hide_zero", value);
        Com_Printf("Center UPS hide zero %s.\n", cl_strafehelperUpsHideZero->integer ? "enabled" : "disabled");
    } else {
        Com_Printf("Invalid value. Usage: 'sh ups hide_zero <0|1>'\n");
    }
}

void SH_Ups_ColorMode_f(void) {
    const char *mode = Cmd_Argv(3);

    if (Cmd_Argc() < 4) {
        Com_Printf("- Center UPS color mode: %s\n", cl_strafehelperUpsColorMode->string);
        return;
    }

    if (!Q_stricmp(mode, "dynamic") || !Q_stricmp(mode, "static") ||
        !Q_stricmp(mode, "threshold") || !Q_stricmp(mode, "rainbow")) {
        Cvar_Set("sh_ups_color_mode", mode);
        Com_Printf("Center UPS color mode set to: %s\n", mode);
    } else {
        Com_Printf("Invalid color mode. Usage: 'sh ups color_mode <dynamic|static|threshold|rainbow>'\n");
    }
}

void SH_Ups_ColorGain_f(void) {
    SH_SetColorCvar("sh_ups_color_gain", "Center UPS gain color", "sh ups color_gain R G B A");
}

void SH_Ups_ColorLoss_f(void) {
    SH_SetColorCvar("sh_ups_color_loss", "Center UPS loss color", "sh ups color_loss R G B A");
}

void SH_Ups_ColorNeutral_f(void) {
    SH_SetColorCvar("sh_ups_color_neutral", "Center UPS neutral color", "sh ups color_neutral R G B A");
}

void SH_Ups_Format_f(void) {
    const char *format = Cmd_Argv(3);

    if (Cmd_Argc() < 4) {
        Com_Printf("- Center UPS format: %s\n", cl_strafehelperUpsFormat->string);
        return;
    }

    if (!Q_stricmp(format, "plain") || !Q_stricmp(format, "suffix") ||
        !Q_stricmp(format, "prefix") || !strcmp(format, "0") ||
        !strcmp(format, "1") || !strcmp(format, "2")) {
        Cvar_Set("sh_ups_format", format);
        Com_Printf("Center UPS format set to: %s\n", format);
    } else {
        Com_Printf("Invalid format. Usage: 'sh ups format <plain|suffix|prefix>'\n");
    }
}

void SH_Ups_Help_f(void) {
    Com_Printf("========================================================================================\n");
    Com_LPrintf(PRINT_WARNING, "Center UPS Menu\n");
    Com_Printf("Usage: sh ups <command> [options]\n");
    Com_Printf("========================================================================================\n");
    Com_Printf("Visibility\n");
    Com_Printf("  %-32s %s\n", "enable", "Draw cl_ups in the center of the screen.");
    Com_Printf("  %-32s %s\n", "disable", "Hide the center cl_ups display.");
    Com_Printf("  %-32s %s\n", "toggle", "Toggle center cl_ups.");
    Com_Printf("  %-32s %s\n", "status", "Show current center cl_ups settings.");
    Com_Printf("----------------------------------------------------------------------------------------\n");
    Com_Printf("Layout\n");
    Com_Printf("  %-32s %s\n", "scale <0.25-8.0>", "Resize center UPS text.");
    Com_Printf("  %-32s %s\n", "ypos <-1000-1000>", "Move center UPS text up/down from center.");
    Com_Printf("  %-32s %s\n", "shadow <0|1>", "Toggle text shadow.");
    Com_Printf("  %-32s %s\n", "hide_zero <0|1>", "Hide when rounded UPS is 0.");
    Com_Printf("  %-32s %s\n", "format <plain|suffix|prefix>", "Use 742, 742 ups, or UPS: 742.");
    Com_Printf("----------------------------------------------------------------------------------------\n");
    Com_Printf("Color\n");
    Com_Printf("  %-32s %s\n", "color_mode <mode>", "dynamic, static, threshold, or rainbow.");
    Com_Printf("  %-32s %s\n", "color_gain R G B A", "Gain/high-speed color.");
    Com_Printf("  %-32s %s\n", "color_loss R G B A", "Loss/low-speed color.");
    Com_Printf("  %-32s %s\n", "color_neutral R G B A", "Neutral/static color.");
    Com_Printf("----------------------------------------------------------------------------------------\n");
    Com_Printf("Examples\n");
    Com_Printf("  sh ups enable\n");
    Com_Printf("  sh ups scale 1.5\n");
    Com_Printf("  sh ups format suffix\n");
    Com_Printf("  sh ups color_mode rainbow\n");
    Com_Printf("========================================================================================\n");
}

void SH_Color_Accel_f(void) {
    const char *color = Cmd_ArgsFrom(3);
    int r, g, b, a;

    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("- Accelerating color: %s\n", cl_strafehelper_color_accelerating->string);
        return;
    }

    if (sscanf(color, "%d %d %d %d", &r, &g, &b, &a) == 4 &&
        r >= 0 && r <= 255 && g >= 0 && g <= 255 &&
        b >= 0 && b <= 255 && a >= 0 && a <= 255) {
        Cvar_Set("sh_color_accelerating", color);
        Com_Printf("Accelerating color set to: %s\n", color);
    } else {
        Com_Printf("Wrong input. Use 'sh hud color_accelerating R G B A'\n");
    }
}

void SH_Color_Optimal_f(void) {
    const char *color = Cmd_ArgsFrom(3);
    int r, g, b, a;

    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("- Optimal color: %s\n", cl_strafehelper_color_optimal->string);
        return;
    }

    if (sscanf(color, "%d %d %d %d", &r, &g, &b, &a) == 4 &&
        r >= 0 && r <= 255 && g >= 0 && g <= 255 &&
        b >= 0 && b <= 255 && a >= 0 && a <= 255) {
        Cvar_Set("sh_color_optimal", color);
        Com_Printf("Optimal color changed to: %s\n", color);
    } else {
        Com_Printf("Wrong input. Use 'sh hud color_optimal R G B A'\n");
    }
}

void SH_Color_CenterMarker_f(void) {
    const char *color = Cmd_ArgsFrom(3);
    int r, g, b, a;

    if (Cmd_Argc() < 4) // Check if fewer than 3 arguments are provided
    {
        Com_Printf("- Center marker color: %s\n", cl_strafehelper_color_centermarker->string);
        return;
    }

    if (sscanf(color, "%d %d %d %d", &r, &g, &b, &a) == 4 &&
        r >= 0 && r <= 255 && g >= 0 && g <= 255 &&
        b >= 0 && b <= 255 && a >= 0 && a <= 255) {
        Cvar_Set("sh_color_centermarker", color);
        Com_Printf("Center marker color set to: %s\n", color);
    } else {
        Com_Printf("Wrong input. Use 'sh hud color_centermarker R G B A'\n");
    }
}

void SH_Hud_Help_f(void) {
    Com_Printf("========================================================================================\n");
    Com_LPrintf(PRINT_WARNING, "Strafe HUD Menu\n");
    Com_Printf("Usage: sh hud <command> [options]\n");
    Com_Printf("========================================================================================\n");
    Com_Printf("Visibility\n");
    Com_Printf("  %-34s %s\n", "enable", "Enable the strafe helper bar.");
    Com_Printf("  %-34s %s\n", "disable", "Disable the strafe helper bar.");
    Com_Printf("  %-34s %s\n", "status", "Show current HUD and UPS settings.");
    Com_Printf("----------------------------------------------------------------------------------------\n");
    Com_Printf("Layout\n");
    Com_Printf("  %-34s %s\n", "scale <value>", "Set bar horizontal scale.");
    Com_Printf("  %-34s %s\n", "ypos <value>", "Set vertical position.");
    Com_Printf("  %-34s %s\n", "height <value>", "Set bar height.");
    Com_Printf("  %-34s %s\n", "center_width <0.1-5.0>", "Set center marker width.");
    Com_Printf("  %-34s %s\n", "optimal_width <value>", "Set optimal marker width.");
    Com_Printf("----------------------------------------------------------------------------------------\n");
    Com_Printf("Style\n");
    Com_Printf("  %-34s %s\n", "alpha <0.0-1.0>", "Set helper opacity.");
    Com_Printf("  %-34s %s\n", "fade_inactive <0|1>", "Fade when there is no movement input.");
    Com_Printf("  %-34s %s\n", "bar_style <style>", "solid, gradient, outline, or minimal.");
    Com_Printf("  %-34s %s\n", "smoothing <0.0-10.0>", "Smooth visual angle movement; 0 disables it.");
    Com_Printf("  %-34s %s\n", "smoothing_mode <mode>", "linear, quadratic, cubic, sine, or exponential.");
    Com_Printf("  %-34s %s\n", "center_marker", "Toggle the center marker.");
    Com_Printf("----------------------------------------------------------------------------------------\n");
    Com_Printf("Color\n");
    Com_Printf("  %-34s %s\n", "color_accelerating R G B A", "Acceleration zone color.");
    Com_Printf("  %-34s %s\n", "color_optimal R G B A", "Optimal marker color.");
    Com_Printf("  %-34s %s\n", "color_centermarker R G B A", "Center marker color.");
    Com_Printf("  %-34s %s\n", "preset <name|number|random>", "Apply a predefined color preset.");
    Com_Printf("----------------------------------------------------------------------------------------\n");
    Com_Printf("Examples\n");
    Com_Printf("  sh hud enable\n");
    Com_Printf("  sh hud bar_style outline\n");
    Com_Printf("  sh hud alpha 0.7\n");
    Com_Printf("  sh hud preset random\n");
    Com_Printf("========================================================================================\n");
}
