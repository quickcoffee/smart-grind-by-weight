#pragma once
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include "touch_driver.h"
#include "../config/constants.h"

class DisplayManager {
private:
    Arduino_DataBus* bus;
    Arduino_GFX* gfx_device;
    lv_display_t* lvgl_display;
    lv_indev_t* lvgl_input;
    lv_color_t* draw_buffer;
    uint16_t* dma_staging_buffer;
    TouchDriver touch_driver;
    uint16_t dma_staging_rows;
    
    uint32_t screen_width;
    uint32_t screen_height;
    uint32_t buffer_size;
    bool initialized;
    bool panel_flush_enabled;
    bool showing_external_paint;

public:
    void init();
    void update();
    void set_brightness(float brightness);
    bool draw_rgb565_file(const char* path, uint16_t width, uint16_t height);

    // Suspends pushing LVGL's rendered output to the panel. LVGL keeps running
    // (timers, input, layout) but its pixels are discarded, which lets callers
    // paint the panel directly via draw_rgb565_file() without LVGL overwriting
    // them on the next refresh. Re-enabling does not repaint by itself - the
    // caller must invalidate the screen it wants restored.
    void set_panel_flush_enabled(bool enabled) { panel_flush_enabled = enabled; }
    bool is_panel_flush_enabled() const { return panel_flush_enabled; }

    // True when draw_rgb565_file() last painted the panel successfully and LVGL
    // has not flushed over it since. Lets the startup splash hand off to the
    // screensaver controller without re-reading the image from flash.
    bool is_showing_external_paint() const { return showing_external_paint; }

    uint32_t get_width() const { return screen_width; }
    uint32_t get_height() const { return screen_height; }
    bool is_initialized() const { return initialized; }
    TouchDriver* get_touch_driver() { return &touch_driver; }
    
private:
    static void display_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);
    static void display_rounder_cb(lv_event_t* e);
    static void touchpad_read_cb(lv_indev_t* indev, lv_indev_data_t* data);
    static uint32_t millis_cb();
};

extern DisplayManager* g_display_manager;
