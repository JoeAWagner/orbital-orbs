#include "Settings.h"
#include "config_helper.h"

Settings settings;

// Fallback defaults in case these are not defined in config.h
#ifndef WIDGET_CYCLE_DELAY
    #define WIDGET_CYCLE_DELAY 0
#endif
#ifndef WEATHER_LOCATION
    #define WEATHER_LOCATION ""
#endif
#ifndef STOCK_TICKER_LIST
    #define STOCK_TICKER_LIST ""
#endif
#ifndef TFT_BRIGHTNESS
    #define TFT_BRIGHTNESS 255
#endif
#ifndef TIMEZONE_API_LOCATION
    #define TIMEZONE_API_LOCATION "America/New_York"
#endif
#ifndef FORMAT_24_HOUR
    #define FORMAT_24_HOUR false
#endif
#ifdef WEATHER_UNITS_METRIC
    #define DEFAULT_METRIC true
#else
    #define DEFAULT_METRIC false
#endif

// NVS namespace and keys (keys must be <= 15 chars)
static const char *NVS_NAMESPACE = "orbs";

void Settings::begin() {
    // read/write mode
    m_prefs.begin(NVS_NAMESPACE, false);
}

uint8_t Settings::getBrightness() {
    return m_prefs.getUChar("brightness", TFT_BRIGHTNESS);
}

void Settings::setBrightness(uint8_t brightness) {
    m_prefs.putUChar("brightness", brightness);
}

int Settings::getCycleDelaySeconds() {
    return m_prefs.getInt("cycleDelay", WIDGET_CYCLE_DELAY);
}

void Settings::setCycleDelaySeconds(int seconds) {
    if (seconds < 0) {
        seconds = 0;
    }
    m_prefs.putInt("cycleDelay", seconds);
}

String Settings::getWeatherLocation() {
    return m_prefs.getString("weatherLoc", WEATHER_LOCATION);
}

void Settings::setWeatherLocation(const String &location) {
    m_prefs.putString("weatherLoc", location);
}

bool Settings::getMetric() {
    return m_prefs.getBool("metric", DEFAULT_METRIC);
}

void Settings::setMetric(bool metric) {
    m_prefs.putBool("metric", metric);
}

String Settings::getStockList() {
    return m_prefs.getString("stocks", STOCK_TICKER_LIST);
}

void Settings::setStockList(const String &list) {
    m_prefs.putString("stocks", list);
}

String Settings::getTimezoneLocation() {
    return m_prefs.getString("tz", TIMEZONE_API_LOCATION);
}

void Settings::setTimezoneLocation(const String &location) {
    m_prefs.putString("tz", location);
}

bool Settings::getFormat24Hour() {
    return m_prefs.getBool("fmt24", FORMAT_24_HOUR);
}

void Settings::setFormat24Hour(bool format24Hour) {
    m_prefs.putBool("fmt24", format24Hour);
}

String Settings::getCustomTitle() {
    return m_prefs.getString("custTitle", "Custom");
}

void Settings::setCustomTitle(const String &v) {
    m_prefs.putString("custTitle", v);
}

String Settings::getCustomText() {
    return m_prefs.getString("custText", "");
}

void Settings::setCustomText(const String &v) {
    m_prefs.putString("custText", v);
}

String Settings::getCustomUrl() {
    return m_prefs.getString("custUrl", "");
}

void Settings::setCustomUrl(const String &v) {
    m_prefs.putString("custUrl", v);
}

String Settings::getCustomField() {
    return m_prefs.getString("custField", "");
}

void Settings::setCustomField(const String &v) {
    m_prefs.putString("custField", v);
}
