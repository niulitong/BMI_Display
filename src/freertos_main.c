/**
 * @file    Freertos main file
 * @author  MootSeeker
 * @date    2024-09-02
 * @brief   Main file for FreeRTOS tasks and hooks.
 * @license MIT License
 */

#include "lvgl/lvgl.h"

#include "FreeRTOS.h"
#include "task.h"

#if LV_USE_OS == LV_OS_FREERTOS

#include "hal/hal.h"
#include <stdio.h>
#include <SDL.h>

typedef struct {
    lv_obj_t * speed_unit;
    lv_obj_t * speed_bar_mask;
    lv_obj_t * speed_bar_gradient;
    lv_obj_t * speed_digit_container;
    lv_obj_t * speed_segments[2][7];
    lv_obj_t * mode_value;
    lv_obj_t * soc_value;
    lv_obj_t * battery_fill;
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
    lv_obj_t * motor_fl_torque;
    lv_obj_t * motor_fl_speed;
    lv_obj_t * motor_fr_torque;
    lv_obj_t * motor_fr_speed;
    lv_obj_t * motor_rl_torque;
    lv_obj_t * motor_rl_speed;
    lv_obj_t * motor_rr_torque;
    lv_obj_t * motor_rr_speed;
    lv_obj_t * key_target;
} dashboard_ui_t;

static dashboard_ui_t g_dashboard;

typedef enum {
    DRIVE_MODE_S = 0,
    DRIVE_MODE_E,
    DRIVE_MODE_C
} drive_mode_t;

static drive_mode_t g_drive_mode = DRIVE_MODE_S;
static int speed = 0;
static int SOC = 72;
static int Mode_Index = 0;
static int torque_M[4] = {120, 118, 116, 114};
static int RPM[4] = {800, 790, 780, 770};
static int Sum_Voltage = 72;
static int Top_Temperature = 46;
static int Sum_I = 15;
static int Tire_Temp_FL = 35;
static int Tire_Temp_FR = 48;
static int Tire_Temp_RL = 58;
static int Tire_Temp_RR = 66;
static bool Motor_FL_Online = false;
static bool Motor_FR_Online = true;
static bool Motor_RL_Online = false;
static bool Motor_RR_Online = true;

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

static lv_color_t temp_to_color(int32_t temp)
{
    if(temp < 30) temp = 30;
    if(temp > 80) temp = 80;

    uint8_t mix = (uint8_t)(((temp - 30) * 255) / 50);
    return lv_color_mix(lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_GREEN), mix);
}

static void apply_vehicle_ui(void)
{
    lv_color_t fl = temp_to_color(Tire_Temp_FL);
    lv_color_t fr = temp_to_color(Tire_Temp_FR);
    lv_color_t rl = temp_to_color(Tire_Temp_RL);
    lv_color_t rr = temp_to_color(Tire_Temp_RR);

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

    if(Motor_FL_Online) lv_obj_clear_flag(g_dashboard.lightning_fl, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(g_dashboard.lightning_fl, LV_OBJ_FLAG_HIDDEN);
    if(Motor_FR_Online) lv_obj_clear_flag(g_dashboard.lightning_fr, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(g_dashboard.lightning_fr, LV_OBJ_FLAG_HIDDEN);
    if(Motor_RL_Online) lv_obj_clear_flag(g_dashboard.lightning_rl, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(g_dashboard.lightning_rl, LV_OBJ_FLAG_HIDDEN);
    if(Motor_RR_Online) lv_obj_clear_flag(g_dashboard.lightning_rr, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(g_dashboard.lightning_rr, LV_OBJ_FLAG_HIDDEN);
}

static void apply_drive_mode_ui(void)
{
    if(g_dashboard.mode_value == NULL) return;

    switch(g_drive_mode) {
        case DRIVE_MODE_S:
            lv_label_set_text(g_dashboard.mode_value, "S");
            lv_obj_set_style_text_color(g_dashboard.mode_value, lv_palette_main(LV_PALETTE_RED), 0);
            break;
        case DRIVE_MODE_E:
            lv_label_set_text(g_dashboard.mode_value, "E");
            lv_obj_set_style_text_color(g_dashboard.mode_value, lv_palette_main(LV_PALETTE_GREEN), 0);
            break;
        case DRIVE_MODE_C:
        default:
            lv_label_set_text(g_dashboard.mode_value, "C");
            lv_obj_set_style_text_color(g_dashboard.mode_value, lv_palette_main(LV_PALETTE_BLUE), 0);
            break;
    }
}

static void sync_mode_from_index(void)
{
    switch(Mode_Index) {
        case 0:
            g_drive_mode = DRIVE_MODE_C;
            break;
        case 1:
            g_drive_mode = DRIVE_MODE_E;
            break;
        case 2:
        default:
            g_drive_mode = DRIVE_MODE_S;
            break;
    }
}

static void mode_key_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    if(key == 's' || key == 'S') {
        g_drive_mode = DRIVE_MODE_S;
    }
    else if(key == 'e' || key == 'E') {
        g_drive_mode = DRIVE_MODE_E;
    }
    else if(key == 'c' || key == 'C') {
        g_drive_mode = DRIVE_MODE_C;
    }
    else {
        return;
    }

    apply_drive_mode_ui();
}

// ........................................................................................................
/**
 * @brief   Malloc failed hook
 *
 * This function is called when a memory allocation (malloc) fails. It logs the available heap size and enters
 * an infinite loop to halt the system.
 *
 * @param   None
 * @return  None
 */
void vApplicationMallocFailedHook(void)
{
    printf("Malloc failed! Available heap: %ld bytes\n", xPortGetFreeHeapSize());
    for( ;; );
}

// ........................................................................................................
/**
 * @brief   Idle hook
 *
 * This function is called when the system is idle. It can be used for low-power mode operations or other
 * maintenance tasks that need to run when the CPU is not busy.
 *
 * @param   None
 * @return  None
 */
void vApplicationIdleHook(void) {}

// ........................................................................................................
/**
 * @brief   Stack overflow hook
 *
 * This function is called when a stack overflow is detected in a task. It logs the task name and enters
 * an infinite loop to halt the system.
 *
 * @param   xTask        Handle of the task that caused the stack overflow
 * @param   pcTaskName   Name of the task that caused the stack overflow
 * @return  None
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("Stack overflow in task %s\n", pcTaskName);
    for(;;);
}

// ........................................................................................................
/**
 * @brief   Tick hook
 *
 * This function is called on each tick interrupt. It can be used to execute periodic operations
 * that need to occur at a fixed time interval.
 *
 * @param   None
 * @return  None
 */
void vApplicationTickHook(void) {}

// ........................................................................................................
/**
 * @brief   Create main dashboard screen
 *
 * This function creates a simple LVGL screen with a "Hello, World!" label centered on the screen.
 *
 * @param   None
 * @return  None
 */
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
    lv_obj_set_style_border_color(panel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    return panel;
}

static lv_obj_t * create_value(lv_obj_t * parent, const char * text, lv_color_t color, int32_t font_size)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_text_font(label, font_size >= 28 ? &lv_font_montserrat_28 : &lv_font_montserrat_20, 0);
    return label;
}

static lv_obj_t * create_segment(lv_obj_t * parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t * segment = lv_obj_create(parent);
    lv_obj_remove_style_all(segment);
    lv_obj_set_pos(segment, x, y);
    lv_obj_set_size(segment, w, h);
    lv_obj_set_style_radius(segment, 3, 0);
    lv_obj_set_style_bg_color(segment, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_bg_opa(segment, LV_OPA_40, 0);
    return segment;
}

static void set_speed_digit_segment_state(lv_obj_t * segment, bool enabled)
{
    lv_obj_set_style_bg_color(segment, enabled ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_bg_opa(segment, enabled ? LV_OPA_COVER : LV_OPA_40, 0);
}

static void create_speed_digits(lv_obj_t * parent)
{
    const lv_coord_t digit_width = 50;
    const lv_coord_t digit_height = 82;
    const lv_coord_t segment_thickness = 8;
    const lv_coord_t digit_gap = 12;
    const lv_coord_t mid_y = (digit_height - segment_thickness) / 2;
    const lv_coord_t bottom_y = digit_height - segment_thickness;

    g_dashboard.speed_digit_container = lv_obj_create(parent);
    lv_obj_remove_style_all(g_dashboard.speed_digit_container);
    lv_obj_set_size(g_dashboard.speed_digit_container,
                    (digit_width * 2) + digit_gap,
                    digit_height);
    lv_obj_align(g_dashboard.speed_digit_container, LV_ALIGN_CENTER, -8, -16);

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

static void set_speed_digits(int32_t speed_value)
{
    uint8_t digits[2];

    if(speed_value < 0) speed_value = 0;
    if(speed_value > 99) speed_value = 99;

    digits[0] = (uint8_t)((speed_value / 10) % 10);
    digits[1] = (uint8_t)(speed_value % 10);

    for(uint32_t digit_index = 0; digit_index < 2U; digit_index++) {
        bool hide_digit = (digit_index == 0U) && (digits[digit_index] == 0U);

        for(uint32_t segment_index = 0; segment_index < 7U; segment_index++) {
            set_speed_digit_segment_state(
                g_dashboard.speed_segments[digit_index][segment_index],
                hide_digit ? false : (g_speed_digit_map[digits[digit_index]][segment_index] != 0U));
        }
    }
}

void create_main_dashboard_screen(void)
{
    lv_obj_t * screen = lv_obj_create(NULL);
    if(screen == NULL) {
        printf("Error: Failed to create dashboard screen\n");
        return;
    }

    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    lv_obj_t * top_area = create_panel(screen, 0, 0, 480, 35, lv_color_hex(0x000000), LV_OPA_COVER);
    lv_obj_set_style_border_width(top_area, 0, 0);

    g_dashboard.speed_bar_mask = lv_obj_create(top_area);
    lv_obj_remove_style_all(g_dashboard.speed_bar_mask);
    lv_obj_set_pos(g_dashboard.speed_bar_mask, 0, 0);
    lv_obj_set_size(g_dashboard.speed_bar_mask, 180, 35);
    lv_obj_set_style_bg_opa(g_dashboard.speed_bar_mask, LV_OPA_TRANSP, 0);
    lv_obj_set_style_clip_corner(g_dashboard.speed_bar_mask, true, 0);

    g_dashboard.speed_bar_gradient = lv_obj_create(g_dashboard.speed_bar_mask);
    lv_obj_remove_style_all(g_dashboard.speed_bar_gradient);
    lv_obj_set_pos(g_dashboard.speed_bar_gradient, 0, 0);
    lv_obj_set_size(g_dashboard.speed_bar_gradient, 480, 35);
    lv_obj_set_style_bg_color(g_dashboard.speed_bar_gradient, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_bg_grad_color(g_dashboard.speed_bar_gradient, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_bg_grad_dir(g_dashboard.speed_bar_gradient, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(g_dashboard.speed_bar_gradient, LV_OPA_COVER, 0);

    lv_obj_t * middle_panel = create_panel(screen, 0, 36, 480, 185, lv_color_hex(0x000000), LV_OPA_COVER);
    lv_obj_set_style_border_width(middle_panel, 0, 0);

    lv_obj_t * mode_box = create_panel(middle_panel, 370, 0, 110, 176, lv_color_hex(0x000000), LV_OPA_COVER);
    lv_obj_set_style_border_width(mode_box, 0, 0);
    lv_obj_set_style_border_side(mode_box, LV_BORDER_SIDE_RIGHT, 0);

    lv_obj_t * battery_outline = create_panel(mode_box, 34, 18, 42, 18, lv_color_hex(0x000000), LV_OPA_COVER);
    lv_obj_set_style_border_width(battery_outline, 1, 0);
    lv_obj_set_style_pad_all(battery_outline, 0, 0);

    lv_obj_t * battery_cap = lv_obj_create(mode_box);
    lv_obj_remove_style_all(battery_cap);
    lv_obj_set_pos(battery_cap, 76, 23);
    lv_obj_set_size(battery_cap, 4, 8);
    lv_obj_set_style_bg_color(battery_cap, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(battery_cap, LV_OPA_COVER, 0);

    g_dashboard.battery_fill = lv_obj_create(battery_outline);
    lv_obj_remove_style_all(g_dashboard.battery_fill);
    lv_obj_set_pos(g_dashboard.battery_fill, 2, 2);
    lv_obj_set_size(g_dashboard.battery_fill, 26, 14);
    lv_obj_set_style_bg_color(g_dashboard.battery_fill, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(g_dashboard.battery_fill, LV_OPA_COVER, 0);

    lv_obj_t * soc_label = lv_label_create(mode_box);
    lv_label_set_text(soc_label, "SOC(%):");
    lv_obj_set_style_text_color(soc_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(soc_label, &lv_font_montserrat_12, 0);
    lv_obj_align(soc_label, LV_ALIGN_TOP_MID, 0, 48);

    g_dashboard.soc_value = lv_label_create(mode_box);
    lv_label_set_text(g_dashboard.soc_value, "72");
    lv_obj_set_style_text_color(g_dashboard.soc_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.soc_value, &lv_font_montserrat_18, 0);
    lv_obj_align(g_dashboard.soc_value, LV_ALIGN_TOP_MID, 0, 68);

    lv_obj_t * voltage_label = lv_label_create(mode_box);
    lv_label_set_text(voltage_label, "TOTAL V:");
    lv_obj_set_style_text_color(voltage_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(voltage_label, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(voltage_label, 10, 118);

    g_dashboard.total_voltage_value = lv_label_create(mode_box);
    lv_label_set_text(g_dashboard.total_voltage_value, "72");
    lv_obj_set_style_text_color(g_dashboard.total_voltage_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.total_voltage_value, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(g_dashboard.total_voltage_value, 72, 118);

    lv_obj_t * current_label = lv_label_create(mode_box);
    lv_label_set_text(current_label, "TOTAL A:");
    lv_obj_set_style_text_color(current_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(current_label, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(current_label, 10, 136);

    g_dashboard.total_current_value = lv_label_create(mode_box);
    lv_label_set_text(g_dashboard.total_current_value, "15");
    lv_obj_set_style_text_color(g_dashboard.total_current_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.total_current_value, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(g_dashboard.total_current_value, 72, 136);

    lv_obj_t * max_temp_label = lv_label_create(mode_box);
    lv_label_set_text(max_temp_label, "MAX T:");
    lv_obj_set_style_text_color(max_temp_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(max_temp_label, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(max_temp_label, 10, 154);

    g_dashboard.max_temp_value = lv_label_create(mode_box);
    lv_label_set_text(g_dashboard.max_temp_value, "46");
    lv_obj_set_style_text_color(g_dashboard.max_temp_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.max_temp_value, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(g_dashboard.max_temp_value, 60, 154);

    lv_obj_t * speed_box = create_panel(middle_panel, 110, 26, 260, 124, lv_color_hex(0x000000), LV_OPA_COVER);
    lv_obj_set_style_border_width(speed_box, 0, 0);
    lv_obj_set_style_pad_all(speed_box, 0, 0);

    create_speed_digits(speed_box);

    g_dashboard.speed_unit = lv_label_create(speed_box);
    lv_label_set_text(g_dashboard.speed_unit, "km/h");
    lv_obj_set_style_text_color(g_dashboard.speed_unit, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(g_dashboard.speed_unit, LV_OPA_60, 0);
    lv_obj_set_style_text_font(g_dashboard.speed_unit, &lv_font_montserrat_14, 0);
    lv_obj_align(g_dashboard.speed_unit, LV_ALIGN_CENTER, -8, 44);

    lv_obj_t * vehicle_box = create_panel(middle_panel, 0, 0, 110, 176, lv_color_hex(0x000000), LV_OPA_COVER);
    lv_obj_set_style_border_width(vehicle_box, 0, 0);

    g_dashboard.mode_value = lv_label_create(vehicle_box);
    lv_obj_set_style_text_font(g_dashboard.mode_value, &lv_font_montserrat_48, 0);
    lv_obj_align(g_dashboard.mode_value, LV_ALIGN_TOP_MID, 0, 6);
    apply_drive_mode_ui();

    g_dashboard.wheel_fl = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(g_dashboard.wheel_fl);
    lv_obj_set_pos(g_dashboard.wheel_fl, 18, 84);
    lv_obj_set_size(g_dashboard.wheel_fl, 16, 30);
    lv_obj_set_style_radius(g_dashboard.wheel_fl, 3, 0);
    lv_obj_set_style_bg_opa(g_dashboard.wheel_fl, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_dashboard.wheel_fl, 1, 0);
    lv_obj_set_style_border_color(g_dashboard.wheel_fl, lv_color_hex(0xFFFFFF), 0);

    g_dashboard.lightning_fl = lv_label_create(vehicle_box);
    lv_label_set_text(g_dashboard.lightning_fl, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(g_dashboard.lightning_fl, lv_color_hex(0xFFD400), 0);
    lv_obj_set_pos(g_dashboard.lightning_fl, 38, 92);

    g_dashboard.wheel_fr = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(g_dashboard.wheel_fr);
    lv_obj_set_pos(g_dashboard.wheel_fr, 80, 84);
    lv_obj_set_size(g_dashboard.wheel_fr, 16, 30);
    lv_obj_set_style_radius(g_dashboard.wheel_fr, 3, 0);
    lv_obj_set_style_bg_opa(g_dashboard.wheel_fr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_dashboard.wheel_fr, 1, 0);
    lv_obj_set_style_border_color(g_dashboard.wheel_fr, lv_color_hex(0xFFFFFF), 0);

    g_dashboard.lightning_fr = lv_label_create(vehicle_box);
    lv_label_set_text(g_dashboard.lightning_fr, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(g_dashboard.lightning_fr, lv_color_hex(0xFFD400), 0);
    lv_obj_align_to(g_dashboard.lightning_fr, g_dashboard.wheel_fr, LV_ALIGN_OUT_LEFT_MID, 0, 0);

    g_dashboard.wheel_rl = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(g_dashboard.wheel_rl);
    lv_obj_set_pos(g_dashboard.wheel_rl, 18, 120);
    lv_obj_set_size(g_dashboard.wheel_rl, 16, 30);
    lv_obj_set_style_radius(g_dashboard.wheel_rl, 3, 0);
    lv_obj_set_style_bg_opa(g_dashboard.wheel_rl, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_dashboard.wheel_rl, 1, 0);
    lv_obj_set_style_border_color(g_dashboard.wheel_rl, lv_color_hex(0xFFFFFF), 0);

    g_dashboard.lightning_rl = lv_label_create(vehicle_box);
    lv_label_set_text(g_dashboard.lightning_rl, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(g_dashboard.lightning_rl, lv_color_hex(0xFFD400), 0);
    lv_obj_set_pos(g_dashboard.lightning_rl, 38, 128);

    g_dashboard.wheel_rr = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(g_dashboard.wheel_rr);
    lv_obj_set_pos(g_dashboard.wheel_rr, 80, 120);
    lv_obj_set_size(g_dashboard.wheel_rr, 16, 30);
    lv_obj_set_style_radius(g_dashboard.wheel_rr, 3, 0);
    lv_obj_set_style_bg_opa(g_dashboard.wheel_rr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_dashboard.wheel_rr, 1, 0);
    lv_obj_set_style_border_color(g_dashboard.wheel_rr, lv_color_hex(0xFFFFFF), 0);

    g_dashboard.lightning_rr = lv_label_create(vehicle_box);
    lv_label_set_text(g_dashboard.lightning_rr, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(g_dashboard.lightning_rr, lv_color_hex(0xFFD400), 0);
    lv_obj_align_to(g_dashboard.lightning_rr, g_dashboard.wheel_rr, LV_ALIGN_OUT_LEFT_MID, 0, 0);

    apply_vehicle_ui();

    lv_obj_t * bottom_info = create_panel(screen, 0, 222, 480, 50, lv_color_hex(0x000000), LV_OPA_COVER);
    lv_obj_set_style_border_width(bottom_info, 0, 0);

    lv_obj_t * motor_fl_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_fl_label, "LF T:");
    lv_obj_set_style_text_color(motor_fl_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(motor_fl_label, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(motor_fl_label, 8, 6);
    g_dashboard.motor_fl_torque = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_fl_torque, "120");
    lv_obj_set_style_text_color(g_dashboard.motor_fl_torque, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.motor_fl_torque, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(g_dashboard.motor_fl_torque, 54, 6);
    g_dashboard.motor_fl_speed = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_fl_speed, "850");
    lv_obj_set_style_text_color(g_dashboard.motor_fl_speed, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.motor_fl_speed, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(g_dashboard.motor_fl_speed, 54, 24);

    lv_obj_t * motor_fl_speed_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_fl_speed_label, "LF N:");
    lv_obj_set_style_text_color(motor_fl_speed_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(motor_fl_speed_label, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(motor_fl_speed_label, 8, 24);

    lv_obj_t * motor_fr_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_fr_label, "RF T:");
    lv_obj_set_style_text_color(motor_fr_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(motor_fr_label, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(motor_fr_label, 126, 6);
    g_dashboard.motor_fr_torque = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_fr_torque, "118");
    lv_obj_set_style_text_color(g_dashboard.motor_fr_torque, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.motor_fr_torque, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(g_dashboard.motor_fr_torque, 172, 6);
    g_dashboard.motor_fr_speed = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_fr_speed, "840");
    lv_obj_set_style_text_color(g_dashboard.motor_fr_speed, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.motor_fr_speed, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(g_dashboard.motor_fr_speed, 172, 24);

    lv_obj_t * motor_fr_speed_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_fr_speed_label, "RF N:");
    lv_obj_set_style_text_color(motor_fr_speed_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(motor_fr_speed_label, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(motor_fr_speed_label, 126, 24);

    lv_obj_t * motor_rl_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_rl_label, "LR T:");
    lv_obj_set_style_text_color(motor_rl_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(motor_rl_label, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(motor_rl_label, 244, 6);
    g_dashboard.motor_rl_torque = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_rl_torque, "116");
    lv_obj_set_style_text_color(g_dashboard.motor_rl_torque, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.motor_rl_torque, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(g_dashboard.motor_rl_torque, 290, 6);
    g_dashboard.motor_rl_speed = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_rl_speed, "830");
    lv_obj_set_style_text_color(g_dashboard.motor_rl_speed, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.motor_rl_speed, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(g_dashboard.motor_rl_speed, 290, 24);

    lv_obj_t * motor_rl_speed_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_rl_speed_label, "LR N:");
    lv_obj_set_style_text_color(motor_rl_speed_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(motor_rl_speed_label, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(motor_rl_speed_label, 244, 24);

    lv_obj_t * motor_rr_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_rr_label, "RR T:");
    lv_obj_set_style_text_color(motor_rr_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(motor_rr_label, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(motor_rr_label, 362, 6);
    g_dashboard.motor_rr_torque = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_rr_torque, "114");
    lv_obj_set_style_text_color(g_dashboard.motor_rr_torque, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.motor_rr_torque, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(g_dashboard.motor_rr_torque, 408, 6);
    g_dashboard.motor_rr_speed = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_rr_speed, "820");
    lv_obj_set_style_text_color(g_dashboard.motor_rr_speed, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.motor_rr_speed, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(g_dashboard.motor_rr_speed, 408, 24);

    lv_obj_t * motor_rr_speed_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_rr_speed_label, "RR N:");
    lv_obj_set_style_text_color(motor_rr_speed_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(motor_rr_speed_label, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(motor_rr_speed_label, 362, 24);

    lv_obj_t * separator_top = lv_obj_create(screen);
    lv_obj_remove_style_all(separator_top);
    lv_obj_set_pos(separator_top, 0, 35);
    lv_obj_set_size(separator_top, 480, 1);
    lv_obj_set_style_bg_color(separator_top, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(separator_top, LV_OPA_COVER, 0);

    lv_obj_t * separator_bottom = lv_obj_create(screen);
    lv_obj_remove_style_all(separator_bottom);
    lv_obj_set_pos(separator_bottom, 0, 221);
    lv_obj_set_size(separator_bottom, 480, 1);
    lv_obj_set_style_bg_color(separator_bottom, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(separator_bottom, LV_OPA_COVER, 0);

    lv_obj_t * speed_left_separator = lv_obj_create(screen);
    lv_obj_remove_style_all(speed_left_separator);
    lv_obj_set_pos(speed_left_separator, 110, 36);
    lv_obj_set_size(speed_left_separator, 1, 185);
    lv_obj_set_style_bg_color(speed_left_separator, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(speed_left_separator, LV_OPA_COVER, 0);

    lv_obj_t * speed_right_separator = lv_obj_create(screen);
    lv_obj_remove_style_all(speed_right_separator);
    lv_obj_set_pos(speed_right_separator, 370, 36);
    lv_obj_set_size(speed_right_separator, 1, 185);
    lv_obj_set_style_bg_color(speed_right_separator, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(speed_right_separator, LV_OPA_COVER, 0);

    g_dashboard.key_target = lv_button_create(screen);
    lv_obj_set_size(g_dashboard.key_target, 1, 1);
    lv_obj_set_style_opa(g_dashboard.key_target, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(g_dashboard.key_target, mode_key_event_cb, LV_EVENT_KEY, NULL);

    lv_scr_load(screen);
    lv_group_focus_obj(g_dashboard.key_target);
    lv_refr_now(NULL);
}

static void update_main_dashboard_demo(void)
{
    static int32_t delta = 2;
    static char speed_buf[8];
    static int32_t soc_delta = -1;
    static char soc_buf[8];

    speed += delta;
    if(speed >= 100) {
        speed = 100;
        delta = -2;
    }
    else if(speed <= 0) {
        speed = 0;
        delta = 2;
    }

    int display_speed = speed;
    if(display_speed < 0) display_speed = 0;
    if(display_speed > 99) display_speed = 99;

    set_speed_digits(display_speed);
    lv_obj_set_width(g_dashboard.speed_bar_mask, display_speed == 0 ? 1 : (display_speed * 480 / 100));

    SOC += soc_delta;
    if(SOC >= 100) {
        SOC = 100;
        soc_delta = -1;
    }
    else if(SOC <= 0) {
        SOC = 0;
        soc_delta = 1;
    }

    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)SOC);
    lv_label_set_text(g_dashboard.soc_value, soc_buf);
    lv_obj_set_width(g_dashboard.battery_fill, SOC == 0 ? 1 : (SOC * 38 / 100));

    Sum_Voltage = 72 + speed / 10;
    Sum_I = 15 + speed / 8;
    Top_Temperature = 46 + speed / 20;
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)Sum_Voltage);
    lv_label_set_text(g_dashboard.total_voltage_value, soc_buf);
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)Sum_I);
    lv_label_set_text(g_dashboard.total_current_value, soc_buf);
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)Top_Temperature);
    lv_label_set_text(g_dashboard.max_temp_value, soc_buf);

    Mode_Index = (speed / 34) % 3;
    sync_mode_from_index();
    apply_drive_mode_ui();

    torque_M[0] = 120 + speed / 2;
    torque_M[1] = 118 + speed / 2;
    torque_M[2] = 116 + speed / 2;
    torque_M[3] = 114 + speed / 2;
    RPM[0] = 800 + speed * 8;
    RPM[1] = 790 + speed * 8;
    RPM[2] = 780 + speed * 8;
    RPM[3] = 770 + speed * 8;

    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)torque_M[0]);
    lv_label_set_text(g_dashboard.motor_fl_torque, soc_buf);
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)RPM[0]);
    lv_label_set_text(g_dashboard.motor_fl_speed, soc_buf);
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)torque_M[1]);
    lv_label_set_text(g_dashboard.motor_fr_torque, soc_buf);
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)RPM[1]);
    lv_label_set_text(g_dashboard.motor_fr_speed, soc_buf);
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)torque_M[2]);
    lv_label_set_text(g_dashboard.motor_rl_torque, soc_buf);
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)RPM[2]);
    lv_label_set_text(g_dashboard.motor_rl_speed, soc_buf);
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)torque_M[3]);
    lv_label_set_text(g_dashboard.motor_rr_torque, soc_buf);
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)RPM[3]);
    lv_label_set_text(g_dashboard.motor_rr_speed, soc_buf);

    Tire_Temp_FL = 30 + speed / 2;
    Tire_Temp_FR = 36 + speed / 2;
    Tire_Temp_RL = 42 + speed / 2;
    Tire_Temp_RR = 48 + speed / 2;

    Motor_FL_Online = ((speed / 10) % 2) != 0;
    Motor_FR_Online = ((speed / 12) % 2) != 0;
    Motor_RL_Online = ((speed / 14) % 2) != 0;
    Motor_RR_Online = ((speed / 16) % 2) != 0;
    apply_vehicle_ui();
}

// ........................................................................................................
/**
 * @brief   LVGL task
 *
 * This task initializes LVGL and runs the main loop, periodically calling the LVGL task handler.
 * It is responsible for managing the LVGL state and rendering updates.
 *
 * @param   pvParameters   Task parameters (not used in this example)
 * @return  None
 */
void lvgl_task(void *pvParameters)
{

    /*Initialize LVGL*/
    lv_init();

    /*Initialize the HAL (display, input devices, tick) for LVGL*/
    sdl_hal_init(480, 272);
    /* Show main dashboard screen */
    create_main_dashboard_screen();

    while (true){
        update_main_dashboard_demo();
        lv_timer_handler(); /* Handle LVGL tasks */
        vTaskDelay(pdMS_TO_TICKS(30)); /* Short delay for the RTOS scheduler */
    }
}

// ........................................................................................................
/**
 * @brief   Another task
 *
 * This task simulates some background work by periodically printing a message.
 *
 * @param   pvParameters   Task parameters (not used in this example)
 * @return  None
 */
void another_task(void *pvParameters)
{
    /* Create some load in an infinite loop */
    while (true){
        printf("Second Task is running :)\n");
        /* Delay the task for 500 milliseconds */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ........................................................................................................
/**
 * @brief   FreeRTOS main function
 *
 * This function sets up and starts the FreeRTOS tasks, including the LVGL task and another demo task.
 *
 * @param   None
 * @return  None
 */
int main(int argc, char **argv)
{
    LV_UNUSED(argc);
    LV_UNUSED(argv);
    /* Initialize LVGL (Light and Versatile Graphics Library) and other resources */

    /* Create the LVGL task */
    if (xTaskCreate(lvgl_task, "LVGL Task", 4096, NULL, 1, NULL) != pdPASS) {
        printf("Error creating LVGL task\n");
        /* Error handling */
    }

    /* Create another task */
    if (xTaskCreate(another_task, "Another Task", 1024, NULL, 1, NULL) != pdPASS) {
        printf("Error creating another task\n");
        /* Error handling */
    }

    /* Start the scheduler */
    vTaskStartScheduler();
}

#endif
