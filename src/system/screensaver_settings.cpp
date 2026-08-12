#include "screensaver_settings.h"

#include <Preferences.h>
#include <algorithm>
#include <atomic>

namespace {

constexpr const char* kPrefsNamespace = "screensaver";
constexpr const char* kIdleTimeoutKey = "idle_timeout_s";
constexpr const char* kStartupTimeoutKey = "startup_s";
constexpr const char* kStartupEnabledKey = "startup";
constexpr const char* kSleepEnabledKey = "sleep";

// These settings are read on every UI tick but change only when the user edits
// them, so they are cached in RAM. Opening an NVS handle per read churned the
// internal heap several times a second for values that are effectively static.
//
// Fields are individually atomic rather than mutex-guarded: writes come from
// the UI and BLE tasks, reads from the UI task, and each field is a naturally
// aligned scalar. A reader racing a writer sees a mix of old and new values for
// at most one 16ms tick, which only shifts a dim timeout by one frame.
std::atomic<uint16_t> g_idle_timeout_s{ScreensaverSettings::kDefaultIdleTimeoutS};
std::atomic<uint8_t> g_startup_timeout_s{ScreensaverSettings::kDefaultStartupTimeoutS};
std::atomic<bool> g_startup_enabled{false};
std::atomic<bool> g_sleep_enabled{false};
std::atomic<bool> g_cache_loaded{false};

void load_cache_from_nvs() {
    uint16_t idle_timeout_s = ScreensaverSettings::kDefaultIdleTimeoutS;
    uint8_t startup_timeout_s = ScreensaverSettings::kDefaultStartupTimeoutS;
    bool startup_enabled = false;
    bool sleep_enabled = false;

    Preferences prefs;
    if (prefs.begin(kPrefsNamespace, true)) {
        idle_timeout_s = prefs.getUShort(kIdleTimeoutKey,
                                         ScreensaverSettings::kDefaultIdleTimeoutS);
        startup_timeout_s = prefs.getUChar(kStartupTimeoutKey,
                                           ScreensaverSettings::kDefaultStartupTimeoutS);
        startup_enabled = prefs.getBool(kStartupEnabledKey, false);
        sleep_enabled = prefs.getBool(kSleepEnabledKey, false);
        prefs.end();
    }

    if (!ScreensaverSettings::is_valid_idle_timeout(idle_timeout_s)) {
        idle_timeout_s = ScreensaverSettings::kDefaultIdleTimeoutS;
    }
    if (!ScreensaverSettings::is_valid_startup_timeout(startup_timeout_s)) {
        startup_timeout_s = ScreensaverSettings::kDefaultStartupTimeoutS;
    }

    g_idle_timeout_s.store(idle_timeout_s);
    g_startup_timeout_s.store(startup_timeout_s);
    g_startup_enabled.store(startup_enabled);
    g_sleep_enabled.store(sleep_enabled);
    g_cache_loaded.store(true);
}

void ensure_cache_loaded() {
    if (!g_cache_loaded.load()) {
        load_cache_from_nvs();
    }
}

bool write_bool_setting(const char* key, bool value) {
    Preferences prefs;
    if (!prefs.begin(kPrefsNamespace, false)) {
        return false;
    }
    bool written = prefs.putBool(key, value) == sizeof(bool);
    prefs.end();
    return written;
}

}  // namespace

namespace ScreensaverSettings {

bool is_valid_idle_timeout(uint16_t idle_timeout_s) {
    return idle_timeout_s >= kMinIdleTimeoutS && idle_timeout_s <= kMaxIdleTimeoutS;
}

bool is_valid_startup_timeout(uint8_t startup_timeout_s) {
    return startup_timeout_s >= kMinStartupTimeoutS && startup_timeout_s <= kMaxStartupTimeoutS;
}

bool is_startup_enabled() {
    ensure_cache_loaded();
    return g_startup_enabled.load();
}

bool is_sleep_enabled() {
    ensure_cache_loaded();
    return g_sleep_enabled.load();
}

bool set_startup_enabled(bool enabled) {
    if (!write_bool_setting(kStartupEnabledKey, enabled)) {
        return false;
    }
    g_startup_enabled.store(enabled);
    return true;
}

bool set_sleep_enabled(bool enabled) {
    if (!write_bool_setting(kSleepEnabledKey, enabled)) {
        return false;
    }
    g_sleep_enabled.store(enabled);
    return true;
}

ScreensaverTimingSettings load_timing() {
    ensure_cache_loaded();
    return ScreensaverTimingSettings{
        g_idle_timeout_s.load(),
        g_startup_timeout_s.load(),
    };
}

bool save_timing(uint16_t idle_timeout_s, uint8_t startup_timeout_s) {
    if (!is_valid_idle_timeout(idle_timeout_s) ||
        !is_valid_startup_timeout(startup_timeout_s)) {
        return false;
    }

    Preferences prefs;
    if (!prefs.begin(kPrefsNamespace, false)) {
        return false;
    }

    size_t idle_written = prefs.putUShort(kIdleTimeoutKey, idle_timeout_s);
    size_t startup_written = prefs.putUChar(kStartupTimeoutKey, startup_timeout_s);
    prefs.end();

    if (idle_written != sizeof(uint16_t) || startup_written != sizeof(uint8_t)) {
        return false;
    }

    g_idle_timeout_s.store(idle_timeout_s);
    g_startup_timeout_s.store(startup_timeout_s);
    return true;
}

uint32_t idle_timeout_ms(const ScreensaverTimingSettings& settings) {
    return static_cast<uint32_t>(settings.idle_timeout_s) * 1000U;
}

uint32_t weight_activity_window_ms(const ScreensaverTimingSettings& settings) {
    return std::min(idle_timeout_ms(settings), kWeightActivityWindowCapMs);
}

}  // namespace ScreensaverSettings
