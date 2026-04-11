#pragma once

// ─── Rotation ─────────────────────────────────────────────────────────────────
#define LCD_ROTATION_LEFT           90    // USB port on the left (default)
#define LCD_ROTATION_RIGHT          270   // USB port on the right

// ─── Brightness ───────────────────────────────────────────────────────────────
#define LCD_BRIGHTNESS_STEP         0.25f // Step size when cycling brightness
#define LCD_BRIGHTNESS_MIN          0.1f // Minimum brightness level (wraps back to this)
#define LCD_BRIGHTNESS_MAX          1.0f  // Maximum brightness level (100%)

// ─── Gauge Geometry ───────────────────────────────────────────────────────────
#define LCD_GAUGE_CX                120   // Gauge center X
#define LCD_GAUGE_CY                130   // Gauge center Y (shifted down)
#define LCD_GAUGE_RADIUS            100   // Gauge arc radius in pixels
#define LCD_GAUGE_THICKNESS         10    // Arc line thickness in pixels

// ─── Gauge Arc Segments (degrees) ─────────────────────────────────────────────
#define LCD_ARC_COLD_START          180   // Start of cold (blue) segment
#define LCD_ARC_COLD_END            225   // End of cold / start of OK (green)
#define LCD_ARC_OK_END              315   // End of OK / start of hot (yellow)
#define LCD_ARC_HOT_END             345   // End of hot / start of danger (red)
#define LCD_ARC_DANGER_END          360   // End of danger segment

// ─── Temperature Range ────────────────────────────────────────────────────────
#define LCD_TEMP_MIN                30.0f // °C — lower clamp for gauge needle
#define LCD_TEMP_MAX                75.0f // °C — upper clamp for gauge needle
#define LCD_TEMP_RANGE              45.0f // LCD_TEMP_MAX - LCD_TEMP_MIN
#define LCD_TEMP_ANGLE_START        180.0f// Needle start angle (degrees)
#define LCD_TEMP_ANGLE_SPAN         180.0f// Total sweep of needle (degrees)

// ─── Needle ───────────────────────────────────────────────────────────────────
#define LCD_NEEDLE_DOT_RADIUS       6     // Radius of the filled-circle needle head

// ─── Dim Color (grey labels) ──────────────────────────────────────────────────
#define LCD_COLOR_DIM               200   // R/G/B value for grey label text

// ─── Text Layout — AP Setup Screen ────────────────────────────────────────────
#define LCD_AP_Y_TITLE              20    // Y position of "WIFI SETUP MODE" label
#define LCD_AP_Y_CONNECT            55    // Y position of "Connect to Wi-Fi:" label
#define LCD_AP_Y_SSID               75    // Y position of AP SSID label

// ─── Text Layout — Main Screen ────────────────────────────────────────────────
#define LCD_MAIN_TEMP_Y             85    // Y position of big central temperature
#define LCD_MAIN_TARGET_X           10    // X position of target temperature label
#define LCD_MAIN_TARGET_Y           5     // Y position of target temperature label
#define LCD_MAIN_MODE_X             120   // X position of mode label (center-top)
#define LCD_MAIN_MODE_Y             5     // Y position of mode label
#define LCD_MAIN_STATUS_X           230   // X position of HEATING/STANDBY label
#define LCD_MAIN_STATUS_Y           5     // Y position of HEATING/STANDBY label
#define LCD_MAIN_RSSI_X             10    // X position of Wi-Fi RSSI label
#define LCD_MAIN_RSSI_Y             118   // Y position of Wi-Fi RSSI label
#define LCD_MAIN_BST_X              230   // X position of BST label (bottom-right)
#define LCD_MAIN_BST_Y              118   // Y position of BST label

// ─── Trend Arrow ──────────────────────────────────────────────────────────────
// Narrow filled-triangle arrow drawn between the HEATING label (bottom ~Y=19)
// and the top of the gauge arc (~Y=30). Stable draws a dash (–).
#define LCD_TREND_CX        220   // X center of trend arrow (right side, below HEATING label)
#define LCD_TREND_Y_TOP      20   // Y top of arrow (just below HEATING/STANDBY label)
#define LCD_TREND_Y_BOT      29   // Y bottom of arrow (just above gauge arc top)
#define LCD_TREND_HW          3   // Half-width in pixels (total 6 px — narrow)
#define LCD_TREND_MID        ((LCD_TREND_Y_TOP + LCD_TREND_Y_BOT) / 2)  // Vertical midpoint for dash

// ─── UI Strings ───────────────────────────────────────────────────────────────
#define LCD_STR_SETUP_TITLE         "WIFI SETUP MODE"
#define LCD_STR_SETUP_CONNECT       "Connect to Wi-Fi:"
#define LCD_STR_AP_SSID             "AP-Boiler-LCD"
#define LCD_STR_HEATING             "HEATING"
#define LCD_STR_STANDBY             "STANDBY"
#define LCD_STR_BST                 "BST"
#define LCD_STR_HEATING_STATE_ON    "ON"    // MQTT payload for heating active
#define LCD_STR_TREND_UP            "Up"
#define LCD_STR_TREND_DOWN          "Down"
#define LCD_STR_TREND_STABLE        "Stable"
