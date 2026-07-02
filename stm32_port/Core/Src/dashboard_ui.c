#include "dashboard_ui.h"

#include "main.h"

#include "../lvgl/lvgl.h"

#include "can.h"
#include "gps.h"
#include "gps_lap.h"

#include <string.h>

typedef struct {
    lv_obj_t * speed_digit_container;
    lv_obj_t * speed_segments[2][7];
    lv_obj_t * lap_current_label;
    lv_obj_t * lap_current_value;
    lv_obj_t * lap_last_label;
    lv_obj_t * lap_last_value;
    lv_obj_t * lap_best_label;
    lv_obj_t * lap_best_value;
    lv_obj_t * delta_bar_track;
    lv_obj_t * delta_bar_fill;
    lv_obj_t * delta_bar_center;
    lv_obj_t * delta_value;
    lv_obj_t * mode_tile;
    lv_obj_t * mode_value;
    lv_obj_t * soc_value;
    lv_obj_t * battery_fill;
    lv_obj_t * throttle_bar_fill;
    lv_obj_t * brake_bar_fill;
    lv_obj_t * wheel_fl;
    lv_obj_t * wheel_fr;
    lv_obj_t * wheel_rl;
    lv_obj_t * wheel_rr;
    lv_obj_t * lightning_fl;
    lv_obj_t * lightning_fr;
    lv_obj_t * lightning_rl;
    lv_obj_t * lightning_rr;
    lv_obj_t * total_voltage_value;
    lv_obj_t * total_current_value;
    lv_obj_t * max_temp_value;
    lv_obj_t * power_live_value;
    lv_obj_t * power_peak_value;
    lv_obj_t * motor_fl_torque;
    lv_obj_t * motor_fl_speed;
    lv_obj_t * motor_fl_power_live;
    lv_obj_t * motor_fl_power_peak;
    lv_obj_t * motor_fl_temp;
    lv_obj_t * motor_fr_torque;
    lv_obj_t * motor_fr_speed;
    lv_obj_t * motor_fr_power_live;
    lv_obj_t * motor_fr_power_peak;
    lv_obj_t * motor_fr_temp;
    lv_obj_t * motor_rl_torque;
    lv_obj_t * motor_rl_speed;
    lv_obj_t * motor_rl_power_live;
    lv_obj_t * motor_rl_power_peak;
    lv_obj_t * motor_rl_temp;
    lv_obj_t * motor_rr_torque;
    lv_obj_t * motor_rr_speed;
    lv_obj_t * motor_rr_power_live;
    lv_obj_t * motor_rr_power_peak;
    lv_obj_t * motor_rr_temp;
    lv_obj_t * laps_current_label;
    lv_obj_t * laps_current_value;
    lv_obj_t * laps_left_label;
    lv_obj_t * laps_left_value;
    lv_obj_t * signal_bars[4];
    lv_obj_t * alert_circle;
    lv_obj_t * alert_label;
    lv_obj_t * odometer_label;
} dashboard_ui_t;

typedef enum {
    DRIVE_MODE_S = 0,
    DRIVE_MODE_Q,
    DRIVE_MODE_C,
    DRIVE_MODE_E
} drive_mode_t;

static dashboard_ui_t g_dashboard;
static drive_mode_t g_drive_mode = DRIVE_MODE_S;
static int32_t g_speed = 24;
static int32_t g_soc = 24;
static int32_t g_mode_index = 0;
static int32_t g_torque[4] = {24, 24, 24, 24};
static int32_t g_rpm[4] = {24, 24, 24, 24};
static int32_t g_sum_voltage = 24;
static int32_t g_top_temperature = 24;
static int32_t g_sum_current = 24;
static int32_t g_current_lap_time = 0;
static int32_t g_last_lap_time = 0;
static int32_t g_best_lap_time = 0;
static int32_t g_lap_delta = 0;
static int32_t g_laps_current = 0;
static int32_t g_laps_left = 0;
static int32_t g_vehicle_distance_m = 0;
static int32_t g_throttle_opening = 0;
static int32_t g_brake_force = 0;
static int32_t g_power_live = 12;
static int32_t g_power_peak = 36;
static int32_t g_motor_power_live[4] = {10, 10, 9, 9};
static int32_t g_motor_power_peak[4] = {24, 24, 23, 23};
static int32_t g_motor_temp[4] = {48, 47, 49, 50};
static int32_t g_tire_temp_fl = 35;
static int32_t g_tire_temp_fr = 48;
static int32_t g_tire_temp_rl = 58;
static int32_t g_tire_temp_rr = 66;
static bool g_motor_fl_online = true;
static bool g_motor_fr_online = true;
static bool g_motor_rl_online = true;
static bool g_motor_rr_online = true;
static dashboard_data_t g_dashboard_data = {
    .speed = 24,
    .soc = 24,
    .mode_index = 0,
    .torque = {24, 24, 24, 24},
    .motor_enable = {1, 1, 1, 1},
    .rpm = {24, 24, 24, 24},
    .sum_voltage = 24,
    .sum_current = 24,
    .max_temperature = 24,
};
static volatile dashboard_data_t g_pending_dashboard_data;
static volatile uint8_t g_dashboard_data_dirty = 0U;
static volatile int32_t g_pending_speed = 24;
static volatile uint8_t g_speed_dirty = 0U;
static volatile int32_t g_pending_signal_level = 0;
static volatile uint8_t g_signal_dirty = 0U;
static volatile int32_t g_pending_lap_delta = 0;
static volatile uint8_t g_lap_delta_dirty = 0U;
static volatile int32_t g_pending_lap_current = 0;
static volatile int32_t g_pending_lap_last = 0;
static volatile int32_t g_pending_lap_best = 0;
static volatile int32_t g_pending_lap_count = 0;
static volatile uint8_t g_lap_times_dirty = 0U;
static uint32_t g_speed_ui_last_tick = 0U;
static uint8_t g_lap_analysis_active = 0U;
static uint8_t g_lap_analysis_blink_visible = 0U;
static uint8_t g_lap_analysis_indicator_visible = 0xFFU;
static uint32_t g_lap_analysis_blink_tick = 0U;

#define DASHBOARD_ALERT_MAX 6U
#define DASHBOARD_ALERT_TEXT_MAX 32U
static char g_alert_queue[DASHBOARD_ALERT_MAX][DASHBOARD_ALERT_TEXT_MAX];
static uint8_t g_alert_count = 0U;
static char g_pending_alert_queue[DASHBOARD_ALERT_MAX][DASHBOARD_ALERT_TEXT_MAX];
static volatile uint8_t g_pending_alert_count = 0U;
static volatile uint8_t g_pending_alert_pop_count = 0U;
static volatile uint8_t g_pending_lap_toggle = 0U;
static volatile uint8_t g_pending_mode_toggle = 0U;

#define DASHBOARD_FONT_SMALL (&lv_font_montserrat_18)
#define DASHBOARD_FONT_MEDIUM (&lv_font_montserrat_18)
#define DASHBOARD_FONT_LARGE (&lv_font_montserrat_32)

static const uint8_t g_speed_digit_map[10][7] = {
    {1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 0, 0, 0, 0},
    {1, 1, 0, 1, 1, 0, 1},
    {1, 1, 1, 1, 0, 0, 1},
    {0, 1, 1, 0, 0, 1, 1},
    {1, 0, 1, 1, 0, 1, 1},
    {1, 0, 1, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 1, 1},
};

#define UI_BG_COLOR lv_color_hex(0xFFFFFF)
#define UI_TEXT_COLOR lv_color_hex(0x000000)
#define UI_BORDER_COLOR lv_color_hex(0x000000)
#define UI_SEGMENT_ON_COLOR lv_color_hex(0x000000)
#define UI_SEGMENT_OFF_COLOR lv_color_hex(0xD8D8D8)

#define SIM_HOR_RES 800
#define SIM_VER_RES 480

#define UI_TOP_HEIGHT 54
#define UI_MIDDLE_Y (UI_TOP_HEIGHT + 1)
#define UI_MIDDLE_HEIGHT 342
#define UI_BOTTOM_Y (UI_MIDDLE_Y + UI_MIDDLE_HEIGHT + 1)
#define UI_BOTTOM_HEIGHT (SIM_VER_RES - UI_BOTTOM_Y)

#define UI_LEFT_PANEL_WIDTH 180
#define UI_CENTER_PANEL_X UI_LEFT_PANEL_WIDTH
#define UI_CENTER_PANEL_WIDTH 440
#define UI_RIGHT_PANEL_X (UI_CENTER_PANEL_X + UI_CENTER_PANEL_WIDTH)
#define UI_RIGHT_PANEL_WIDTH (SIM_HOR_RES - UI_RIGHT_PANEL_X)

#define MODE_TOUCH_X_MIN 0
#define MODE_TOUCH_X_MAX UI_LEFT_PANEL_WIDTH
#define MODE_TOUCH_Y_MIN UI_MIDDLE_Y
#define MODE_TOUCH_Y_MAX (UI_MIDDLE_Y + UI_MIDDLE_HEIGHT)

#define LAP_TOUCH_X_MIN 0
#define LAP_TOUCH_X_MAX SIM_HOR_RES
#define LAP_TOUCH_Y_MIN 0
#define LAP_TOUCH_Y_MAX UI_TOP_HEIGHT

#define ALERT_TOUCH_X_MIN 322
#define ALERT_TOUCH_X_MAX 580
#define ALERT_TOUCH_Y_MIN UI_MIDDLE_Y
#define ALERT_TOUCH_Y_MAX (UI_MIDDLE_Y + 72)

#define DELTA_BAR_X 430
#define DELTA_BAR_Y 17
#define DELTA_BAR_W 84
#define DELTA_BAR_H 16
#define DELTA_BAR_INNER_X 6
#define DELTA_BAR_CENTER_X 42
#define DELTA_BAR_RIGHT_X 78
#define DELTA_BAR_FILL_H 15

#define PEDAL_BAR_W 24
#define PEDAL_BAR_H 122
#define PEDAL_BAR_TOP_Y 214

#define UI_BATTERY_FILL_MAX_W 50
#define UI_SPEED_DIGIT_W 84
#define UI_SPEED_DIGIT_H 140
#define UI_SPEED_SEG_THICKNESS 12
#define UI_SPEED_DIGIT_GAP 18

static lv_color_t temp_to_color(int32_t temp)
{
    if(temp < 30) temp = 30;
    if(temp > 80) temp = 80;

    return lv_color_mix(lv_palette_main(LV_PALETTE_RED),
                        lv_palette_main(LV_PALETTE_GREEN),
                        (uint8_t)(((temp - 30) * 255) / 50));
}

static lv_obj_t * create_panel(lv_obj_t * parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h,
                               lv_color_t bg_color, lv_opa_t bg_opa)
{
    lv_obj_t * panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_pos(panel, x, y);
    lv_obj_set_size(panel, w, h);
    lv_obj_set_style_radius(panel, 0, 0);
    lv_obj_set_style_bg_color(panel, bg_color, 0);
    lv_obj_set_style_bg_opa(panel, bg_opa, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, UI_BORDER_COLOR, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    return panel;
}

static lv_obj_t * create_value(lv_obj_t * parent, const char * text, lv_color_t color, const lv_font_t * font)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_text_font(label, font, 0);
    return label;
}

static lv_obj_t * create_segment(lv_obj_t * parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t * segment = lv_obj_create(parent);
    lv_obj_remove_style_all(segment);
    lv_obj_set_pos(segment, x, y);
    lv_obj_set_size(segment, w, h);
    lv_obj_set_style_radius(segment, 3, 0);
    lv_obj_set_style_bg_color(segment, UI_SEGMENT_OFF_COLOR, 0);
    lv_obj_set_style_bg_opa(segment, LV_OPA_40, 0);
    return segment;
}

static void update_lap_delta_ui(void);
static void dashboard_toggle_mode(void);

static void set_speed_digit_segment_state(lv_obj_t * segment, bool enabled)
{
    lv_obj_set_style_bg_color(segment, enabled ? UI_SEGMENT_ON_COLOR : UI_SEGMENT_OFF_COLOR, 0);
    lv_obj_set_style_bg_opa(segment, enabled ? LV_OPA_COVER : LV_OPA_40, 0);
}

static void create_speed_digits(lv_obj_t * parent)
{
    const lv_coord_t digit_width = UI_SPEED_DIGIT_W;
    const lv_coord_t digit_height = UI_SPEED_DIGIT_H;
    const lv_coord_t segment_thickness = UI_SPEED_SEG_THICKNESS;
    const lv_coord_t digit_gap = UI_SPEED_DIGIT_GAP;
    const lv_coord_t mid_y = (digit_height - segment_thickness) / 2;
    const lv_coord_t bottom_y = digit_height - segment_thickness;

    g_dashboard.speed_digit_container = lv_obj_create(parent);
    lv_obj_remove_style_all(g_dashboard.speed_digit_container);
    lv_obj_set_size(g_dashboard.speed_digit_container,
                    (digit_width * 2) + digit_gap,
                    digit_height);
    lv_obj_align(g_dashboard.speed_digit_container, LV_ALIGN_CENTER, 0, -24);

    for(uint32_t digit_index = 0; digit_index < 2U; digit_index++) {
        lv_coord_t base_x = (lv_coord_t)digit_index * (digit_width + digit_gap);
        lv_obj_t ** segments = g_dashboard.speed_segments[digit_index];

        segments[0] = create_segment(g_dashboard.speed_digit_container, base_x + segment_thickness, 0,
                                     digit_width - (segment_thickness * 2), segment_thickness);
        segments[1] = create_segment(g_dashboard.speed_digit_container, base_x + digit_width - segment_thickness,
                                     segment_thickness, segment_thickness, mid_y - segment_thickness / 2);
        segments[2] = create_segment(g_dashboard.speed_digit_container, base_x + digit_width - segment_thickness,
                                     mid_y + segment_thickness / 2, segment_thickness,
                                     mid_y - segment_thickness / 2);
        segments[3] = create_segment(g_dashboard.speed_digit_container, base_x + segment_thickness, bottom_y,
                                     digit_width - (segment_thickness * 2), segment_thickness);
        segments[4] = create_segment(g_dashboard.speed_digit_container, base_x, mid_y + segment_thickness / 2,
                                     segment_thickness, mid_y - segment_thickness / 2);
        segments[5] = create_segment(g_dashboard.speed_digit_container, base_x, segment_thickness,
                                     segment_thickness, mid_y - segment_thickness / 2);
        segments[6] = create_segment(g_dashboard.speed_digit_container, base_x + segment_thickness, mid_y,
                                     digit_width - (segment_thickness * 2), segment_thickness);
    }
}

static void set_speed_digits(int32_t speed)
{
    uint8_t digits[2];

    if(speed < 0) speed = 0;
    if(speed > 99) speed = 99;

    digits[0] = (uint8_t)((speed / 10) % 10);
    digits[1] = (uint8_t)(speed % 10);

    for(uint32_t digit_index = 0; digit_index < 2U; digit_index++) {
        bool hide_digit = (digit_index == 0U) && (digits[digit_index] == 0U);

        for(uint32_t segment_index = 0; segment_index < 7U; segment_index++) {
            set_speed_digit_segment_state(
                g_dashboard.speed_segments[digit_index][segment_index],
                hide_digit ? false : (g_speed_digit_map[digits[digit_index]][segment_index] != 0U));
        }
    }
}

static void format_lap_time(char * buf, size_t buf_size, int32_t hundredths)
{
    int32_t minutes;
    int32_t seconds;
    int32_t centiseconds;

    if(hundredths < 0) hundredths = 0;

    minutes = hundredths / 6000;
    seconds = (hundredths / 100) % 60;
    centiseconds = hundredths % 100;
    lv_snprintf(buf, buf_size, "%ld:%02ld.%02ld", (long)minutes, (long)seconds, (long)centiseconds);
}

static void update_alert_ui(void)
{
    char alert_buf[48];

    if(g_dashboard.alert_label == NULL) {
        return;
    }

    if(g_alert_count == 0U) {
        lv_label_set_text(g_dashboard.alert_label, "");
        lv_obj_add_flag(g_dashboard.alert_label, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_snprintf(alert_buf, sizeof(alert_buf), "%u %s", (unsigned)g_alert_count, g_alert_queue[0]);
    lv_label_set_text(g_dashboard.alert_label, alert_buf);
    lv_obj_clear_flag(g_dashboard.alert_label, LV_OBJ_FLAG_HIDDEN);
}

static void dashboard_push_alert_local(const char * text)
{
    uint32_t text_len;

    if((text == NULL) || (text[0] == '\0')) {
        return;
    }

    for(uint32_t index = 0U; index < g_alert_count; index++) {
        if(strncmp(g_alert_queue[index], text, DASHBOARD_ALERT_TEXT_MAX) == 0) {
            update_alert_ui();
            return;
        }
    }

    if(g_alert_count >= DASHBOARD_ALERT_MAX) {
        for(uint32_t index = 1U; index < DASHBOARD_ALERT_MAX; index++) {
            (void)strncpy(g_alert_queue[index - 1U], g_alert_queue[index], DASHBOARD_ALERT_TEXT_MAX);
            g_alert_queue[index - 1U][DASHBOARD_ALERT_TEXT_MAX - 1U] = '\0';
        }
        g_alert_count = DASHBOARD_ALERT_MAX - 1U;
    }

    text_len = strlen(text);
    if(text_len >= DASHBOARD_ALERT_TEXT_MAX) {
        text_len = DASHBOARD_ALERT_TEXT_MAX - 1U;
    }
    (void)memcpy(g_alert_queue[g_alert_count], text, text_len);
    g_alert_queue[g_alert_count][text_len] = '\0';
    g_alert_count++;
    update_alert_ui();
}

static void dashboard_pop_alert(void)
{
    if(g_alert_count == 0U) {
        update_alert_ui();
        return;
    }

    for(uint32_t index = 1U; index < g_alert_count; index++) {
        (void)strncpy(g_alert_queue[index - 1U], g_alert_queue[index], DASHBOARD_ALERT_TEXT_MAX);
        g_alert_queue[index - 1U][DASHBOARD_ALERT_TEXT_MAX - 1U] = '\0';
    }

    g_alert_count--;
    if(g_alert_count < DASHBOARD_ALERT_MAX) {
        g_alert_queue[g_alert_count][0] = '\0';
    }
    update_alert_ui();
}

static void dashboard_reset_lap_ui(void)
{
    char buf[16];

    g_current_lap_time = 0;
    g_last_lap_time = 0;
    g_best_lap_time = 0;
    g_lap_delta = 0;
    g_laps_current = 0;
    g_laps_left = 0;

    format_lap_time(buf, sizeof(buf), g_best_lap_time);
    lv_label_set_text(g_dashboard.lap_best_value, buf);
    format_lap_time(buf, sizeof(buf), g_last_lap_time);
    lv_label_set_text(g_dashboard.lap_last_value, buf);
    format_lap_time(buf, sizeof(buf), g_current_lap_time);
    lv_label_set_text(g_dashboard.lap_current_value, buf);
    lv_label_set_text(g_dashboard.laps_current_value, "00");
    lv_label_set_text(g_dashboard.laps_left_value, "00");
    update_lap_delta_ui();
}

static void update_lap_analysis_indicator(uint32_t now)
{
    if(g_dashboard.alert_circle == NULL) {
        return;
    }

    if(g_lap_analysis_active == 0U) {
        g_lap_analysis_blink_visible = 0U;
        if(g_lap_analysis_indicator_visible != 0U) {
            lv_obj_add_flag(g_dashboard.alert_circle, LV_OBJ_FLAG_HIDDEN);
            g_lap_analysis_indicator_visible = 0U;
        }
        return;
    }

    if((g_lap_analysis_blink_tick == 0U) || ((now - g_lap_analysis_blink_tick) >= 500U)) {
        g_lap_analysis_blink_tick = now;
        g_lap_analysis_blink_visible = (g_lap_analysis_blink_visible == 0U) ? 1U : 0U;
    }

    if(g_lap_analysis_indicator_visible == g_lap_analysis_blink_visible) {
        return;
    }

    g_lap_analysis_indicator_visible = g_lap_analysis_blink_visible;
    if(g_lap_analysis_indicator_visible != 0U) {
        lv_obj_clear_flag(g_dashboard.alert_circle, LV_OBJ_FLAG_HIDDEN);
    }
    else {
        lv_obj_add_flag(g_dashboard.alert_circle, LV_OBJ_FLAG_HIDDEN);
    }
}

static void dashboard_toggle_lap_analysis(void)
{
	GPS_Data_t gps_data;
	float line_heading;

    if(g_lap_analysis_active != 0U) {
        g_lap_analysis_active = 0U;
        g_lap_analysis_blink_visible = 0U;
        GPS_Lap_SetAnalysisActive(0U);
        update_lap_analysis_indicator(HAL_GetTick());
        return;
    }

    GPS_GetData(&gps_data);
	if(gps_data.valid == 0U) {
		Dashboard_UI_PushAlert("gps not fixed");
		return;
	}
    if(gps_data.signal_level <= GPS_SIG_LEVEL_NONE) {
        Dashboard_UI_PushAlert("gps no signal");
        return;
    }
	line_heading = gps_data.heading_valid != 0U ?
				   gps_data.heading_angle : gps_data.track_angle;

    GPS_Lap_Reset();
    GPS_Lap_SetStartLine(gps_data.latitude,
                         gps_data.longitude,
                         line_heading);
    GPS_Lap_SetFinishLine(gps_data.latitude,
                          gps_data.longitude,
                          line_heading);
    if(GPS_Lap_StartAtCurrent(&gps_data) == 0U) {
        Dashboard_UI_PushAlert("gps position invalid");
        return;
    }
    dashboard_reset_lap_ui();
    g_lap_analysis_active = 1U;
    g_lap_analysis_blink_visible = 1U;
    g_lap_analysis_blink_tick = HAL_GetTick();
    GPS_Lap_SetAnalysisActive(1U);
    update_lap_analysis_indicator(g_lap_analysis_blink_tick);
}

static void update_lap_delta_ui(void)
{
    char delta_buf[16];
    int32_t delta_abs;
    int32_t max_abs = 50;
    lv_coord_t center_tick_x = DELTA_BAR_CENTER_X;
    lv_coord_t right_tick_x = DELTA_BAR_RIGHT_X;
    lv_coord_t fill_width;

    if((g_dashboard.delta_bar_fill == NULL) || (g_dashboard.delta_value == NULL)) {
        return;
    }

    delta_abs = g_lap_delta < 0 ? -g_lap_delta : g_lap_delta;
    if(g_lap_delta > 0) {
        lv_snprintf(delta_buf, sizeof(delta_buf), "+%ld.%02lds", (long)(delta_abs / 100), (long)(delta_abs % 100));
        lv_obj_set_style_text_color(g_dashboard.delta_value, lv_palette_main(LV_PALETTE_RED), 0);
    }
    else if(g_lap_delta < 0) {
        lv_snprintf(delta_buf, sizeof(delta_buf), "-%ld.%02lds", (long)(delta_abs / 100), (long)(delta_abs % 100));
        lv_obj_set_style_text_color(g_dashboard.delta_value, lv_palette_main(LV_PALETTE_GREEN), 0);
    }
    else {
        lv_snprintf(delta_buf, sizeof(delta_buf), "0.00s");
        lv_obj_set_style_text_color(g_dashboard.delta_value, UI_TEXT_COLOR, 0);
    }
    lv_label_set_text(g_dashboard.delta_value, delta_buf);

    if(delta_abs > max_abs) delta_abs = max_abs;
    fill_width = (lv_coord_t)((delta_abs * (right_tick_x - center_tick_x)) / max_abs);
    if((fill_width < 1) && (g_lap_delta != 0)) fill_width = 1;

    if(g_lap_delta < 0) {
        lv_obj_set_pos(g_dashboard.delta_bar_fill, center_tick_x, 0);
        lv_obj_set_size(g_dashboard.delta_bar_fill, fill_width, DELTA_BAR_FILL_H);
        lv_obj_set_style_bg_color(g_dashboard.delta_bar_fill, lv_palette_main(LV_PALETTE_GREEN), 0);
    }
    else if(g_lap_delta > 0) {
        lv_obj_set_pos(g_dashboard.delta_bar_fill, center_tick_x - fill_width, 0);
        lv_obj_set_size(g_dashboard.delta_bar_fill, fill_width, DELTA_BAR_FILL_H);
        lv_obj_set_style_bg_color(g_dashboard.delta_bar_fill, lv_palette_main(LV_PALETTE_RED), 0);
    }
    else {
        lv_obj_set_pos(g_dashboard.delta_bar_fill, center_tick_x, 0);
        lv_obj_set_size(g_dashboard.delta_bar_fill, 1, DELTA_BAR_FILL_H);
        lv_obj_set_style_bg_color(g_dashboard.delta_bar_fill, lv_color_hex(0x808080), 0);
    }
}

static void apply_drive_mode_ui(void)
{
    const char * mode_text = "S";
    lv_color_t tile_color = lv_palette_main(LV_PALETTE_RED);
    lv_color_t text_color = lv_color_hex(0xFFFFFF);

    switch(g_drive_mode) {
        case DRIVE_MODE_S:
            mode_text = "S";
            tile_color = lv_palette_main(LV_PALETTE_RED);
            break;
        case DRIVE_MODE_Q:
            mode_text = "Q";
            tile_color = lv_color_hex(0xFFD400);
            break;
        case DRIVE_MODE_C:
            mode_text = "C";
            tile_color = lv_palette_main(LV_PALETTE_BLUE);
            break;
        case DRIVE_MODE_E:
            mode_text = "E";
            tile_color = lv_palette_main(LV_PALETTE_GREEN);
            break;
        default:
            mode_text = "S";
            tile_color = lv_palette_main(LV_PALETTE_RED);
            break;
    }

    if(g_dashboard.mode_tile != NULL) {
        lv_obj_set_style_bg_color(g_dashboard.mode_tile, tile_color, 0);
    }
    if(g_dashboard.mode_value != NULL) {
        lv_label_set_text(g_dashboard.mode_value, mode_text);
        lv_obj_set_style_text_color(g_dashboard.mode_value, text_color, 0);
    }
}

static void sync_mode_from_index(void)
{
    switch(g_mode_index) {
        case 0:
            g_drive_mode = DRIVE_MODE_S;
            break;
        case 1:
            g_drive_mode = DRIVE_MODE_Q;
            break;
        case 2:
            g_drive_mode = DRIVE_MODE_C;
            break;
        case 3:
            g_drive_mode = DRIVE_MODE_E;
            break;
        default:
            g_drive_mode = DRIVE_MODE_S;
            break;
    }
}

static void apply_vehicle_ui(void)
{
    lv_color_t fl = temp_to_color(g_tire_temp_fl);
    lv_color_t fr = temp_to_color(g_tire_temp_fr);
    lv_color_t rl = temp_to_color(g_tire_temp_rl);
    lv_color_t rr = temp_to_color(g_tire_temp_rr);

    lv_obj_set_style_bg_opa(g_dashboard.wheel_fl, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(g_dashboard.wheel_fl, fl, 0);
    lv_obj_set_style_bg_grad_color(g_dashboard.wheel_fl, fl, 0);
    lv_obj_set_style_bg_grad_dir(g_dashboard.wheel_fl, LV_GRAD_DIR_VER, 0);

    lv_obj_set_style_bg_opa(g_dashboard.wheel_fr, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(g_dashboard.wheel_fr, fr, 0);
    lv_obj_set_style_bg_grad_color(g_dashboard.wheel_fr, fr, 0);
    lv_obj_set_style_bg_grad_dir(g_dashboard.wheel_fr, LV_GRAD_DIR_VER, 0);

    lv_obj_set_style_bg_opa(g_dashboard.wheel_rl, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(g_dashboard.wheel_rl, rl, 0);
    lv_obj_set_style_bg_grad_color(g_dashboard.wheel_rl, rl, 0);
    lv_obj_set_style_bg_grad_dir(g_dashboard.wheel_rl, LV_GRAD_DIR_VER, 0);

    lv_obj_set_style_bg_opa(g_dashboard.wheel_rr, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(g_dashboard.wheel_rr, rr, 0);
    lv_obj_set_style_bg_grad_color(g_dashboard.wheel_rr, rr, 0);
    lv_obj_set_style_bg_grad_dir(g_dashboard.wheel_rr, LV_GRAD_DIR_VER, 0);

    if(g_motor_fl_online) lv_obj_clear_flag(g_dashboard.lightning_fl, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(g_dashboard.lightning_fl, LV_OBJ_FLAG_HIDDEN);
    if(g_motor_fr_online) lv_obj_clear_flag(g_dashboard.lightning_fr, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(g_dashboard.lightning_fr, LV_OBJ_FLAG_HIDDEN);
    if(g_motor_rl_online) lv_obj_clear_flag(g_dashboard.lightning_rl, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(g_dashboard.lightning_rl, LV_OBJ_FLAG_HIDDEN);
    if(g_motor_rr_online) lv_obj_clear_flag(g_dashboard.lightning_rr, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(g_dashboard.lightning_rr, LV_OBJ_FLAG_HIDDEN);
}

static void update_signal_bars(int32_t level)
{
    static const lv_color_t grey  = { 0x55, 0x55, 0x55 };
    static const lv_color_t red   = { 0x1A, 0x1A, 0xFF };
    static const lv_color_t yellow = { 0x00, 0xD4, 0xFF };
    static const lv_color_t green = { 0x5A, 0xD4, 0x2F };
    lv_color_t bar_color[4];

    if(level < 0) level = 0;
    if(level > 4) level = 4;
    switch(level) {
    case 0:
        bar_color[0] = grey; bar_color[1] = grey; bar_color[2] = grey; bar_color[3] = grey;
        break;
    case 1:
        bar_color[0] = red;  bar_color[1] = grey; bar_color[2] = grey; bar_color[3] = grey;
        break;
    case 2:
        bar_color[0] = yellow; bar_color[1] = yellow; bar_color[2] = grey; bar_color[3] = grey;
        break;
    case 3:
        bar_color[0] = green; bar_color[1] = green; bar_color[2] = green; bar_color[3] = grey;
        break;
    default:
        bar_color[0] = green; bar_color[1] = green; bar_color[2] = green; bar_color[3] = green;
        break;
    }
    for(uint32_t i = 0U; i < 4U; i++) {
        if(g_dashboard.signal_bars[i] != NULL) {
            lv_obj_set_style_bg_color(g_dashboard.signal_bars[i], bar_color[i], 0);
        }
    }
}

static void dashboard_apply_data(void)
{
    static char text_buf[16];

    g_speed = g_dashboard_data.speed;                 /* Display speed: integer km/h */
    g_soc = g_dashboard_data.soc;                     /* BMS(非DBC总线) */
    g_mode_index = g_dashboard_data.mode_index;       /* DBC BO_1289 Debug9: ModeFlag */
    g_sum_voltage = g_dashboard_data.sum_voltage;     /* BMS(非DBC总线) */
    g_sum_current = g_dashboard_data.sum_current;     /* BMS(非DBC总线) */
    g_top_temperature = g_dashboard_data.max_temperature;  /* BMS(非DBC总线) */
    for(uint32_t index = 0; index < 4U; index++) {
        g_torque[index] = g_dashboard_data.torque[index];    /* DBC BO_1282 Debug2: ActualTorque */
        g_rpm[index] = g_dashboard_data.rpm[index];          /* DBC BO_1285 Debug5: ActualVelocity */
        g_motor_temp[index] = g_dashboard_data.motor_temp[index]; /* DBC BO_1286 Debug6: Motor_temperature */
    }
    g_motor_fl_online = g_dashboard_data.motor_enable[0] != 0U;  /* DBC BO_1289: FL_AMK_bEnable */
    g_motor_rl_online = g_dashboard_data.motor_enable[1] != 0U;  /* DBC BO_1289: RL_AMK_bEnable */
    g_motor_fr_online = g_dashboard_data.motor_enable[2] != 0U;  /* DBC BO_1289: FR_AMK_bEnable */
    g_motor_rr_online = g_dashboard_data.motor_enable[3] != 0U;  /* DBC BO_1289: RR_AMK_bEnable */

    int32_t display_speed = g_speed;
    if(display_speed < 0) display_speed = 0;
    if(display_speed > 99) display_speed = 99;

    set_speed_digits(display_speed);

    lv_snprintf(text_buf, sizeof(text_buf), "%ld%%", (long)g_soc);
    lv_label_set_text(g_dashboard.soc_value, text_buf);
    lv_obj_set_width(g_dashboard.battery_fill, g_soc == 0 ? 1 : (g_soc * 30 / 100));

    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_sum_voltage);
    lv_label_set_text(g_dashboard.total_voltage_value, text_buf);
    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_sum_current);
    lv_label_set_text(g_dashboard.total_current_value, text_buf);
    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_top_temperature);
    lv_label_set_text(g_dashboard.max_temp_value, text_buf);
    g_power_live = (g_sum_voltage * g_sum_current) / 100;
    if(g_power_live < 0) g_power_live = 0;
    if(g_power_live > g_power_peak) g_power_peak = g_power_live;
    if(g_dashboard.power_live_value != NULL) {
        lv_snprintf(text_buf, sizeof(text_buf), "%ldkW", (long)g_power_live);
        lv_label_set_text(g_dashboard.power_live_value, text_buf);
    }
    if(g_dashboard.power_peak_value != NULL) {
        lv_snprintf(text_buf, sizeof(text_buf), "%ldkW", (long)g_power_peak);
        lv_label_set_text(g_dashboard.power_peak_value, text_buf);
    }

    if(g_dashboard.throttle_bar_fill != NULL) {
        int32_t throttle_height = g_dashboard_data.aps_open_pct == 0 ? 1 : (g_dashboard_data.aps_open_pct * PEDAL_BAR_H / 100);
        lv_obj_set_pos(g_dashboard.throttle_bar_fill, 0, PEDAL_BAR_H - throttle_height);
        lv_obj_set_size(g_dashboard.throttle_bar_fill, PEDAL_BAR_W, throttle_height);
    }
    if(g_dashboard.brake_bar_fill != NULL) {
        int32_t brake_height = g_dashboard_data.brake_pct == 0 ? 1 : (g_dashboard_data.brake_pct * PEDAL_BAR_H / 100);
        lv_obj_set_pos(g_dashboard.brake_bar_fill, 0, PEDAL_BAR_H - brake_height);
        lv_obj_set_size(g_dashboard.brake_bar_fill, PEDAL_BAR_W, brake_height);
    }

    sync_mode_from_index();
    apply_drive_mode_ui();

    g_motor_power_live[0] = (g_torque[0] * g_rpm[0]) / 12000;
    g_motor_power_live[1] = (g_torque[1] * g_rpm[1]) / 12000;
    g_motor_power_live[2] = (g_torque[2] * g_rpm[2]) / 12000;
    g_motor_power_live[3] = (g_torque[3] * g_rpm[3]) / 12000;
    for(uint32_t i = 0; i < 4U; i++) {
        if(g_motor_power_live[i] > g_motor_power_peak[i]) {
            g_motor_power_peak[i] = g_motor_power_live[i];
        }
    }

    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_torque[0]);
    lv_label_set_text(g_dashboard.motor_fl_torque, text_buf);
    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_rpm[0]);
    lv_label_set_text(g_dashboard.motor_fl_speed, text_buf);
    if(g_dashboard.motor_fl_power_live != NULL) {
        lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_motor_power_live[0]);
        lv_label_set_text(g_dashboard.motor_fl_power_live, text_buf);
        lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_motor_power_peak[0]);
        lv_label_set_text(g_dashboard.motor_fl_power_peak, text_buf);
        lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_motor_temp[0]);
        lv_label_set_text(g_dashboard.motor_fl_temp, text_buf);
    }
    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_torque[1]);
    lv_label_set_text(g_dashboard.motor_rl_torque, text_buf);
    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_rpm[1]);
    lv_label_set_text(g_dashboard.motor_rl_speed, text_buf);
    if(g_dashboard.motor_rl_power_live != NULL) {
        lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_motor_power_live[1]);
        lv_label_set_text(g_dashboard.motor_rl_power_live, text_buf);
        lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_motor_power_peak[1]);
        lv_label_set_text(g_dashboard.motor_rl_power_peak, text_buf);
        lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_motor_temp[1]);
        lv_label_set_text(g_dashboard.motor_rl_temp, text_buf);
    }
    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_torque[2]);
    lv_label_set_text(g_dashboard.motor_fr_torque, text_buf);
    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_rpm[2]);
    lv_label_set_text(g_dashboard.motor_fr_speed, text_buf);
    if(g_dashboard.motor_fr_power_live != NULL) {
        lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_motor_power_live[2]);
        lv_label_set_text(g_dashboard.motor_fr_power_live, text_buf);
        lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_motor_power_peak[2]);
        lv_label_set_text(g_dashboard.motor_fr_power_peak, text_buf);
        lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_motor_temp[2]);
        lv_label_set_text(g_dashboard.motor_fr_temp, text_buf);
    }
    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_torque[3]);
    lv_label_set_text(g_dashboard.motor_rr_torque, text_buf);
    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_rpm[3]);
    lv_label_set_text(g_dashboard.motor_rr_speed, text_buf);
    if(g_dashboard.motor_rr_power_live != NULL) {
        lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_motor_power_live[3]);
        lv_label_set_text(g_dashboard.motor_rr_power_live, text_buf);
        lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_motor_power_peak[3]);
        lv_label_set_text(g_dashboard.motor_rr_power_peak, text_buf);
        lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_motor_temp[3]);
        lv_label_set_text(g_dashboard.motor_rr_temp, text_buf);
    }

    for(uint32_t i = 0U; i < 4U; i++) {
        int32_t mtemp = g_motor_temp[i];
        if(mtemp < 0) mtemp = 0;
        if(mtemp > 200) mtemp = 200;
        switch(i) {
            case 0: g_tire_temp_fl = mtemp; break;
            case 1: g_tire_temp_rl = mtemp; break;
            case 2: g_tire_temp_fr = mtemp; break;
            case 3: g_tire_temp_rr = mtemp; break;
        }
    }

    update_signal_bars(g_dashboard_data.signal_level);

    {
        int32_t odo_tenths = g_dashboard_data.odometer_tenths;
        int32_t odo_int = odo_tenths / 10;
        int32_t odo_frac = odo_tenths % 10;
        if(odo_frac < 0) { odo_frac = -odo_frac; odo_int = -odo_int; }
        if(odo_int > 999) odo_int = 999;
        lv_snprintf(text_buf, sizeof(text_buf), "%ld.%01ld km", (long)odo_int, (long)odo_frac);
        lv_label_set_text(g_dashboard.odometer_label, text_buf);
    }

    apply_vehicle_ui();
}

void Dashboard_UI_SubmitData(const dashboard_data_t * data)
{
    uint32_t index;

    if(data == NULL) {
        return;
    }

    g_pending_dashboard_data.speed = data->speed;
    g_pending_dashboard_data.soc = data->soc;
    g_pending_dashboard_data.mode_index = data->mode_index;
    g_pending_dashboard_data.sum_voltage = data->sum_voltage;
    g_pending_dashboard_data.sum_current = data->sum_current;
    g_pending_dashboard_data.max_temperature = data->max_temperature;
    for(index = 0; index < 4U; index++) {
        g_pending_dashboard_data.torque[index] = data->torque[index];
        g_pending_dashboard_data.motor_enable[index] = data->motor_enable[index];
        g_pending_dashboard_data.rpm[index] = data->rpm[index];
        g_pending_dashboard_data.motor_temp[index] = data->motor_temp[index];
    }
    g_pending_dashboard_data.alert_active = data->alert_active;
    g_pending_dashboard_data.odometer_tenths = data->odometer_tenths;
    g_pending_dashboard_data.brake_pct = data->brake_pct;

    g_dashboard_data_dirty = 1U;
}

void Dashboard_UI_SubmitSignalLevel(int32_t level)
{
    if(level < 0) level = 0;
    if(level > 4) level = 4;
    g_pending_signal_level = level;
    g_signal_dirty = 1U;
}

void Dashboard_UI_SubmitSpeed(int32_t speed)
{
    if(speed < 0) speed = 0;
    if(speed > 300) speed = 300;
    if((g_speed_dirty != 0U) && (g_pending_speed == speed)) {
        return;
    }
    g_pending_speed = speed;
    g_speed_dirty = 1U;
}

void Dashboard_UI_SubmitLapDelta(int32_t delta_hundredths)
{
    if(delta_hundredths > 500) delta_hundredths = 500;
    if(delta_hundredths < -500) delta_hundredths = -500;
    g_pending_lap_delta = delta_hundredths;
    g_lap_delta_dirty = 1U;
}

void Dashboard_UI_SubmitLapTimes(int32_t current_hundredths,
								 int32_t last_hundredths,
								 int32_t best_hundredths,
								 int32_t lap_count)
{
	g_pending_lap_current = current_hundredths;
	g_pending_lap_last = last_hundredths;
	g_pending_lap_best = best_hundredths;
	g_pending_lap_count = lap_count;
	g_lap_times_dirty = 1U;
}

void Dashboard_UI_PushAlert(const char * text)
{
    uint32_t primask;
    uint32_t text_len;

    if((text == NULL) || (text[0] == '\0')) {
        return;
    }

    text_len = strlen(text);
    if(text_len >= DASHBOARD_ALERT_TEXT_MAX) {
        text_len = DASHBOARD_ALERT_TEXT_MAX - 1U;
    }

    primask = __get_PRIMASK();
    __disable_irq();
    if(g_pending_alert_count >= DASHBOARD_ALERT_MAX) {
        for(uint32_t index = 1U; index < DASHBOARD_ALERT_MAX; index++) {
            (void)memcpy(g_pending_alert_queue[index - 1U],
                         g_pending_alert_queue[index],
                         DASHBOARD_ALERT_TEXT_MAX);
        }
        g_pending_alert_count = DASHBOARD_ALERT_MAX - 1U;
    }
    (void)memcpy(g_pending_alert_queue[g_pending_alert_count], text, text_len);
    g_pending_alert_queue[g_pending_alert_count][text_len] = '\0';
    g_pending_alert_count++;
    if(primask == 0U) {
        __enable_irq();
    }
}

void Dashboard_UI_Process(void)
{
    uint32_t index;
    uint32_t primask;
    int32_t pending_speed;
    int32_t pending_signal_level;
    char pending_alert_queue[DASHBOARD_ALERT_MAX][DASHBOARD_ALERT_TEXT_MAX];
    char pending_alert_text[DASHBOARD_ALERT_TEXT_MAX];
    uint8_t pending_alert_count;
    uint32_t now;

    now = HAL_GetTick();
    update_lap_analysis_indicator(now);

	if((g_dashboard_data_dirty == 0U) && (g_speed_dirty == 0U) &&
	   (g_signal_dirty == 0U) && (g_lap_delta_dirty == 0U) &&
	   (g_lap_times_dirty == 0U) &&
	   (g_pending_alert_count == 0U) && (g_pending_alert_pop_count == 0U) &&
	   (g_pending_lap_toggle == 0U) && (g_pending_mode_toggle == 0U)) {
        return;
    }

    if(g_pending_mode_toggle != 0U) {
        primask = __get_PRIMASK();
        __disable_irq();
        g_pending_mode_toggle = 0U;
        if(primask == 0U) {
            __enable_irq();
        }

        dashboard_toggle_mode();
    }

    if(g_signal_dirty != 0U) {
        primask = __get_PRIMASK();
        __disable_irq();
        pending_signal_level = g_pending_signal_level;
        g_signal_dirty = 0U;
        if(primask == 0U) {
            __enable_irq();
        }

        if(pending_signal_level < 0) pending_signal_level = 0;
        if(pending_signal_level > 4) pending_signal_level = 4;
        g_dashboard_data.signal_level = pending_signal_level;
        update_signal_bars(pending_signal_level);
    }

    if(g_pending_lap_toggle != 0U) {
        primask = __get_PRIMASK();
        __disable_irq();
        g_pending_lap_toggle = 0U;
        if(primask == 0U) {
            __enable_irq();
        }

        dashboard_toggle_lap_analysis();
    }

    while(g_pending_alert_pop_count != 0U) {
        primask = __get_PRIMASK();
        __disable_irq();
        g_pending_alert_pop_count--;
        if(primask == 0U) {
            __enable_irq();
        }

        dashboard_pop_alert();
    }

    if(g_pending_alert_count != 0U) {
        primask = __get_PRIMASK();
        __disable_irq();
        pending_alert_count = g_pending_alert_count;
        if(pending_alert_count > DASHBOARD_ALERT_MAX) {
            pending_alert_count = DASHBOARD_ALERT_MAX;
        }
        for(uint32_t alert_index = 0U; alert_index < pending_alert_count; alert_index++) {
            (void)memcpy(pending_alert_queue[alert_index],
                         g_pending_alert_queue[alert_index],
                         DASHBOARD_ALERT_TEXT_MAX);
            pending_alert_queue[alert_index][DASHBOARD_ALERT_TEXT_MAX - 1U] = '\0';
        }
        g_pending_alert_count = 0U;
        if(primask == 0U) {
            __enable_irq();
        }

        for(uint32_t alert_index = 0U; alert_index < pending_alert_count; alert_index++) {
            (void)strncpy(pending_alert_text, pending_alert_queue[alert_index], sizeof(pending_alert_text));
            pending_alert_text[sizeof(pending_alert_text) - 1U] = '\0';
            dashboard_push_alert_local(pending_alert_text);
        }
    }

    if(g_dashboard_data_dirty != 0U) {
        primask = __get_PRIMASK();
        __disable_irq();
        g_dashboard_data.speed = g_pending_dashboard_data.speed;
        g_dashboard_data.soc = g_pending_dashboard_data.soc;
        g_dashboard_data.mode_index = g_pending_dashboard_data.mode_index;
        g_dashboard_data.sum_voltage = g_pending_dashboard_data.sum_voltage;
        g_dashboard_data.sum_current = g_pending_dashboard_data.sum_current;
        g_dashboard_data.max_temperature = g_pending_dashboard_data.max_temperature;
        for(index = 0; index < 4U; index++) {
            g_dashboard_data.torque[index] = g_pending_dashboard_data.torque[index];
            g_dashboard_data.motor_enable[index] = g_pending_dashboard_data.motor_enable[index];
            g_dashboard_data.rpm[index] = g_pending_dashboard_data.rpm[index];
            g_dashboard_data.motor_temp[index] = g_pending_dashboard_data.motor_temp[index];
        }
        g_dashboard_data.alert_active = g_pending_dashboard_data.alert_active;
        g_dashboard_data.odometer_tenths = g_pending_dashboard_data.odometer_tenths;
        g_dashboard_data.brake_pct = g_pending_dashboard_data.brake_pct;
        g_dashboard_data_dirty = 0U;
        if(primask == 0U) {
            __enable_irq();
        }

        dashboard_apply_data();
    }

    if(g_speed_dirty != 0U) {
        if((g_speed_ui_last_tick == 0U) || ((now - g_speed_ui_last_tick) >= 100U)) {
            primask = __get_PRIMASK();
            __disable_irq();
            pending_speed = g_pending_speed;
            g_speed_dirty = 0U;
            if(primask == 0U) {
                __enable_irq();
            }

            if(pending_speed < 0) pending_speed = 0;
            if(pending_speed > 99) pending_speed = 99;
            g_dashboard_data.speed = pending_speed;
            if(pending_speed != g_speed) {
                g_speed = pending_speed;
                set_speed_digits(pending_speed);
            }
            g_speed_ui_last_tick = now;
        }
    }

    if(g_lap_delta_dirty != 0U) {
        g_lap_delta_dirty = 0U;
        g_lap_delta = g_pending_lap_delta;
        update_lap_delta_ui();
    }

    if(g_lap_times_dirty != 0U) {
        char lap_buf[16];

        g_lap_times_dirty = 0U;
        g_current_lap_time = g_pending_lap_current;
        g_last_lap_time = g_pending_lap_last;
        g_best_lap_time = g_pending_lap_best;
        g_laps_current = g_pending_lap_count;

        format_lap_time(lap_buf, sizeof(lap_buf), g_best_lap_time);
        lv_label_set_text(g_dashboard.lap_best_value, lap_buf);
        format_lap_time(lap_buf, sizeof(lap_buf), g_last_lap_time);
        lv_label_set_text(g_dashboard.lap_last_value, lap_buf);
        format_lap_time(lap_buf, sizeof(lap_buf), g_current_lap_time);
        lv_label_set_text(g_dashboard.lap_current_value, lap_buf);
        lv_snprintf(lap_buf, sizeof(lap_buf), "%02ld", (long)g_laps_current);
        lv_label_set_text(g_dashboard.laps_current_value, lap_buf);
    }
}

void Dashboard_UI_Init(void)
{
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen, UI_BG_COLOR, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    lv_obj_t * top_area = create_panel(screen, 0, 0, SIM_HOR_RES, UI_TOP_HEIGHT, UI_BG_COLOR, LV_OPA_COVER);
    lv_obj_set_style_border_width(top_area, 0, 0);

    g_dashboard.lap_best_label = lv_label_create(top_area);
    lv_label_set_text(g_dashboard.lap_best_label, "Best");
    lv_obj_set_style_text_color(g_dashboard.lap_best_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.lap_best_label, DASHBOARD_FONT_MEDIUM, 0);
    lv_obj_set_pos(g_dashboard.lap_best_label, 16, 16);

    g_dashboard.lap_best_value = lv_label_create(top_area);
    lv_label_set_text(g_dashboard.lap_best_value, "0:00.00");
    lv_obj_set_style_text_color(g_dashboard.lap_best_value, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.lap_best_value, DASHBOARD_FONT_MEDIUM, 0);
    lv_obj_set_pos(g_dashboard.lap_best_value, 76, 16);

    g_dashboard.lap_last_label = lv_label_create(top_area);
    lv_label_set_text(g_dashboard.lap_last_label, "Pre");
    lv_obj_set_style_text_color(g_dashboard.lap_last_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.lap_last_label, DASHBOARD_FONT_MEDIUM, 0);
    lv_obj_set_pos(g_dashboard.lap_last_label, 164, 16);

    g_dashboard.lap_last_value = lv_label_create(top_area);
    lv_label_set_text(g_dashboard.lap_last_value, "0:00.00");
    lv_obj_set_style_text_color(g_dashboard.lap_last_value, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.lap_last_value, DASHBOARD_FONT_MEDIUM, 0);
    lv_obj_set_pos(g_dashboard.lap_last_value, 214, 16);

    g_dashboard.lap_current_label = lv_label_create(top_area);
    lv_label_set_text(g_dashboard.lap_current_label, "Cur");
    lv_obj_set_style_text_color(g_dashboard.lap_current_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.lap_current_label, DASHBOARD_FONT_MEDIUM, 0);
    lv_obj_set_pos(g_dashboard.lap_current_label, 316, 16);

    g_dashboard.lap_current_value = lv_label_create(top_area);
    lv_label_set_text(g_dashboard.lap_current_value, "0:00.00");
    lv_obj_set_style_text_color(g_dashboard.lap_current_value, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.lap_current_value, DASHBOARD_FONT_MEDIUM, 0);
    lv_obj_set_pos(g_dashboard.lap_current_value, 364, 16);

    g_dashboard.delta_bar_track = lv_obj_create(top_area);
    lv_obj_remove_style_all(g_dashboard.delta_bar_track);
    lv_obj_set_pos(g_dashboard.delta_bar_track, DELTA_BAR_X, DELTA_BAR_Y);
    lv_obj_set_size(g_dashboard.delta_bar_track, DELTA_BAR_W, DELTA_BAR_H);
    lv_obj_set_style_bg_color(g_dashboard.delta_bar_track, UI_BG_COLOR, 0);
    lv_obj_set_style_bg_opa(g_dashboard.delta_bar_track, LV_OPA_COVER, 0);

    lv_obj_t * delta_bottom_line = lv_obj_create(g_dashboard.delta_bar_track);
    lv_obj_remove_style_all(delta_bottom_line);
    lv_obj_set_pos(delta_bottom_line, DELTA_BAR_INNER_X, DELTA_BAR_FILL_H);
    lv_obj_set_size(delta_bottom_line, DELTA_BAR_RIGHT_X - DELTA_BAR_INNER_X, 1);
    lv_obj_set_style_bg_color(delta_bottom_line, UI_BORDER_COLOR, 0);
    lv_obj_set_style_bg_opa(delta_bottom_line, LV_OPA_COVER, 0);

    g_dashboard.delta_bar_fill = lv_obj_create(g_dashboard.delta_bar_track);
    lv_obj_remove_style_all(g_dashboard.delta_bar_fill);
    lv_obj_set_pos(g_dashboard.delta_bar_fill, DELTA_BAR_CENTER_X, 0);
    lv_obj_set_size(g_dashboard.delta_bar_fill, 1, DELTA_BAR_FILL_H);
    lv_obj_set_style_bg_color(g_dashboard.delta_bar_fill, lv_color_hex(0x808080), 0);
    lv_obj_set_style_bg_opa(g_dashboard.delta_bar_fill, LV_OPA_COVER, 0);

    for(uint32_t tick_index = 0; tick_index < 7; tick_index++) {
        lv_obj_t * delta_tick = lv_obj_create(g_dashboard.delta_bar_track);
        lv_obj_remove_style_all(delta_tick);
        lv_coord_t tick_x = (lv_coord_t)(DELTA_BAR_INNER_X + ((tick_index * (DELTA_BAR_RIGHT_X - DELTA_BAR_INNER_X)) / 6));
        lv_obj_set_pos(delta_tick, tick_x, 1);
        lv_obj_set_size(delta_tick, 2, DELTA_BAR_FILL_H - 1);
        lv_obj_set_style_bg_color(delta_tick, UI_BORDER_COLOR, 0);
        lv_obj_set_style_bg_opa(delta_tick, LV_OPA_COVER, 0);
        lv_obj_move_foreground(delta_tick);
    }

    g_dashboard.delta_bar_center = lv_obj_create(g_dashboard.delta_bar_track);
    lv_obj_remove_style_all(g_dashboard.delta_bar_center);
    lv_obj_set_pos(g_dashboard.delta_bar_center, DELTA_BAR_CENTER_X, 1);
    lv_obj_set_size(g_dashboard.delta_bar_center, 2, DELTA_BAR_FILL_H - 1);
    lv_obj_set_style_bg_color(g_dashboard.delta_bar_center, UI_TEXT_COLOR, 0);
    lv_obj_set_style_bg_opa(g_dashboard.delta_bar_center, LV_OPA_COVER, 0);

    g_dashboard.delta_value = lv_label_create(top_area);
    lv_label_set_text(g_dashboard.delta_value, "0.00s");
    lv_obj_set_style_text_color(g_dashboard.delta_value, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.delta_value, DASHBOARD_FONT_MEDIUM, 0);
    lv_obj_set_pos(g_dashboard.delta_value, 526, 15);

    g_dashboard.laps_current_label = lv_label_create(top_area);
    lv_label_set_text(g_dashboard.laps_current_label, "Lap");
    lv_obj_set_style_text_color(g_dashboard.laps_current_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.laps_current_label, DASHBOARD_FONT_MEDIUM, 0);
    lv_obj_set_pos(g_dashboard.laps_current_label, 628, 16);

    g_dashboard.laps_current_value = lv_label_create(top_area);
    lv_label_set_text(g_dashboard.laps_current_value, "00");
    lv_obj_set_style_text_color(g_dashboard.laps_current_value, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.laps_current_value, DASHBOARD_FONT_MEDIUM, 0);
    lv_obj_set_pos(g_dashboard.laps_current_value, 664, 16);

    g_dashboard.laps_left_label = lv_label_create(top_area);
    lv_label_set_text(g_dashboard.laps_left_label, "Left");
    lv_obj_set_style_text_color(g_dashboard.laps_left_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.laps_left_label, DASHBOARD_FONT_MEDIUM, 0);
    lv_obj_set_pos(g_dashboard.laps_left_label, 704, 16);

    g_dashboard.laps_left_value = lv_label_create(top_area);
    lv_label_set_text(g_dashboard.laps_left_value, "00");
    lv_obj_set_style_text_color(g_dashboard.laps_left_value, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.laps_left_value, DASHBOARD_FONT_MEDIUM, 0);
    lv_obj_set_pos(g_dashboard.laps_left_value, 748, 16);

    lv_obj_t * middle_panel = create_panel(screen, 0, UI_MIDDLE_Y, SIM_HOR_RES, UI_MIDDLE_HEIGHT, UI_BG_COLOR, LV_OPA_COVER);
    lv_obj_set_style_border_width(middle_panel, 0, 0);

    lv_obj_t * mode_box = create_panel(middle_panel, UI_RIGHT_PANEL_X, 0, UI_RIGHT_PANEL_WIDTH, UI_MIDDLE_HEIGHT - 9, UI_BG_COLOR, LV_OPA_COVER);
    lv_obj_set_style_border_width(mode_box, 0, 0);

    lv_obj_t * battery_outline = create_panel(mode_box, 18, 18, 54, 24, UI_BG_COLOR, LV_OPA_COVER);
    lv_obj_set_style_border_width(battery_outline, 1, 0);
    lv_obj_set_style_pad_all(battery_outline, 0, 0);

    lv_obj_t * battery_cap = lv_obj_create(mode_box);
    lv_obj_remove_style_all(battery_cap);
    lv_obj_set_pos(battery_cap, 72, 24);
    lv_obj_set_size(battery_cap, 5, 12);
    lv_obj_set_style_bg_color(battery_cap, UI_TEXT_COLOR, 0);
    lv_obj_set_style_bg_opa(battery_cap, LV_OPA_COVER, 0);

    g_dashboard.battery_fill = lv_obj_create(battery_outline);
    lv_obj_remove_style_all(g_dashboard.battery_fill);
    lv_obj_set_pos(g_dashboard.battery_fill, 2, 2);
    lv_obj_set_size(g_dashboard.battery_fill, 36, 20);
    lv_obj_set_style_bg_color(g_dashboard.battery_fill, UI_TEXT_COLOR, 0);
    lv_obj_set_style_bg_opa(g_dashboard.battery_fill, LV_OPA_COVER, 0);

    g_dashboard.soc_value = lv_label_create(mode_box);
    lv_label_set_text(g_dashboard.soc_value, "24%");
    lv_obj_set_style_text_color(g_dashboard.soc_value, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.soc_value, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(g_dashboard.soc_value, 86, 18);

    lv_obj_t * power_live_label = lv_label_create(mode_box);
    lv_label_set_text(power_live_label, "P NOW:");
    lv_obj_set_style_text_color(power_live_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(power_live_label, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(power_live_label, 16, 60);

    g_dashboard.power_live_value = lv_label_create(mode_box);
    lv_label_set_text(g_dashboard.power_live_value, "12kW");
    lv_obj_set_style_text_color(g_dashboard.power_live_value, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.power_live_value, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(g_dashboard.power_live_value, 98, 60);

    lv_obj_t * power_peak_label = lv_label_create(mode_box);
    lv_label_set_text(power_peak_label, "P PEAK:");
    lv_obj_set_style_text_color(power_peak_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(power_peak_label, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(power_peak_label, 16, 88);

    g_dashboard.power_peak_value = lv_label_create(mode_box);
    lv_label_set_text(g_dashboard.power_peak_value, "36kW");
    lv_obj_set_style_text_color(g_dashboard.power_peak_value, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.power_peak_value, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(g_dashboard.power_peak_value, 98, 88);

    lv_obj_t * voltage_label = lv_label_create(mode_box);
    lv_label_set_text(voltage_label, "TOTAL V:");
    lv_obj_set_style_text_color(voltage_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(voltage_label, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(voltage_label, 16, 126);

    g_dashboard.total_voltage_value = lv_label_create(mode_box);
    lv_label_set_text(g_dashboard.total_voltage_value, "24");
    lv_obj_set_style_text_color(g_dashboard.total_voltage_value, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.total_voltage_value, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(g_dashboard.total_voltage_value, 98, 126);

    lv_obj_t * current_label = lv_label_create(mode_box);
    lv_label_set_text(current_label, "TOTAL A:");
    lv_obj_set_style_text_color(current_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(current_label, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(current_label, 16, 154);

    g_dashboard.total_current_value = lv_label_create(mode_box);
    lv_label_set_text(g_dashboard.total_current_value, "24");
    lv_obj_set_style_text_color(g_dashboard.total_current_value, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.total_current_value, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(g_dashboard.total_current_value, 98, 154);

    lv_obj_t * max_temp_label = lv_label_create(mode_box);
    lv_label_set_text(max_temp_label, "MAX T:");
    lv_obj_set_style_text_color(max_temp_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(max_temp_label, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(max_temp_label, 16, 182);

    g_dashboard.max_temp_value = lv_label_create(mode_box);
    lv_label_set_text(g_dashboard.max_temp_value, "24");
    lv_obj_set_style_text_color(g_dashboard.max_temp_value, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.max_temp_value, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(g_dashboard.max_temp_value, 88, 182);

    lv_obj_t * brake_bar_track = lv_obj_create(mode_box);
    lv_obj_remove_style_all(brake_bar_track);
    lv_obj_set_pos(brake_bar_track, 40, PEDAL_BAR_TOP_Y);
    lv_obj_set_size(brake_bar_track, PEDAL_BAR_W, PEDAL_BAR_H);
    lv_obj_set_style_bg_color(brake_bar_track, UI_BG_COLOR, 0);
    lv_obj_set_style_bg_opa(brake_bar_track, LV_OPA_COVER, 0);

    lv_obj_t * brake_limit = lv_obj_create(mode_box);
    lv_obj_remove_style_all(brake_limit);
    lv_obj_set_pos(brake_limit, 40, PEDAL_BAR_TOP_Y - 8);
    lv_obj_set_size(brake_limit, PEDAL_BAR_W, 5);
    lv_obj_set_style_bg_color(brake_limit, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_bg_opa(brake_limit, LV_OPA_COVER, 0);

    g_dashboard.brake_bar_fill = lv_obj_create(brake_bar_track);
    lv_obj_remove_style_all(g_dashboard.brake_bar_fill);
    lv_obj_set_pos(g_dashboard.brake_bar_fill, 0, PEDAL_BAR_H - 1);
    lv_obj_set_size(g_dashboard.brake_bar_fill, PEDAL_BAR_W, 1);
    lv_obj_set_style_bg_color(g_dashboard.brake_bar_fill, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_bg_opa(g_dashboard.brake_bar_fill, LV_OPA_COVER, 0);

    lv_obj_t * throttle_bar_track = lv_obj_create(mode_box);
    lv_obj_remove_style_all(throttle_bar_track);
    lv_obj_set_pos(throttle_bar_track, 104, PEDAL_BAR_TOP_Y);
    lv_obj_set_size(throttle_bar_track, PEDAL_BAR_W, PEDAL_BAR_H);
    lv_obj_set_style_bg_color(throttle_bar_track, UI_BG_COLOR, 0);
    lv_obj_set_style_bg_opa(throttle_bar_track, LV_OPA_COVER, 0);

    lv_obj_t * throttle_limit = lv_obj_create(mode_box);
    lv_obj_remove_style_all(throttle_limit);
    lv_obj_set_pos(throttle_limit, 104, PEDAL_BAR_TOP_Y - 8);
    lv_obj_set_size(throttle_limit, PEDAL_BAR_W, 5);
    lv_obj_set_style_bg_color(throttle_limit, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_bg_opa(throttle_limit, LV_OPA_COVER, 0);

    g_dashboard.throttle_bar_fill = lv_obj_create(throttle_bar_track);
    lv_obj_remove_style_all(g_dashboard.throttle_bar_fill);
    lv_obj_set_pos(g_dashboard.throttle_bar_fill, 0, PEDAL_BAR_H - 1);
    lv_obj_set_size(g_dashboard.throttle_bar_fill, PEDAL_BAR_W, 1);
    lv_obj_set_style_bg_color(g_dashboard.throttle_bar_fill, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_bg_opa(g_dashboard.throttle_bar_fill, LV_OPA_COVER, 0);

    lv_obj_t * speed_box = create_panel(middle_panel, UI_CENTER_PANEL_X, 72, UI_CENTER_PANEL_WIDTH, 210, UI_BG_COLOR, LV_OPA_COVER);
    lv_obj_set_style_border_width(speed_box, 0, 0);

    create_speed_digits(speed_box);

    lv_obj_t * speed_unit = lv_label_create(speed_box);
    lv_label_set_text(speed_unit, "km/h");
    lv_obj_set_style_text_color(speed_unit, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_opa(speed_unit, LV_OPA_60, 0);
    lv_obj_set_style_text_font(speed_unit, DASHBOARD_FONT_SMALL, 0);
    lv_obj_align(speed_unit, LV_ALIGN_CENTER, 0, 72);

    lv_obj_t * vehicle_box = create_panel(middle_panel, 0, 0, UI_LEFT_PANEL_WIDTH, UI_MIDDLE_HEIGHT - 9, UI_BG_COLOR, LV_OPA_COVER);
    lv_obj_set_style_border_width(vehicle_box, 0, 0);

    g_dashboard.mode_tile = create_panel(vehicle_box, 52, 10, 76, 76, lv_palette_main(LV_PALETTE_RED), LV_OPA_COVER);
    lv_obj_set_style_border_width(g_dashboard.mode_tile, 2, 0);
    lv_obj_set_style_radius(g_dashboard.mode_tile, 0, 0);

    g_dashboard.mode_value = lv_label_create(g_dashboard.mode_tile);
    lv_obj_set_style_text_font(g_dashboard.mode_value, DASHBOARD_FONT_LARGE, 0);
    lv_obj_align(g_dashboard.mode_value, LV_ALIGN_CENTER, 0, 0);
    apply_drive_mode_ui();

    g_dashboard.wheel_fl = create_panel(vehicle_box, 30, 160, 24, 54, UI_BG_COLOR, LV_OPA_TRANSP);
    lv_obj_set_style_radius(g_dashboard.wheel_fl, 3, 0);
    g_dashboard.lightning_fl = lv_label_create(vehicle_box);
    lv_label_set_text(g_dashboard.lightning_fl, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(g_dashboard.lightning_fl, lv_color_hex(0xFFD400), 0);
    lv_obj_set_style_text_font(g_dashboard.lightning_fl, DASHBOARD_FONT_LARGE, 0);
    lv_obj_align_to(g_dashboard.lightning_fl, g_dashboard.wheel_fl, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    g_dashboard.wheel_fr = create_panel(vehicle_box, 126, 160, 24, 54, UI_BG_COLOR, LV_OPA_TRANSP);
    lv_obj_set_style_radius(g_dashboard.wheel_fr, 3, 0);
    g_dashboard.lightning_fr = lv_label_create(vehicle_box);
    lv_label_set_text(g_dashboard.lightning_fr, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(g_dashboard.lightning_fr, lv_color_hex(0xFFD400), 0);
    lv_obj_set_style_text_font(g_dashboard.lightning_fr, DASHBOARD_FONT_LARGE, 0);
    lv_obj_align_to(g_dashboard.lightning_fr, g_dashboard.wheel_fr, LV_ALIGN_OUT_LEFT_MID, 0, 0);

    g_dashboard.wheel_rl = create_panel(vehicle_box, 30, 228, 24, 54, UI_BG_COLOR, LV_OPA_TRANSP);
    lv_obj_set_style_radius(g_dashboard.wheel_rl, 3, 0);
    g_dashboard.lightning_rl = lv_label_create(vehicle_box);
    lv_label_set_text(g_dashboard.lightning_rl, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(g_dashboard.lightning_rl, lv_color_hex(0xFFD400), 0);
    lv_obj_set_style_text_font(g_dashboard.lightning_rl, DASHBOARD_FONT_LARGE, 0);
    lv_obj_align_to(g_dashboard.lightning_rl, g_dashboard.wheel_rl, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    g_dashboard.wheel_rr = create_panel(vehicle_box, 126, 228, 24, 54, UI_BG_COLOR, LV_OPA_TRANSP);
    lv_obj_set_style_radius(g_dashboard.wheel_rr, 3, 0);
    g_dashboard.lightning_rr = lv_label_create(vehicle_box);
    lv_label_set_text(g_dashboard.lightning_rr, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(g_dashboard.lightning_rr, lv_color_hex(0xFFD400), 0);
    lv_obj_set_style_text_font(g_dashboard.lightning_rr, DASHBOARD_FONT_LARGE, 0);
    lv_obj_align_to(g_dashboard.lightning_rr, g_dashboard.wheel_rr, LV_ALIGN_OUT_LEFT_MID, 0, 0);

    apply_vehicle_ui();

    lv_obj_t * bottom_info = create_panel(screen, 0, UI_BOTTOM_Y, SIM_HOR_RES, UI_BOTTOM_HEIGHT, UI_BG_COLOR, LV_OPA_COVER);
    lv_obj_set_style_border_width(bottom_info, 0, 0);

    lv_obj_t * motor_fl_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_fl_label, "LF T:");
    lv_obj_set_style_text_color(motor_fl_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_fl_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_fl_label, 14, 10);
    g_dashboard.motor_fl_torque = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_fl_torque, "24");
    lv_obj_set_style_text_color(g_dashboard.motor_fl_torque, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_fl_torque, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_fl_torque, 68, 10);
    g_dashboard.motor_fl_speed = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_fl_speed, "24");
    lv_obj_set_style_text_color(g_dashboard.motor_fl_speed, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_fl_speed, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_fl_speed, 68, 30);

    lv_obj_t * motor_fl_speed_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_fl_speed_label, "LF N:");
    lv_obj_set_style_text_color(motor_fl_speed_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_fl_speed_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_fl_speed_label, 14, 30);

    lv_obj_t * motor_fl_power_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_fl_power_label, "P:");
    lv_obj_set_style_text_color(motor_fl_power_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_fl_power_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_fl_power_label, 102, 10);
    g_dashboard.motor_fl_power_live = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_fl_power_live, "10");
    lv_obj_set_style_text_color(g_dashboard.motor_fl_power_live, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_fl_power_live, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_fl_power_live, 140, 10);

    lv_obj_t * motor_fl_peak_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_fl_peak_label, "Pk:");
    lv_obj_set_style_text_color(motor_fl_peak_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_fl_peak_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_fl_peak_label, 102, 30);
    g_dashboard.motor_fl_power_peak = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_fl_power_peak, "24");
    lv_obj_set_style_text_color(g_dashboard.motor_fl_power_peak, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_fl_power_peak, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_fl_power_peak, 150, 30);

    lv_obj_t * motor_fl_temp_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_fl_temp_label, "Tm:");
    lv_obj_set_style_text_color(motor_fl_temp_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_fl_temp_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_fl_temp_label, 14, 50);
    g_dashboard.motor_fl_temp = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_fl_temp, "48");
    lv_obj_set_style_text_color(g_dashboard.motor_fl_temp, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_fl_temp, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_fl_temp, 68, 50);

    lv_obj_t * motor_fr_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_fr_label, "RF T:");
    lv_obj_set_style_text_color(motor_fr_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_fr_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_fr_label, 214, 10);
    g_dashboard.motor_fr_torque = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_fr_torque, "24");
    lv_obj_set_style_text_color(g_dashboard.motor_fr_torque, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_fr_torque, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_fr_torque, 268, 10);
    g_dashboard.motor_fr_speed = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_fr_speed, "24");
    lv_obj_set_style_text_color(g_dashboard.motor_fr_speed, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_fr_speed, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_fr_speed, 268, 30);

    lv_obj_t * motor_fr_speed_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_fr_speed_label, "RF N:");
    lv_obj_set_style_text_color(motor_fr_speed_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_fr_speed_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_fr_speed_label, 214, 30);

    lv_obj_t * motor_fr_power_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_fr_power_label, "P:");
    lv_obj_set_style_text_color(motor_fr_power_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_fr_power_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_fr_power_label, 302, 10);
    g_dashboard.motor_fr_power_live = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_fr_power_live, "10");
    lv_obj_set_style_text_color(g_dashboard.motor_fr_power_live, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_fr_power_live, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_fr_power_live, 340, 10);

    lv_obj_t * motor_fr_peak_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_fr_peak_label, "Pk:");
    lv_obj_set_style_text_color(motor_fr_peak_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_fr_peak_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_fr_peak_label, 302, 30);
    g_dashboard.motor_fr_power_peak = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_fr_power_peak, "24");
    lv_obj_set_style_text_color(g_dashboard.motor_fr_power_peak, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_fr_power_peak, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_fr_power_peak, 350, 30);

    lv_obj_t * motor_fr_temp_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_fr_temp_label, "Tm:");
    lv_obj_set_style_text_color(motor_fr_temp_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_fr_temp_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_fr_temp_label, 214, 50);
    g_dashboard.motor_fr_temp = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_fr_temp, "47");
    lv_obj_set_style_text_color(g_dashboard.motor_fr_temp, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_fr_temp, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_fr_temp, 268, 50);

    lv_obj_t * motor_rl_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_rl_label, "LR T:");
    lv_obj_set_style_text_color(motor_rl_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_rl_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_rl_label, 414, 10);
    g_dashboard.motor_rl_torque = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_rl_torque, "24");
    lv_obj_set_style_text_color(g_dashboard.motor_rl_torque, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_rl_torque, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_rl_torque, 468, 10);
    g_dashboard.motor_rl_speed = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_rl_speed, "24");
    lv_obj_set_style_text_color(g_dashboard.motor_rl_speed, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_rl_speed, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_rl_speed, 468, 30);

    lv_obj_t * motor_rl_speed_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_rl_speed_label, "LR N:");
    lv_obj_set_style_text_color(motor_rl_speed_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_rl_speed_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_rl_speed_label, 414, 30);

    lv_obj_t * motor_rl_power_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_rl_power_label, "P:");
    lv_obj_set_style_text_color(motor_rl_power_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_rl_power_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_rl_power_label, 502, 10);
    g_dashboard.motor_rl_power_live = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_rl_power_live, "9");
    lv_obj_set_style_text_color(g_dashboard.motor_rl_power_live, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_rl_power_live, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_rl_power_live, 540, 10);

    lv_obj_t * motor_rl_peak_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_rl_peak_label, "Pk:");
    lv_obj_set_style_text_color(motor_rl_peak_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_rl_peak_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_rl_peak_label, 502, 30);
    g_dashboard.motor_rl_power_peak = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_rl_power_peak, "23");
    lv_obj_set_style_text_color(g_dashboard.motor_rl_power_peak, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_rl_power_peak, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_rl_power_peak, 550, 30);

    lv_obj_t * motor_rl_temp_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_rl_temp_label, "Tm:");
    lv_obj_set_style_text_color(motor_rl_temp_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_rl_temp_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_rl_temp_label, 414, 50);
    g_dashboard.motor_rl_temp = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_rl_temp, "49");
    lv_obj_set_style_text_color(g_dashboard.motor_rl_temp, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_rl_temp, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_rl_temp, 468, 50);

    lv_obj_t * motor_rr_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_rr_label, "RR T:");
    lv_obj_set_style_text_color(motor_rr_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_rr_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_rr_label, 614, 10);
    g_dashboard.motor_rr_torque = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_rr_torque, "24");
    lv_obj_set_style_text_color(g_dashboard.motor_rr_torque, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_rr_torque, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_rr_torque, 668, 10);
    g_dashboard.motor_rr_speed = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_rr_speed, "24");
    lv_obj_set_style_text_color(g_dashboard.motor_rr_speed, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_rr_speed, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_rr_speed, 668, 30);

    lv_obj_t * motor_rr_speed_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_rr_speed_label, "RR N:");
    lv_obj_set_style_text_color(motor_rr_speed_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_rr_speed_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_rr_speed_label, 614, 30);

    lv_obj_t * motor_rr_power_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_rr_power_label, "P:");
    lv_obj_set_style_text_color(motor_rr_power_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_rr_power_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_rr_power_label, 702, 10);
    g_dashboard.motor_rr_power_live = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_rr_power_live, "9");
    lv_obj_set_style_text_color(g_dashboard.motor_rr_power_live, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_rr_power_live, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_rr_power_live, 740, 10);

    lv_obj_t * motor_rr_peak_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_rr_peak_label, "Pk:");
    lv_obj_set_style_text_color(motor_rr_peak_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_rr_peak_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_rr_peak_label, 702, 30);
    g_dashboard.motor_rr_power_peak = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_rr_power_peak, "23");
    lv_obj_set_style_text_color(g_dashboard.motor_rr_power_peak, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_rr_power_peak, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_rr_power_peak, 750, 30);

    lv_obj_t * motor_rr_temp_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_rr_temp_label, "Tm:");
    lv_obj_set_style_text_color(motor_rr_temp_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(motor_rr_temp_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_rr_temp_label, 614, 50);
    g_dashboard.motor_rr_temp = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_rr_temp, "50");
    lv_obj_set_style_text_color(g_dashboard.motor_rr_temp, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(g_dashboard.motor_rr_temp, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_rr_temp, 668, 50);

    for(uint32_t separator_index = 1; separator_index < 4; separator_index++) {
        lv_obj_t * motor_separator = lv_obj_create(bottom_info);
        lv_obj_remove_style_all(motor_separator);
        lv_obj_set_pos(motor_separator, (lv_coord_t)(separator_index * 200), 0);
        lv_obj_set_size(motor_separator, 2, UI_BOTTOM_HEIGHT);
        lv_obj_set_style_bg_color(motor_separator, UI_BORDER_COLOR, 0);
        lv_obj_set_style_bg_opa(motor_separator, LV_OPA_COVER, 0);
        lv_obj_move_foreground(motor_separator);
    }

    lv_obj_t * separator_top = lv_obj_create(screen);
    lv_obj_remove_style_all(separator_top);
    lv_obj_set_pos(separator_top, 0, UI_TOP_HEIGHT);
    lv_obj_set_size(separator_top, SIM_HOR_RES, 1);
    lv_obj_set_style_bg_color(separator_top, UI_BORDER_COLOR, 0);
    lv_obj_set_style_bg_opa(separator_top, LV_OPA_COVER, 0);

    lv_obj_t * separator_bottom = lv_obj_create(screen);
    lv_obj_remove_style_all(separator_bottom);
    lv_obj_set_pos(separator_bottom, 0, UI_BOTTOM_Y - 1);
    lv_obj_set_size(separator_bottom, SIM_HOR_RES, 1);
    lv_obj_set_style_bg_color(separator_bottom, UI_BORDER_COLOR, 0);
    lv_obj_set_style_bg_opa(separator_bottom, LV_OPA_COVER, 0);

    lv_obj_t * separator_value_top = lv_obj_create(screen);
    lv_obj_remove_style_all(separator_value_top);
    lv_obj_set_pos(separator_value_top, UI_CENTER_PANEL_X, UI_MIDDLE_Y + 72);
    lv_obj_set_size(separator_value_top, UI_CENTER_PANEL_WIDTH, 1);
    lv_obj_set_style_bg_color(separator_value_top, UI_BORDER_COLOR, 0);
    lv_obj_set_style_bg_opa(separator_value_top, LV_OPA_COVER, 0);

    {
        static const int bar_heights[4] = {12, 24, 36, 48};
        const lv_coord_t bar_w = 8;
        const lv_coord_t bar_gap = 2;
        const lv_coord_t total_w = (lv_coord_t)(4 * bar_w + 3 * bar_gap);
        const lv_coord_t bar_x = UI_RIGHT_PANEL_X - total_w - 2;
        const lv_coord_t bar_base_y = (lv_coord_t)(UI_TOP_HEIGHT + 1 + 72 - 2);
        const lv_color_t grey  = { 0x55, 0x55, 0x55 };

        for(uint32_t i = 0U; i < 4U; i++) {
            g_dashboard.signal_bars[i] = lv_obj_create(screen);
            lv_obj_set_pos(g_dashboard.signal_bars[i],
                (lv_coord_t)(bar_x + i * (bar_w + bar_gap)),
                (lv_coord_t)(bar_base_y - bar_heights[i]));
            lv_obj_set_size(g_dashboard.signal_bars[i], bar_w, (lv_coord_t)bar_heights[i]);
            lv_obj_set_style_border_width(g_dashboard.signal_bars[i], 0, 0);
            lv_obj_set_style_pad_all(g_dashboard.signal_bars[i], 0, 0);
            lv_obj_set_style_radius(g_dashboard.signal_bars[i], 0, 0);
            lv_obj_set_style_bg_color(g_dashboard.signal_bars[i], grey, 0);
            lv_obj_set_style_bg_opa(g_dashboard.signal_bars[i], LV_OPA_COVER, 0);
        }
    }

    {
        const lv_coord_t circle_d = 24;
        const lv_coord_t circle_x = UI_CENTER_PANEL_X + 12;
        const lv_coord_t circle_y = (lv_coord_t)(UI_MIDDLE_Y + 1 + (72 - circle_d) / 2);
        g_dashboard.alert_circle = lv_obj_create(screen);
        lv_obj_set_pos(g_dashboard.alert_circle, circle_x, circle_y);
        lv_obj_set_size(g_dashboard.alert_circle, circle_d, circle_d);
        lv_obj_set_style_border_width(g_dashboard.alert_circle, 0, 0);
        lv_obj_set_style_pad_all(g_dashboard.alert_circle, 0, 0);
        lv_obj_set_style_radius(g_dashboard.alert_circle, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(g_dashboard.alert_circle, lv_color_make(0xFF, 0x1A, 0x1A), 0);
        lv_obj_set_style_bg_opa(g_dashboard.alert_circle, LV_OPA_COVER, 0);
        lv_obj_add_flag(g_dashboard.alert_circle, LV_OBJ_FLAG_HIDDEN);

        g_dashboard.odometer_label = lv_label_create(screen);
        lv_obj_set_pos(g_dashboard.odometer_label, circle_x + circle_d + 12, circle_y - 3);
        lv_obj_set_style_text_color(g_dashboard.odometer_label, UI_TEXT_COLOR, 0);
        lv_obj_set_style_text_font(g_dashboard.odometer_label, DASHBOARD_FONT_SMALL, 0);
        lv_label_set_text(g_dashboard.odometer_label, "41.2 km");

        g_dashboard.alert_label = lv_label_create(screen);
        lv_obj_set_pos(g_dashboard.alert_label, ALERT_TOUCH_X_MIN, circle_y - 3);
        lv_obj_set_size(g_dashboard.alert_label, ALERT_TOUCH_X_MAX - ALERT_TOUCH_X_MIN, circle_d + 6);
        lv_label_set_long_mode(g_dashboard.alert_label, LV_LABEL_LONG_MODE_CLIP);
        lv_obj_set_style_text_color(g_dashboard.alert_label, lv_palette_main(LV_PALETTE_RED), 0);
        lv_obj_set_style_text_font(g_dashboard.alert_label, DASHBOARD_FONT_SMALL, 0);
        lv_label_set_text(g_dashboard.alert_label, "");
        lv_obj_add_flag(g_dashboard.alert_label, LV_OBJ_FLAG_HIDDEN);
        update_alert_ui();
    }

    lv_obj_t * speed_left_separator = lv_obj_create(screen);
    lv_obj_remove_style_all(speed_left_separator);
    lv_obj_set_pos(speed_left_separator, UI_LEFT_PANEL_WIDTH, UI_MIDDLE_Y);
    lv_obj_set_size(speed_left_separator, 1, UI_MIDDLE_HEIGHT);
    lv_obj_set_style_bg_color(speed_left_separator, UI_BORDER_COLOR, 0);
    lv_obj_set_style_bg_opa(speed_left_separator, LV_OPA_COVER, 0);

    lv_obj_t * speed_right_separator = lv_obj_create(screen);
    lv_obj_remove_style_all(speed_right_separator);
    lv_obj_set_pos(speed_right_separator, UI_RIGHT_PANEL_X, UI_MIDDLE_Y);
    lv_obj_set_size(speed_right_separator, 1, UI_MIDDLE_HEIGHT);
    lv_obj_set_style_bg_color(speed_right_separator, UI_BORDER_COLOR, 0);
    lv_obj_set_style_bg_opa(speed_right_separator, LV_OPA_COVER, 0);

    lv_screen_load(screen);
    dashboard_apply_data();
}

static void dashboard_toggle_mode(void)
{
    g_drive_mode = (drive_mode_t)(((int32_t)g_drive_mode + 1) % 4);
    g_mode_index = (int32_t)g_drive_mode;
    g_dashboard_data.mode_index = g_mode_index;
    apply_drive_mode_ui();
    apply_vehicle_ui();
    CAN_RequestDriveMode(g_mode_index);
}

void Dashboard_UI_SubmitTouchState(uint16_t x, uint16_t y, uint8_t pressed)
{
    static uint8_t last_pressed = 0U;
    static uint32_t last_toggle_tick = 0U;
    static uint32_t last_lap_toggle_tick = 0U;
    static uint32_t last_alert_touch_tick = 0U;
    uint32_t now = HAL_GetTick();
    uint8_t in_mode_area = (uint8_t)((x >= MODE_TOUCH_X_MIN) && (x < MODE_TOUCH_X_MAX) &&
                                     (y >= MODE_TOUCH_Y_MIN) && (y < MODE_TOUCH_Y_MAX));
    uint8_t in_lap_area = (uint8_t)((x >= LAP_TOUCH_X_MIN) && (x < LAP_TOUCH_X_MAX) &&
                                    (y >= LAP_TOUCH_Y_MIN) && (y < LAP_TOUCH_Y_MAX));
    uint8_t in_alert_area = (uint8_t)((x >= ALERT_TOUCH_X_MIN) && (x < ALERT_TOUCH_X_MAX) &&
                                      (y >= ALERT_TOUCH_Y_MIN) && (y < ALERT_TOUCH_Y_MAX));

    if((pressed != 0U) && (last_pressed == 0U)) {
        if((in_lap_area != 0U) && ((now - last_lap_toggle_tick) >= 200U)) {
            last_lap_toggle_tick = now;
            g_pending_lap_toggle = 1U;
        }
        else if((in_alert_area != 0U) && ((now - last_alert_touch_tick) >= 200U)) {
            last_alert_touch_tick = now;
            if(g_pending_alert_pop_count < 255U) {
                g_pending_alert_pop_count++;
            }
        }
        else if((in_mode_area != 0U) && ((now - last_toggle_tick) >= 200U)) {
            last_toggle_tick = now;
            g_pending_mode_toggle = 1U;
        }
    }

    last_pressed = pressed != 0U ? 1U : 0U;
}

const dashboard_data_t * Dashboard_UI_GetCurrentData(void)
{
    return &g_dashboard_data;
}
