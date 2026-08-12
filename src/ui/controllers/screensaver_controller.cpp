#include "screensaver_controller.h"
#include "../../config/constants.h"
#include "../../config/logging.h"
#include "../../hardware/display_manager.h"
#include "../../system/screensaver_settings.h"
#include <LittleFS.h>

ScreensaverController::ScreensaverController()
    : display_(nullptr)
    , overlay_screen_(nullptr)
    , previous_screen_(nullptr)
    , visible_(false) {
}

ScreensaverController::~ScreensaverController() {
    hide();
}

bool ScreensaverController::has_image() const {
    return LittleFS.exists(BLE_IMAGE_FILENAME);
}

bool ScreensaverController::is_startup_enabled() const {
    return ScreensaverSettings::is_startup_enabled();
}

bool ScreensaverController::is_sleep_enabled() const {
    return ScreensaverSettings::is_sleep_enabled();
}

uint32_t ScreensaverController::get_startup_timeout_ms() const {
    auto settings = ScreensaverSettings::load_timing();
    return static_cast<uint32_t>(settings.startup_timeout_s) * 1000U;
}

void ScreensaverController::show() {
    begin_takeover(true);
}

void ScreensaverController::show_over_existing_paint() {
    begin_takeover(false);
}

bool ScreensaverController::begin_takeover(bool paint_image) {
    if (visible_) return true;
    if (!display_ || !display_->is_initialized()) return false;
    if (!has_image()) return false;

    // Save the current active screen so we can restore it on hide().
    // Screens in this project are created once and never deleted, so
    // this pointer remains valid for the lifetime of the application.
    lv_obj_t* restore_target = lv_scr_act();

    // Empty screen that only exists to swallow touches aimed at the widgets
    // now hidden behind the screensaver.
    lv_obj_t* overlay = lv_obj_create(nullptr);
    if (!overlay) {
        LOG_BLE("Screensaver: Failed to create overlay screen\n");
        return false;
    }
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, 0);

    // Suspend the flush before loading the overlay so LVGL never gets a chance
    // to paint it black over the image.
    display_->set_panel_flush_enabled(false);
    lv_screen_load(overlay);

    if (paint_image &&
        !display_->draw_rgb565_file(BLE_IMAGE_FILENAME,
                                    HW_DISPLAY_WIDTH_PX,
                                    HW_DISPLAY_HEIGHT_PX)) {
        // Nothing was painted, so handing the panel back is the only sane
        // outcome - otherwise the display freezes on whatever was there.
        LOG_BLE("Screensaver: Image draw failed, releasing display\n");
        lv_screen_load(restore_target);
        lv_obj_delete(overlay);
        display_->set_panel_flush_enabled(true);
        lv_obj_invalidate(restore_target);
        return false;
    }

    previous_screen_ = restore_target;
    overlay_screen_ = overlay;
    visible_ = true;
    return true;
}

void ScreensaverController::hide() {
    if (!visible_) return;

    if (previous_screen_) {
        lv_screen_load(previous_screen_);
    }

    if (overlay_screen_) {
        lv_obj_delete(overlay_screen_);
        overlay_screen_ = nullptr;
    }

    if (display_) {
        display_->set_panel_flush_enabled(true);
    }

    // LVGL believes the panel still matches its last render, so nothing would
    // repaint over the screensaver without an explicit full invalidate.
    if (previous_screen_) {
        lv_obj_invalidate(previous_screen_);
        previous_screen_ = nullptr;
    }

    visible_ = false;
}
