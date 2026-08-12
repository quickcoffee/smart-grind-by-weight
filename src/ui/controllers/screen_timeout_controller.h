#pragma once

#include <cstdint>
#include "../../system/screensaver_settings.h"

class UIManager;
class ScreensaverController;
class DisplayManager;

// Implements automatic screen dimming based on touch/weight activity

class ScreenTimeoutController {
public:
    explicit ScreenTimeoutController(UIManager* manager);

    void register_events();
    void update();

    void set_screensaver_controller(ScreensaverController* controller) {
        screensaver_controller_ = controller;
    }

private:
    void restore_normal_display(DisplayManager* display);
    bool is_protected_state() const;

    UIManager* ui_manager_;
    ScreensaverController* screensaver_controller_ = nullptr;
    bool screen_dimmed_;
};
