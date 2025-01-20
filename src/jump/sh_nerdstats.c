#include "strafe_helper.h"
#include <src/client/client.h>
#include <math.h>
#include <stdbool.h>
#include "strafe_helper_customization.h"
#include <stddef.h>


char* SH_NerdStats_Draw(float hud_width, float hud_height, int font_pic)
{
    char formatted[64];
    const int lineSpacing = CHAR_HEIGHT * 1.5f;
    char buffer[MAX_STRING_CHARS];

    const int dataIndent = 20;
    const int titleIndent = 40;

    int block_width = 240;
    int block_height = hud_height;
    int x = 0;
    int margin = 10;
    int left_y = CHAR_HEIGHT + margin;


    if (cl_strafehelperNerdStats->integer == 1)
    {
        x = CHAR_WIDTH;
    }
    else if (cl_strafehelperNerdStats->integer == 2)
    {
        x = (hud_width - block_width) + 10.0f;
    }


    if (cl_strafehelperNerdStats && cl_strafehelperNerdStats->integer)
    {
        shc_drawFilledRectangle(x - 10, 0, block_width, block_height, shc_ElementId_NerdStats);


        // -------------------------------------
        // [Physics Timings]
        // -------------------------------------

        // Frame Timings Section
        R_SetColor(U32_YELLOW);
        left_y += lineSpacing * 2.0; // Add spacing
        R_DrawString(x + titleIndent, left_y, 0, MAX_STRING_CHARS, "[Frame Timings]", font_pic);
        left_y += lineSpacing;
        R_SetColor(U32_WHITE);

        // Call and display the values
        CL_Mfps_m(formatted, sizeof(formatted));
        snprintf(buffer, sizeof(buffer), "%-12s %s", "Client FPS:", formatted);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;

        R_Fps_m(formatted, sizeof(formatted));
        snprintf(buffer, sizeof(buffer), "%-12s %s", "Render FPS:", formatted);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;

        CL_Mmps_m(formatted, sizeof(formatted));
        snprintf(buffer, sizeof(buffer), "%-12s %s FPS", "Main FPS:", formatted);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;

        CL_Pps_m(formatted, sizeof(formatted));
        snprintf(buffer, sizeof(buffer), "%-12s %s packets", "Packets/sec:", formatted);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;

        CL_Ping_m(formatted, sizeof(formatted));
        snprintf(buffer, sizeof(buffer), "%-12s %s ms", "Ping:", formatted);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;

        CL_Lag_m(formatted, sizeof(formatted));
        snprintf(buffer, sizeof(buffer), "%-12s %s ", "Lag:", formatted);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;
        // -------------------------------------
        // [Velocity]
        // -------------------------------------
        R_SetColor(U32_YELLOW);
        left_y += lineSpacing * 2.0; // some extra spacing
        //
        R_DrawString(x + titleIndent, left_y, 0, MAX_STRING_CHARS, "[Predicted Velocity]", font_pic);
        left_y += lineSpacing;
        R_SetColor(U32_WHITE);

        snprintf(buffer, sizeof(buffer), "%-12s %+8.2f", "UPS:", ns.pred_velocity);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;

        snprintf(buffer, sizeof(buffer), "%-12s %+8.2f", "X:", ns.pred_velocity_x);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;

        snprintf(buffer, sizeof(buffer), "%-12s %+8.2f", "Y:", ns.pred_velocity_y);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;

        snprintf(buffer, sizeof(buffer), "%-12s %+8.2f", "Z:", ns.pred_velocity_z);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;


        // -------------------------------------
        // [Position]
        // -------------------------------------
        R_SetColor(U32_YELLOW);
        left_y += lineSpacing * 2.0; // some extra spacing
        R_DrawString(x + titleIndent, left_y, 0, MAX_STRING_CHARS, "[Origin]", font_pic);
        left_y += lineSpacing;
        R_SetColor(U32_WHITE);

        snprintf(buffer, sizeof(buffer), "%-12s %+8.2f", "X:", ns.pred_pos_x);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;
        snprintf(buffer, sizeof(buffer), "%-12s %+8.2f", "Y:", ns.pred_pos_y);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;
        snprintf(buffer, sizeof(buffer), "%-12s %+8.2f", "Z:", ns.pred_pos_z);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;


        // -------------------------------------
        // Orientation (pitch, yaw, roll)
        // -------------------------------------

        R_SetColor(U32_YELLOW);
        left_y += lineSpacing * 2.0; // some extra spacing
        R_DrawString(x + titleIndent, left_y, 0, MAX_STRING_CHARS, "[Orientation]", font_pic);
        left_y += lineSpacing;
        R_SetColor(U32_WHITE);

        snprintf(buffer, sizeof(buffer), "%-12s %+8.2f", "Pitch:", ns.pitch);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;
        snprintf(buffer, sizeof(buffer), "%-12s %+8.2f", "Roll:", ns.roll);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;
        snprintf(buffer, sizeof(buffer), "%-12s %+8.4f", "ViewAngle/Yaw:", ns.viewangles);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;


        // -------------------------------------
        // [Angles]
        // -------------------------------------
        R_SetColor(U32_YELLOW);
        left_y += lineSpacing * 2.0; // some extra spacing
        R_DrawString(x + titleIndent, left_y, 0, MAX_STRING_CHARS, "[Angles]", font_pic);
        left_y += lineSpacing;
        R_SetColor(U32_WHITE);
        //
        snprintf(buffer, sizeof(buffer), "%-12s %+8.2f", "Optimal:", sh.angle_optimal);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;

        snprintf(buffer, sizeof(buffer), "%-12s %+8.2f", "Minimum:", sh.angle_minimum);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;

        snprintf(buffer, sizeof(buffer), "%-12s %+8.2f", "Maximum:", sh.angle_maximum);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;

        snprintf(buffer, sizeof(buffer), "%-12s %+8.2f", "Angle Diff:", sh.angle_diff);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;




        // -------------------------------------
        // [PM_Accelerate]
        // -------------------------------------
        R_SetColor(U32_YELLOW);
        left_y += lineSpacing * 2.0; // some extra spacing
        R_DrawString(x + titleIndent, left_y, 0, MAX_STRING_CHARS, "[PM_Accelerate]", font_pic);
        left_y += lineSpacing;
        R_SetColor(U32_WHITE);
        //
        snprintf(buffer, sizeof(buffer), "%-12s %+8.2f", "Addspeed:", ns.addspeed_nerd);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;

        snprintf(buffer, sizeof(buffer), "%-12s %+8.2f", "Curr speed:", ns.currentspeed_nerd);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;

        snprintf(buffer, sizeof(buffer), "%-12s %+8.2f", "Accelspeed:", ns.accelspeed_nerd);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;

        snprintf(buffer, sizeof(buffer), "%-12s %+8.2f", "Wishdir:", ns.wishdir_nerd);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;
        snprintf(buffer, sizeof(buffer), "%-12s %+8.2f", "Fwd vel ang:", ns.forward_velocity_angle_nerd);
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;


        // -------------------------------------
        // [PMove Debug]
        // -------------------------------------
        R_SetColor(U32_YELLOW);
        left_y += lineSpacing * 2.0; // some extra spacing
        R_DrawString(x + titleIndent, left_y, 0, MAX_STRING_CHARS, "[PMove Debug]", font_pic);
        left_y += lineSpacing;
        R_SetColor(U32_WHITE);
        //
        static const char* const types[] = {
            "NORMAL", "SPECTATOR", "DEAD", "GIB", "FREEZE"
        };
        static const char* const flags[] = {
            "DUCKED", "JUMP_HELD", "ON_GROUND",
            "TIME_WATERJUMP", "TIME_LAND", "TIME_TELEPORT",
            "NO_PREDICTION", "TELEPORT_BIT"
        };
        unsigned pm_type = cl.frame.ps.pmove.pm_type;
        if (pm_type > PM_FREEZE)
            pm_type = PM_FREEZE;

        snprintf(buffer, sizeof(buffer), "%-12s %s", "pm_type:",
                 (pm_type <= PM_FREEZE ? types[pm_type] : "UNKNOWN"));
        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
        left_y += lineSpacing;

        R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, "", font_pic);
        left_y += lineSpacing;

        unsigned pm_flags = cl.frame.ps.pmove.pm_flags;
        for (unsigned i = 0; i < 8; i++)
        {
            if (pm_flags & (1 << i))
            {
                snprintf(buffer, sizeof(buffer), "%-12s %s", "", flags[i]);
                R_DrawString(x + dataIndent, left_y, 0, MAX_STRING_CHARS, buffer, font_pic);
                left_y += lineSpacing;
            }
        }
    }


    return 0;
}
// Helper function to print a section header
void printSectionHeader(const char* title) {
    Com_Printf("\n---------- %s ----------\n", title);
}

// Unified helper for key-value pairs (strings or floats)
void printKeyValueGeneric(const char* label, const char* value) {
    Com_Printf("%-25s | %-15s\n", label, value); // Wider label and value columns
}

void printKeyValueFloat(const char* label, float value) {
    char formattedValue[32];
    snprintf(formattedValue, sizeof(formattedValue), "%+10.2f", value); // Format float as a string
    printKeyValueGeneric(label, formattedValue); // Reuse generic helper
}

void printKeyValueFloatPrecise(const char* label, float value) {
    char formattedValue[32];
    snprintf(formattedValue, sizeof(formattedValue), "%+10.4f", value); // Format high-precision float
    printKeyValueGeneric(label, formattedValue); // Reuse generic helper
}

void DebugNow(void) {
    // Print debug information immediately
    Com_LPrintf(PRINT_WARNING, "========== DebugNow Triggered ==========\n");

    // Predicted Velocity Section
    printSectionHeader("Predicted Velocity");
    printKeyValueFloat("UPS", ns.pred_velocity);
    printKeyValueFloat("X", ns.pred_velocity_x);
    printKeyValueFloat("Y", ns.pred_velocity_y);
    printKeyValueFloat("Z", ns.pred_velocity_z);

    // Predicted Origin Section
    printSectionHeader("Predicted Origin");
    printKeyValueFloat("X", ns.pred_pos_x);
    printKeyValueFloat("Y", ns.pred_pos_y);
    printKeyValueFloat("Z", ns.pred_pos_z);

    // Orientation Section
    printSectionHeader("Orientation");
    printKeyValueFloat("Pitch", ns.pitch);
    printKeyValueFloat("Roll", ns.roll);
    printKeyValueFloatPrecise("ViewAngle/Yaw", ns.viewangles);

    // Angles Section
    printSectionHeader("Angles");
    printKeyValueFloat("Optimal", sh.angle_optimal);
    printKeyValueFloat("Minimum", sh.angle_minimum);
    printKeyValueFloat("Maximum", sh.angle_maximum);
    printKeyValueFloat("Angle Diff", sh.angle_diff);

    // PM_Accelerate Section
    printSectionHeader("PM_Accelerate");
    printKeyValueFloat("Addspeed", ns.addspeed_nerd);
    printKeyValueFloat("Curr speed", ns.currentspeed_nerd);
    printKeyValueFloat("Accelspeed", ns.accelspeed_nerd);
    printKeyValueFloat("Wishdir", ns.wishdir_nerd);
    printKeyValueFloat("Fwd vel ang", ns.forward_velocity_angle_nerd);

    Com_LPrintf(PRINT_WARNING, "========================================\n");
}


void SH_DebugNow_f(void)
{
    DebugNow();
}
