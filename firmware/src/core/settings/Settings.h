#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <Preferences.h>

// Runtime-editable settings, persisted in NVS flash.
// The config.h #defines provide the defaults the first time the device boots
// (or whenever a key has never been written). After that, values set from the
// web control panel are stored on the device and survive reboots - no reflashing.
class Settings {
public:
    void begin();

    uint8_t getBrightness();
    void setBrightness(uint8_t brightness);

    // Auto-rotate through widgets every N seconds (0 = disabled)
    int getCycleDelaySeconds();
    void setCycleDelaySeconds(int seconds);

    String getWeatherLocation();
    void setWeatherLocation(const String &location);

    // true = metric units, false = imperial/US
    bool getMetric();
    void setMetric(bool metric);

    String getStockList();
    void setStockList(const String &list);

    String getTimezoneLocation();
    void setTimezoneLocation(const String &location);

    bool getFormat24Hour();
    void setFormat24Hour(bool format24Hour);

private:
    Preferences m_prefs;
};

// Global singleton, defined in Settings.cpp
extern Settings settings;

#endif // SETTINGS_H
