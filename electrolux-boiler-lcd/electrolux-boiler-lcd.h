#pragma once

#include <cmath>
#include "electrolux-boiler-lcd-defines.h"

// ─── Rotation ─────────────────────────────────────────────────────────────────

// Toggles display between LCD_ROTATION_LEFT and LCD_ROTATION_RIGHT and refreshes.
void lcd_toggle_rotation() {
    id(rotation_setting) = (id(rotation_setting) == LCD_ROTATION_LEFT) ? LCD_ROTATION_RIGHT : LCD_ROTATION_LEFT;
    if (id(rotation_setting) == LCD_ROTATION_LEFT)
        id(my_display).set_rotation(DISPLAY_ROTATION_90_DEGREES);
    else
        id(my_display).set_rotation(DISPLAY_ROTATION_270_DEGREES);
    id(my_display).update();
}

// ─── Backlight ────────────────────────────────────────────────────────────────

// Steps brightness through LCD_BRIGHTNESS_MIN → ... → LCD_BRIGHTNESS_MAX → LCD_BRIGHTNESS_MIN and syncs the HA slider.
void lcd_cycle_brightness() {
    id(current_brightness_val) += LCD_BRIGHTNESS_STEP;
    if (id(current_brightness_val) > LCD_BRIGHTNESS_MAX) id(current_brightness_val) = LCD_BRIGHTNESS_MIN;
    auto call = id(brightness_slider).make_call();
    call.set_value(id(current_brightness_val));
    call.perform();
}

// Forces the backlight on at the saved brightness level when in AP mode (offline).
void lcd_ensure_backlight_ap_mode() {
    if (!id(online_status).state) {
        auto call = id(display_backlight).make_call();
        call.set_brightness(id(current_brightness_val));
        call.set_state(true);
        call.perform();
    }
}

// ─── Display ──────────────────────────────────────────────────────────────────

// Draws a thick arc by rendering concentric arcs of decreasing radius.
// Extracted as a free function to avoid stack pressure from a capturing lambda inside lcd_draw_screen.
template<typename Display>
static void lcd_draw_thick_arc(Display& it, int cx, int cy, int r, int thickness, int start_angle, int end_angle, Color color) {
    for (int t = 0; t < thickness; t++) {
        int current_r = r - t;
        for (int a = start_angle; a < end_angle; a++) {
            float rad1 = a * M_PI / 180.0f;
            float rad2 = (a + 1) * M_PI / 180.0f;
            it.line(
                cx + (int)(current_r * cosf(rad1)), cy + (int)(current_r * sinf(rad1)),
                cx + (int)(current_r * cosf(rad2)), cy + (int)(current_r * sinf(rad2)),
                color);
        }
    }
}

// Draws a narrow trend arrow (▲/▼) or dash (–) between the HEATING label and the gauge arc.
// Up = green ▲, Down = blue ▼, Stable = grey –.
template<typename Display>
static void lcd_draw_trend(Display& it, const std::string& trend) {
    if (trend == LCD_STR_TREND_UP) {
        it.filled_triangle(
            LCD_TREND_CX,                LCD_TREND_Y_TOP,  // tip
            LCD_TREND_CX - LCD_TREND_HW, LCD_TREND_Y_BOT,  // base left
            LCD_TREND_CX + LCD_TREND_HW, LCD_TREND_Y_BOT,  // base right
            id(green));
    } else if (trend == LCD_STR_TREND_DOWN) {
        it.filled_triangle(
            LCD_TREND_CX - LCD_TREND_HW, LCD_TREND_Y_TOP,  // base left
            LCD_TREND_CX + LCD_TREND_HW, LCD_TREND_Y_TOP,  // base right
            LCD_TREND_CX,                LCD_TREND_Y_BOT,  // tip
            id(blue));
    } else {
        it.line(
            LCD_TREND_CX - LCD_TREND_HW, LCD_TREND_MID,
            LCD_TREND_CX + LCD_TREND_HW, LCD_TREND_MID,
            Color(LCD_COLOR_DIM, LCD_COLOR_DIM, LCD_COLOR_DIM));
    }
}

// Renders the boiler LCD screen: AP-mode setup page or live gauge/info page.
template<typename Display>
static void lcd_draw_screen(Display& it) {
    // AP mode: show Wi-Fi setup instructions
    if (!id(online_status).state) {
        int w = it.get_width();
        int h = it.get_height();
        int cx = w / 2;
        it.fill(id(black));
        it.rectangle(0, 0, w, h, id(blue));
        it.printf(cx, LCD_AP_Y_TITLE,   &id(font_medium), id(white),  TextAlign::CENTER, LCD_STR_SETUP_TITLE);
        it.printf(cx, LCD_AP_Y_CONNECT, &id(font_small),  id(white),  TextAlign::CENTER, LCD_STR_SETUP_CONNECT);
        it.printf(cx, LCD_AP_Y_SSID,    &id(font_medium), id(yellow), TextAlign::CENTER, LCD_STR_AP_SSID);
        return;
    }

    it.fill(id(black));

    // --- DRAW GAUGE ---
    const int cx        = LCD_GAUGE_CX;
    const int cy        = LCD_GAUGE_CY;
    const int r         = LCD_GAUGE_RADIUS;
    const int thickness = LCD_GAUGE_THICKNESS;

    // Segments: Blue (Cold), Green (OK), Yellow (Hot), Red (Danger)
    lcd_draw_thick_arc(it, cx, cy, r, thickness, LCD_ARC_COLD_START, LCD_ARC_COLD_END,   id(blue));
    lcd_draw_thick_arc(it, cx, cy, r, thickness, LCD_ARC_COLD_END,   LCD_ARC_OK_END,     id(green));
    lcd_draw_thick_arc(it, cx, cy, r, thickness, LCD_ARC_OK_END,     LCD_ARC_HOT_END,    id(yellow));
    lcd_draw_thick_arc(it, cx, cy, r, thickness, LCD_ARC_HOT_END,    LCD_ARC_DANGER_END, id(red));

    // --- DRAW NEEDLE ---
    // Clamp and map LCD_TEMP_MIN..LCD_TEMP_MAX → LCD_TEMP_ANGLE_START..LCD_TEMP_ANGLE_START+LCD_TEMP_ANGLE_SPAN
    float temp = id(current_temp).state;
    if (std::isnan(temp) || temp < LCD_TEMP_MIN) temp = LCD_TEMP_MIN;
    if (temp > LCD_TEMP_MAX) temp = LCD_TEMP_MAX;
    float rad = (LCD_TEMP_ANGLE_START + (temp - LCD_TEMP_MIN) * (LCD_TEMP_ANGLE_SPAN / LCD_TEMP_RANGE)) * M_PI / 180.0f;
    it.filled_circle(cx + (int)(r * cosf(rad)), cy + (int)(r * sinf(rad)), LCD_NEEDLE_DOT_RADIUS, id(white));

    // --- TEXT ---
    if (id(current_temp).has_state())
        it.printf(cx, LCD_MAIN_TEMP_Y, &id(font_huge), id(white), TextAlign::CENTER, "%.0f°", id(current_temp).state);

    it.printf(LCD_MAIN_TARGET_X, LCD_MAIN_TARGET_Y, &id(font_medium), Color(LCD_COLOR_DIM, LCD_COLOR_DIM, LCD_COLOR_DIM), "%.0f°", id(target_temp).state);

    if (id(is_heating).state)
        it.print(LCD_MAIN_STATUS_X, LCD_MAIN_STATUS_Y, &id(font_small), id(red),   TextAlign::TOP_RIGHT, LCD_STR_HEATING);
    else
        it.print(LCD_MAIN_STATUS_X, LCD_MAIN_STATUS_Y, &id(font_small), id(green), TextAlign::TOP_RIGHT, LCD_STR_STANDBY);

    it.print(LCD_MAIN_MODE_X - 10, LCD_MAIN_MODE_Y, &id(font_small), id(yellow), TextAlign::TOP_CENTER, id(mqtt_raw_mode).state.c_str());

    if (id(is_bst).state)
        it.print(LCD_MAIN_BST_X + 13, LCD_MAIN_BST_Y, &id(font_small), id(yellow), TextAlign::TOP_RIGHT, LCD_STR_BST);

    // Trend arrow: ▲ Up (green), ▼ Down (blue), – Stable (grey)
    lcd_draw_trend(it, id(mqtt_raw_trend).state);

    it.printf(LCD_MAIN_RSSI_X, LCD_MAIN_RSSI_Y, &id(font_small), Color(LCD_COLOR_DIM, LCD_COLOR_DIM, LCD_COLOR_DIM), "%.0f dBm", id(wifi_rssi).state);
}

