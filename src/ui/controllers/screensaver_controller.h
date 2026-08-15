#pragma once

#include <lvgl.h>
#include <cstdint>

class DisplayManager;

/**
 * ScreensaverController - Displays a custom screensaver image from LittleFS.
 *
 * The image is streamed from flash straight to the panel in small row chunks
 * (see DisplayManager::draw_rgb565_file), so a full-screen 280x456 RGB565
 * image costs a few KB of transient buffer instead of a 250KB resident one.
 *
 * While the screensaver is up, LVGL's panel flush is suspended so it cannot
 * repaint over the image. An empty overlay screen is still loaded on top of
 * the UI so stray touches land on nothing instead of activating widgets
 * hidden behind the screensaver.
 *
 * Used for startup splash and power-save display modes.
 */
class ScreensaverController {
public:
    ScreensaverController();
    ~ScreensaverController();

    void set_display(DisplayManager* display) { display_ = display; }

    /// Check if a screensaver image file exists on LittleFS
    bool has_image() const;

    /// Read preference: show image on startup
    bool is_startup_enabled() const;

    /// Read preference: show image during sleep/dim
    bool is_sleep_enabled() const;

    /// Read preference: startup display duration in milliseconds
    uint32_t get_startup_timeout_ms() const;

    /// Paint the image to the panel and take over the display
    void show();

    /**
     * Take over the display without repainting, for when the image is already
     * on the panel - main.cpp paints the startup splash before the UI task
     * exists, and re-reading 250KB from flash just to show the same pixels
     * would undo that head start.
     */
    void show_over_existing_paint();

    /// Release the display back to LVGL and force a full repaint
    void hide();

    bool is_visible() const { return visible_; }

private:
    bool begin_takeover(bool paint_image);

    DisplayManager* display_;
    lv_obj_t* overlay_screen_;
    lv_obj_t* previous_screen_;
    bool visible_;
};
