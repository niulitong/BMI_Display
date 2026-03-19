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
    lv_obj_t * gear_value;
    lv_obj_t * voltage_value;
    lv_obj_t * can_status_value;
    lv_obj_t * mode_value;
    lv_obj_t * alarm_value;
} dashboard_ui_t;

static dashboard_ui_t g_dashboard;

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
    lv_obj_set_style_pad_all(panel, 8, 0);
    return panel;
}

static lv_obj_t * create_caption(lv_obj_t * parent, const char * text)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    return label;
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

    lv_obj_t * top_bar = create_panel(screen, 8, 8, 464, 40, lv_color_hex(0x000000), LV_OPA_COVER);
    lv_obj_t * title = lv_label_create(top_bar);
    lv_label_set_text(title, "Vehicle Main Screen");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t * time_label = lv_label_create(top_bar);
    lv_label_set_text(time_label, "12:30");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(time_label, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_t * status_label = lv_label_create(top_bar);
    lv_label_set_text(status_label, LV_SYMBOL_OK " CAN Online");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(status_label, LV_ALIGN_RIGHT_MID, -92, 0);

    lv_obj_t * left_panel = create_panel(screen, 8, 56, 150, 150, lv_color_hex(0x000000), LV_OPA_COVER);
    lv_obj_t * speed_caption = create_caption(left_panel, "Speed");
    lv_obj_align(speed_caption, LV_ALIGN_TOP_LEFT, 0, 0);
    g_dashboard.speed_value = create_value(left_panel, "68", lv_color_hex(0xFFFFFF), 28);
    lv_obj_align(g_dashboard.speed_value, LV_ALIGN_CENTER, 0, -10);
    lv_obj_t * speed_unit = lv_label_create(left_panel);
    lv_label_set_text(speed_unit, "km/h");
    lv_obj_set_style_text_color(speed_unit, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(speed_unit, LV_ALIGN_CENTER, 0, 32);

    lv_obj_t * center_panel = create_panel(screen, 166, 56, 198, 150, lv_color_hex(0x000000), LV_OPA_COVER);
    lv_obj_t * gear_caption = create_caption(center_panel, "Drive State");
    lv_obj_align(gear_caption, LV_ALIGN_TOP_LEFT, 0, 0);
    g_dashboard.gear_value = create_value(center_panel, "D", lv_color_hex(0xFFFFFF), 28);
    lv_obj_align(g_dashboard.gear_value, LV_ALIGN_LEFT_MID, 6, -10);
    g_dashboard.mode_value = create_value(center_panel, "NORMAL", lv_color_hex(0xFFFFFF), 20);
    lv_obj_align(g_dashboard.mode_value, LV_ALIGN_LEFT_MID, 52, -8);
    g_dashboard.alarm_value = create_value(center_panel, LV_SYMBOL_WARNING " No Active Alarm", lv_color_hex(0xFFFFFF), 20);
    lv_obj_align(g_dashboard.alarm_value, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lv_obj_t * right_panel = create_panel(screen, 372, 56, 100, 150, lv_color_hex(0x000000), LV_OPA_COVER);
    lv_obj_t * voltage_caption = create_caption(right_panel, "Voltage");
    lv_obj_align(voltage_caption, LV_ALIGN_TOP_LEFT, 0, 0);
    g_dashboard.voltage_value = create_value(right_panel, "24.6V", lv_color_hex(0xFFFFFF), 20);
    lv_obj_align(g_dashboard.voltage_value, LV_ALIGN_TOP_LEFT, 0, 34);
    lv_obj_t * temp_caption = create_caption(right_panel, "MCU Temp");
    lv_obj_align(temp_caption, LV_ALIGN_TOP_LEFT, 0, 82);
    lv_obj_t * temp_value = create_value(right_panel, "36C", lv_color_hex(0xFFFFFF), 20);
    lv_obj_align(temp_value, LV_ALIGN_TOP_LEFT, 0, 112);

    lv_obj_t * bottom_bar = create_panel(screen, 8, 214, 464, 50, lv_color_hex(0x000000), LV_OPA_COVER);
    lv_obj_t * nav_1 = lv_label_create(bottom_bar);
    lv_label_set_text(nav_1, LV_SYMBOL_HOME " Main");
    lv_obj_set_style_text_color(nav_1, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(nav_1, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t * nav_2 = lv_label_create(bottom_bar);
    lv_label_set_text(nav_2, LV_SYMBOL_SETTINGS " Settings");
    lv_obj_set_style_text_color(nav_2, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(nav_2, LV_ALIGN_LEFT_MID, 130, 0);

    lv_obj_t * nav_3 = lv_label_create(bottom_bar);
    lv_label_set_text(nav_3, LV_SYMBOL_BELL " Alarm");
    lv_obj_set_style_text_color(nav_3, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(nav_3, LV_ALIGN_LEFT_MID, 270, 0);

    g_dashboard.can_status_value = lv_label_create(bottom_bar);
    lv_label_set_text(g_dashboard.can_status_value, "Bus 1: OK   Bus 2: OK");
    lv_obj_set_style_text_color(g_dashboard.can_status_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(g_dashboard.can_status_value, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_t * separator_h = lv_obj_create(screen);
    lv_obj_remove_style_all(separator_h);
    lv_obj_set_pos(separator_h, 8, 210);
    lv_obj_set_size(separator_h, 464, 1);
    lv_obj_set_style_bg_color(separator_h, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(separator_h, LV_OPA_COVER, 0);

    lv_obj_t * separator_v1 = lv_obj_create(screen);
    lv_obj_remove_style_all(separator_v1);
    lv_obj_set_pos(separator_v1, 162, 56);
    lv_obj_set_size(separator_v1, 1, 150);
    lv_obj_set_style_bg_color(separator_v1, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(separator_v1, LV_OPA_COVER, 0);

    lv_obj_t * separator_v2 = lv_obj_create(screen);
    lv_obj_remove_style_all(separator_v2);
    lv_obj_set_pos(separator_v2, 368, 56);
    lv_obj_set_size(separator_v2, 1, 150);
    lv_obj_set_style_bg_color(separator_v2, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(separator_v2, LV_OPA_COVER, 0);

    lv_scr_load(screen);
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
        lv_timer_handler(); /* Handle LVGL tasks */
        vTaskDelay(pdMS_TO_TICKS(5)); /* Short delay for the RTOS scheduler */
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
