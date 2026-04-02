#include "dashboard_ui.h"

#include "main.h"

#include "../lvgl/lvgl.h"

typedef struct {
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
} dashboard_ui_t;

typedef enum {
    DRIVE_MODE_S = 0,
    DRIVE_MODE_E,
    DRIVE_MODE_C
} drive_mode_t;

static dashboard_ui_t g_dashboard;
static drive_mode_t g_drive_mode = DRIVE_MODE_S;
static int32_t g_speed = 0;
static int32_t g_soc = 72;
static int32_t g_mode_index = 0;
static int32_t g_torque[4] = {120, 118, 116, 114};
static int32_t g_rpm[4] = {800, 790, 780, 770};
static int32_t g_sum_voltage = 72;
static int32_t g_top_temperature = 46;
static int32_t g_sum_current = 15;
static int32_t g_tire_temp_fl = 35;
static int32_t g_tire_temp_fr = 48;
static int32_t g_tire_temp_rl = 58;
static int32_t g_tire_temp_rr = 66;
static bool g_motor_fl_online = false;
static bool g_motor_fr_online = true;
static bool g_motor_rl_online = false;
static bool g_motor_rr_online = true;
static dashboard_data_t g_dashboard_data = {
    .speed = 0,
    .soc = 72,
    .mode_index = 0,
    .torque = {120, 118, 116, 114},
    .rpm = {800, 790, 780, 770},
    .sum_voltage = 72,
    .sum_current = 15,
    .max_temperature = 46,
};
static volatile dashboard_data_t g_pending_dashboard_data;
static volatile uint8_t g_dashboard_data_dirty = 0U;

#define DASHBOARD_FONT_SMALL (&lv_font_montserrat_14)
#define DASHBOARD_FONT_LARGE (&lv_font_montserrat_28)

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
    lv_obj_set_style_border_color(panel, lv_color_hex(0xFFFFFF), 0);
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

static void apply_drive_mode_ui(void)
{
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
    switch(g_mode_index) {
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

static void dashboard_apply_data(void)
{
    static char text_buf[12];

    g_speed = g_dashboard_data.speed;
    g_soc = g_dashboard_data.soc;
    g_mode_index = g_dashboard_data.mode_index;
    g_sum_voltage = g_dashboard_data.sum_voltage;
    g_sum_current = g_dashboard_data.sum_current;
    g_top_temperature = g_dashboard_data.max_temperature;
    for(uint32_t index = 0; index < 4U; index++) {
        g_torque[index] = g_dashboard_data.torque[index];
        g_rpm[index] = g_dashboard_data.rpm[index];
    }

    int32_t display_speed = g_speed;
    if(display_speed < 0) display_speed = 0;
    if(display_speed > 99) display_speed = 99;

    set_speed_digits(display_speed);
    lv_obj_set_width(g_dashboard.speed_bar_mask, display_speed == 0 ? 1 : (display_speed * 480 / 100));

    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_soc);
    lv_label_set_text(g_dashboard.soc_value, text_buf);
    lv_obj_set_width(g_dashboard.battery_fill, g_soc == 0 ? 1 : (g_soc * 38 / 100));

    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_sum_voltage);
    lv_label_set_text(g_dashboard.total_voltage_value, text_buf);
    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_sum_current);
    lv_label_set_text(g_dashboard.total_current_value, text_buf);
    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_top_temperature);
    lv_label_set_text(g_dashboard.max_temp_value, text_buf);

    sync_mode_from_index();
    apply_drive_mode_ui();

    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_torque[0]);
    lv_label_set_text(g_dashboard.motor_fl_torque, text_buf);
    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_rpm[0]);
    lv_label_set_text(g_dashboard.motor_fl_speed, text_buf);
    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_torque[1]);
    lv_label_set_text(g_dashboard.motor_fr_torque, text_buf);
    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_rpm[1]);
    lv_label_set_text(g_dashboard.motor_fr_speed, text_buf);
    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_torque[2]);
    lv_label_set_text(g_dashboard.motor_rl_torque, text_buf);
    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_rpm[2]);
    lv_label_set_text(g_dashboard.motor_rl_speed, text_buf);
    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_torque[3]);
    lv_label_set_text(g_dashboard.motor_rr_torque, text_buf);
    lv_snprintf(text_buf, sizeof(text_buf), "%ld", (long)g_rpm[3]);
    lv_label_set_text(g_dashboard.motor_rr_speed, text_buf);

    g_tire_temp_fl = 30 + g_speed / 2;
    g_tire_temp_fr = 36 + g_speed / 2;
    g_tire_temp_rl = 42 + g_speed / 2;
    g_tire_temp_rr = 48 + g_speed / 2;

    g_motor_fl_online = ((g_speed / 10) % 2) != 0;
    g_motor_fr_online = ((g_speed / 12) % 2) != 0;
    g_motor_rl_online = ((g_speed / 14) % 2) != 0;
    g_motor_rr_online = ((g_speed / 16) % 2) != 0;
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
        g_pending_dashboard_data.rpm[index] = data->rpm[index];
    }

    g_dashboard_data_dirty = 1U;
}

void Dashboard_UI_Process(void)
{
    uint32_t index;
    uint32_t primask;

    if(g_dashboard_data_dirty == 0U) {
        return;
    }

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
        g_dashboard_data.rpm[index] = g_pending_dashboard_data.rpm[index];
    }
    g_dashboard_data_dirty = 0U;
    if(primask == 0U) {
        __enable_irq();
    }

    dashboard_apply_data();
}

void Dashboard_UI_Init(void)
{
    lv_obj_t * screen = lv_obj_create(NULL);
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

    lv_obj_t * battery_outline = create_panel(mode_box, 34, 18, 42, 18, lv_color_hex(0x000000), LV_OPA_COVER);
    lv_obj_set_style_border_width(battery_outline, 1, 0);

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
    lv_obj_set_style_text_font(soc_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_align(soc_label, LV_ALIGN_TOP_MID, 0, 48);

    g_dashboard.soc_value = lv_label_create(mode_box);
    lv_label_set_text(g_dashboard.soc_value, "72");
    lv_obj_set_style_text_color(g_dashboard.soc_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.soc_value, DASHBOARD_FONT_SMALL, 0);
    lv_obj_align(g_dashboard.soc_value, LV_ALIGN_TOP_MID, 0, 68);

    lv_obj_t * voltage_label = lv_label_create(mode_box);
    lv_label_set_text(voltage_label, "TOTAL V:");
    lv_obj_set_style_text_color(voltage_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(voltage_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(voltage_label, 10, 118);

    g_dashboard.total_voltage_value = lv_label_create(mode_box);
    lv_label_set_text(g_dashboard.total_voltage_value, "72");
    lv_obj_set_style_text_color(g_dashboard.total_voltage_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.total_voltage_value, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.total_voltage_value, 72, 118);

    lv_obj_t * current_label = lv_label_create(mode_box);
    lv_label_set_text(current_label, "TOTAL A:");
    lv_obj_set_style_text_color(current_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(current_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(current_label, 10, 136);

    g_dashboard.total_current_value = lv_label_create(mode_box);
    lv_label_set_text(g_dashboard.total_current_value, "15");
    lv_obj_set_style_text_color(g_dashboard.total_current_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.total_current_value, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.total_current_value, 72, 136);

    lv_obj_t * max_temp_label = lv_label_create(mode_box);
    lv_label_set_text(max_temp_label, "MAX T:");
    lv_obj_set_style_text_color(max_temp_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(max_temp_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(max_temp_label, 10, 154);

    g_dashboard.max_temp_value = lv_label_create(mode_box);
    lv_label_set_text(g_dashboard.max_temp_value, "46");
    lv_obj_set_style_text_color(g_dashboard.max_temp_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.max_temp_value, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.max_temp_value, 60, 154);

    lv_obj_t * speed_box = create_panel(middle_panel, 110, 26, 260, 124, lv_color_hex(0x000000), LV_OPA_COVER);
    lv_obj_set_style_border_width(speed_box, 0, 0);

    create_speed_digits(speed_box);

    lv_obj_t * speed_unit = lv_label_create(speed_box);
    lv_label_set_text(speed_unit, "km/h");
    lv_obj_set_style_text_color(speed_unit, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(speed_unit, LV_OPA_60, 0);
    lv_obj_set_style_text_font(speed_unit, &lv_font_montserrat_14, 0);
    lv_obj_align(speed_unit, LV_ALIGN_CENTER, -8, 44);

    lv_obj_t * vehicle_box = create_panel(middle_panel, 0, 0, 110, 176, lv_color_hex(0x000000), LV_OPA_COVER);
    lv_obj_set_style_border_width(vehicle_box, 0, 0);

    g_dashboard.mode_value = lv_label_create(vehicle_box);
    lv_obj_set_style_text_font(g_dashboard.mode_value, DASHBOARD_FONT_LARGE, 0);
    lv_obj_align(g_dashboard.mode_value, LV_ALIGN_TOP_MID, 0, 10);
    apply_drive_mode_ui();

    g_dashboard.wheel_fl = create_panel(vehicle_box, 18, 84, 16, 30, lv_color_hex(0x000000), LV_OPA_TRANSP);
    lv_obj_set_style_radius(g_dashboard.wheel_fl, 3, 0);
    g_dashboard.lightning_fl = lv_label_create(vehicle_box);
    lv_label_set_text(g_dashboard.lightning_fl, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(g_dashboard.lightning_fl, lv_color_hex(0xFFD400), 0);
    lv_obj_set_pos(g_dashboard.lightning_fl, 38, 92);

    g_dashboard.wheel_fr = create_panel(vehicle_box, 80, 84, 16, 30, lv_color_hex(0x000000), LV_OPA_TRANSP);
    lv_obj_set_style_radius(g_dashboard.wheel_fr, 3, 0);
    g_dashboard.lightning_fr = lv_label_create(vehicle_box);
    lv_label_set_text(g_dashboard.lightning_fr, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(g_dashboard.lightning_fr, lv_color_hex(0xFFD400), 0);
    lv_obj_align_to(g_dashboard.lightning_fr, g_dashboard.wheel_fr, LV_ALIGN_OUT_LEFT_MID, 0, 0);

    g_dashboard.wheel_rl = create_panel(vehicle_box, 18, 120, 16, 30, lv_color_hex(0x000000), LV_OPA_TRANSP);
    lv_obj_set_style_radius(g_dashboard.wheel_rl, 3, 0);
    g_dashboard.lightning_rl = lv_label_create(vehicle_box);
    lv_label_set_text(g_dashboard.lightning_rl, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(g_dashboard.lightning_rl, lv_color_hex(0xFFD400), 0);
    lv_obj_set_pos(g_dashboard.lightning_rl, 38, 128);

    g_dashboard.wheel_rr = create_panel(vehicle_box, 80, 120, 16, 30, lv_color_hex(0x000000), LV_OPA_TRANSP);
    lv_obj_set_style_radius(g_dashboard.wheel_rr, 3, 0);
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
    lv_obj_set_style_text_font(motor_fl_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_fl_label, 8, 6);
    g_dashboard.motor_fl_torque = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_fl_torque, "120");
    lv_obj_set_style_text_color(g_dashboard.motor_fl_torque, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.motor_fl_torque, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_fl_torque, 54, 6);
    g_dashboard.motor_fl_speed = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_fl_speed, "850");
    lv_obj_set_style_text_color(g_dashboard.motor_fl_speed, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.motor_fl_speed, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_fl_speed, 54, 24);

    lv_obj_t * motor_fl_speed_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_fl_speed_label, "LF N:");
    lv_obj_set_style_text_color(motor_fl_speed_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(motor_fl_speed_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_fl_speed_label, 8, 24);

    lv_obj_t * motor_fr_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_fr_label, "RF T:");
    lv_obj_set_style_text_color(motor_fr_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(motor_fr_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_fr_label, 126, 6);
    g_dashboard.motor_fr_torque = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_fr_torque, "118");
    lv_obj_set_style_text_color(g_dashboard.motor_fr_torque, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.motor_fr_torque, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_fr_torque, 172, 6);
    g_dashboard.motor_fr_speed = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_fr_speed, "840");
    lv_obj_set_style_text_color(g_dashboard.motor_fr_speed, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.motor_fr_speed, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_fr_speed, 172, 24);

    lv_obj_t * motor_fr_speed_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_fr_speed_label, "RF N:");
    lv_obj_set_style_text_color(motor_fr_speed_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(motor_fr_speed_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_fr_speed_label, 126, 24);

    lv_obj_t * motor_rl_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_rl_label, "LR T:");
    lv_obj_set_style_text_color(motor_rl_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(motor_rl_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_rl_label, 244, 6);
    g_dashboard.motor_rl_torque = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_rl_torque, "116");
    lv_obj_set_style_text_color(g_dashboard.motor_rl_torque, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.motor_rl_torque, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_rl_torque, 290, 6);
    g_dashboard.motor_rl_speed = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_rl_speed, "830");
    lv_obj_set_style_text_color(g_dashboard.motor_rl_speed, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.motor_rl_speed, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_rl_speed, 290, 24);

    lv_obj_t * motor_rl_speed_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_rl_speed_label, "LR N:");
    lv_obj_set_style_text_color(motor_rl_speed_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(motor_rl_speed_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_rl_speed_label, 244, 24);

    lv_obj_t * motor_rr_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_rr_label, "RR T:");
    lv_obj_set_style_text_color(motor_rr_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(motor_rr_label, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(motor_rr_label, 362, 6);
    g_dashboard.motor_rr_torque = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_rr_torque, "114");
    lv_obj_set_style_text_color(g_dashboard.motor_rr_torque, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.motor_rr_torque, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_rr_torque, 408, 6);
    g_dashboard.motor_rr_speed = lv_label_create(bottom_info);
    lv_label_set_text(g_dashboard.motor_rr_speed, "820");
    lv_obj_set_style_text_color(g_dashboard.motor_rr_speed, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_dashboard.motor_rr_speed, DASHBOARD_FONT_SMALL, 0);
    lv_obj_set_pos(g_dashboard.motor_rr_speed, 408, 24);

    lv_obj_t * motor_rr_speed_label = lv_label_create(bottom_info);
    lv_label_set_text(motor_rr_speed_label, "RR N:");
    lv_obj_set_style_text_color(motor_rr_speed_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(motor_rr_speed_label, DASHBOARD_FONT_SMALL, 0);
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

    lv_screen_load(screen);
    dashboard_apply_data();
}