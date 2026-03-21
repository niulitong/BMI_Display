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
    lv_obj_t * speed_value;
    lv_obj_t * speed_unit;
    lv_obj_t * speed_bar_mask;
    lv_obj_t * speed_bar_gradient;
    lv_obj_t * mode_value;
    lv_obj_t * soc_value;
    lv_obj_t * battery_fill;
    lv_obj_t * wheel_fl;
    lv_obj_t * wheel_fr;
    lv_obj_t * wheel_rl;
    lv_obj_t * wheel_rr;
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
static int32_t g_soc_value = 72;
static bool g_wheel_fl_alarm = false;
static bool g_wheel_fr_alarm = true;
static bool g_wheel_rl_alarm = false;
static bool g_wheel_rr_alarm = true;
static int32_t g_total_voltage = 72;
static int32_t g_total_current = 15;
static int32_t g_max_temp = 46;

static void apply_wheel_ui(lv_obj_t * wheel_obj, bool alarm)
{
    if(wheel_obj == NULL) return;
    lv_obj_set_style_bg_color(wheel_obj, alarm ? lv_palette_main(LV_PALETTE_RED) : lv_color_hex(0xFFFFFF), 0);
}

static void apply_vehicle_ui(void)
{
    apply_wheel_ui(g_dashboard.wheel_fl, g_wheel_fl_alarm);
    apply_wheel_ui(g_dashboard.wheel_fr, g_wheel_fr_alarm);
    apply_wheel_ui(g_dashboard.wheel_rl, g_wheel_rl_alarm);
    apply_wheel_ui(g_dashboard.wheel_rr, g_wheel_rr_alarm);
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

    g_dashboard.speed_value = create_value(speed_box, "68", lv_color_hex(0xFFFFFF), 28);
    lv_obj_set_style_text_font(g_dashboard.speed_value, &lv_font_montserrat_48, 0);
    lv_obj_set_style_transform_zoom(g_dashboard.speed_value, 420, 0);
    lv_obj_align(g_dashboard.speed_value, LV_ALIGN_CENTER, -18, -22);

    g_dashboard.speed_unit = lv_label_create(speed_box);
    lv_label_set_text(g_dashboard.speed_unit, "km/h");
    lv_obj_set_style_text_color(g_dashboard.speed_unit, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(g_dashboard.speed_unit, LV_OPA_60, 0);
    lv_obj_set_style_text_font(g_dashboard.speed_unit, &lv_font_montserrat_14, 0);
    lv_obj_align(g_dashboard.speed_unit, LV_ALIGN_CENTER, -12, 40);

    lv_obj_t * vehicle_box = create_panel(middle_panel, 0, 0, 110, 176, lv_color_hex(0x000000), LV_OPA_COVER);
    lv_obj_set_style_border_width(vehicle_box, 0, 0);

    g_dashboard.mode_value = lv_label_create(vehicle_box);
    lv_obj_set_style_text_font(g_dashboard.mode_value, &lv_font_montserrat_48, 0);
    lv_obj_align(g_dashboard.mode_value, LV_ALIGN_TOP_MID, 0, 6);
    apply_drive_mode_ui();

    lv_obj_t * front_wing = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(front_wing);
    lv_obj_set_pos(front_wing, 24, 52);
    lv_obj_set_size(front_wing, 62, 5);
    lv_obj_set_style_border_width(front_wing, 1, 0);
    lv_obj_set_style_border_color(front_wing, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(front_wing, LV_OPA_TRANSP, 0);

    lv_obj_t * nose = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(nose);
    lv_obj_set_pos(nose, 49, 57);
    lv_obj_set_size(nose, 12, 26);
    lv_obj_set_style_border_width(nose, 1, 0);
    lv_obj_set_style_border_color(nose, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(nose, LV_OPA_TRANSP, 0);

    lv_obj_t * front_arm_l = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(front_arm_l);
    lv_obj_set_pos(front_arm_l, 31, 72);
    lv_obj_set_size(front_arm_l, 18, 1);
    lv_obj_set_style_bg_color(front_arm_l, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(front_arm_l, LV_OPA_COVER, 0);

    lv_obj_t * front_arm_r = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(front_arm_r);
    lv_obj_set_pos(front_arm_r, 61, 72);
    lv_obj_set_size(front_arm_r, 18, 1);
    lv_obj_set_style_bg_color(front_arm_r, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(front_arm_r, LV_OPA_COVER, 0);

    lv_obj_t * chassis = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(chassis);
    lv_obj_set_pos(chassis, 40, 82);
    lv_obj_set_size(chassis, 30, 62);
    lv_obj_set_style_border_width(chassis, 1, 0);
    lv_obj_set_style_border_color(chassis, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(chassis, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(chassis, 6, 0);

    lv_obj_t * sidepod_l = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(sidepod_l);
    lv_obj_set_pos(sidepod_l, 28, 98);
    lv_obj_set_size(sidepod_l, 12, 26);
    lv_obj_set_style_border_width(sidepod_l, 1, 0);
    lv_obj_set_style_border_color(sidepod_l, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(sidepod_l, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(sidepod_l, 3, 0);

    lv_obj_t * sidepod_r = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(sidepod_r);
    lv_obj_set_pos(sidepod_r, 70, 98);
    lv_obj_set_size(sidepod_r, 12, 26);
    lv_obj_set_style_border_width(sidepod_r, 1, 0);
    lv_obj_set_style_border_color(sidepod_r, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(sidepod_r, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(sidepod_r, 3, 0);

    lv_obj_t * cockpit = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(cockpit);
    lv_obj_set_pos(cockpit, 44, 92);
    lv_obj_set_size(cockpit, 22, 20);
    lv_obj_set_style_border_width(cockpit, 1, 0);
    lv_obj_set_style_border_color(cockpit, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(cockpit, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(cockpit, 4, 0);

    lv_obj_t * engine_cover = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(engine_cover);
    lv_obj_set_pos(engine_cover, 44, 122);
    lv_obj_set_size(engine_cover, 22, 28);
    lv_obj_set_style_border_width(engine_cover, 1, 0);
    lv_obj_set_style_border_color(engine_cover, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(engine_cover, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(engine_cover, 3, 0);

    lv_obj_t * rear_body = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(rear_body);
    lv_obj_set_pos(rear_body, 36, 150);
    lv_obj_set_size(rear_body, 38, 12);
    lv_obj_set_style_border_width(rear_body, 1, 0);
    lv_obj_set_style_border_color(rear_body, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(rear_body, LV_OPA_TRANSP, 0);

    lv_obj_t * rear_wing = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(rear_wing);
    lv_obj_set_pos(rear_wing, 26, 162);
    lv_obj_set_size(rear_wing, 58, 5);
    lv_obj_set_style_border_width(rear_wing, 1, 0);
    lv_obj_set_style_border_color(rear_wing, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(rear_wing, LV_OPA_TRANSP, 0);

    lv_obj_t * rear_arm_l = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(rear_arm_l);
    lv_obj_set_pos(rear_arm_l, 31, 144);
    lv_obj_set_size(rear_arm_l, 14, 1);
    lv_obj_set_style_bg_color(rear_arm_l, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(rear_arm_l, LV_OPA_COVER, 0);

    lv_obj_t * rear_arm_r = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(rear_arm_r);
    lv_obj_set_pos(rear_arm_r, 65, 144);
    lv_obj_set_size(rear_arm_r, 14, 1);
    lv_obj_set_style_bg_color(rear_arm_r, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(rear_arm_r, LV_OPA_COVER, 0);

    g_dashboard.wheel_fl = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(g_dashboard.wheel_fl);
    lv_obj_set_pos(g_dashboard.wheel_fl, 16, 76);
    lv_obj_set_size(g_dashboard.wheel_fl, 12, 26);
    lv_obj_set_style_radius(g_dashboard.wheel_fl, 3, 0);
    lv_obj_set_style_bg_opa(g_dashboard.wheel_fl, LV_OPA_COVER, 0);

    g_dashboard.wheel_fr = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(g_dashboard.wheel_fr);
    lv_obj_set_pos(g_dashboard.wheel_fr, 82, 76);
    lv_obj_set_size(g_dashboard.wheel_fr, 12, 26);
    lv_obj_set_style_radius(g_dashboard.wheel_fr, 3, 0);
    lv_obj_set_style_bg_opa(g_dashboard.wheel_fr, LV_OPA_COVER, 0);

    g_dashboard.wheel_rl = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(g_dashboard.wheel_rl);
    lv_obj_set_pos(g_dashboard.wheel_rl, 16, 140);
    lv_obj_set_size(g_dashboard.wheel_rl, 12, 26);
    lv_obj_set_style_radius(g_dashboard.wheel_rl, 3, 0);
    lv_obj_set_style_bg_opa(g_dashboard.wheel_rl, LV_OPA_COVER, 0);

    g_dashboard.wheel_rr = lv_obj_create(vehicle_box);
    lv_obj_remove_style_all(g_dashboard.wheel_rr);
    lv_obj_set_pos(g_dashboard.wheel_rr, 82, 140);
    lv_obj_set_size(g_dashboard.wheel_rr, 12, 26);
    lv_obj_set_style_radius(g_dashboard.wheel_rr, 3, 0);
    lv_obj_set_style_bg_opa(g_dashboard.wheel_rr, LV_OPA_COVER, 0);

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
}

static void update_main_dashboard_demo(void)
{
    static int32_t speed = 0;
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

    lv_snprintf(speed_buf, sizeof(speed_buf), "%ld", (long)speed);
    lv_label_set_text(g_dashboard.speed_value, speed_buf);
    lv_obj_set_width(g_dashboard.speed_bar_mask, speed == 0 ? 1 : (speed * 480 / 100));

    g_soc_value += soc_delta;
    if(g_soc_value >= 100) {
        g_soc_value = 100;
        soc_delta = -1;
    }
    else if(g_soc_value <= 0) {
        g_soc_value = 0;
        soc_delta = 1;
    }

    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)g_soc_value);
    lv_label_set_text(g_dashboard.soc_value, soc_buf);
    lv_obj_set_width(g_dashboard.battery_fill, g_soc_value == 0 ? 1 : (g_soc_value * 38 / 100));

    g_total_voltage = 72 + speed / 10;
    g_total_current = 15 + speed / 8;
    g_max_temp = 46 + speed / 20;
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)g_total_voltage);
    lv_label_set_text(g_dashboard.total_voltage_value, soc_buf);
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)g_total_current);
    lv_label_set_text(g_dashboard.total_current_value, soc_buf);
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)g_max_temp);
    lv_label_set_text(g_dashboard.max_temp_value, soc_buf);

    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)(120 + speed / 2));
    lv_label_set_text(g_dashboard.motor_fl_torque, soc_buf);
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)(800 + speed * 8));
    lv_label_set_text(g_dashboard.motor_fl_speed, soc_buf);
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)(118 + speed / 2));
    lv_label_set_text(g_dashboard.motor_fr_torque, soc_buf);
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)(790 + speed * 8));
    lv_label_set_text(g_dashboard.motor_fr_speed, soc_buf);
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)(116 + speed / 2));
    lv_label_set_text(g_dashboard.motor_rl_torque, soc_buf);
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)(780 + speed * 8));
    lv_label_set_text(g_dashboard.motor_rl_speed, soc_buf);
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)(114 + speed / 2));
    lv_label_set_text(g_dashboard.motor_rr_torque, soc_buf);
    lv_snprintf(soc_buf, sizeof(soc_buf), "%ld", (long)(770 + speed * 8));
    lv_label_set_text(g_dashboard.motor_rr_speed, soc_buf);

    g_wheel_fl_alarm = speed > 120;
    g_wheel_fr_alarm = speed > 80;
    g_wheel_rl_alarm = speed > 140;
    g_wheel_rr_alarm = speed > 100;
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
