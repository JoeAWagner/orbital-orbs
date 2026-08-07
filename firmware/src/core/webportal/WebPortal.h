#ifndef WEBPORTAL_H
#define WEBPORTAL_H

#include "ScreenManager.h"
#include "WidgetSet.h"
#include <Arduino.h>
#include <WebServer.h>
#include <functional>

// A lightweight on-device web control panel.
// Served on port 80 once WiFi is connected. Lets you view live status,
// control the orbs in real time, and change persisted settings from a
// browser - no reflashing required.
class WebPortal {
public:
    WebPortal(ScreenManager *sm, WidgetSet *widgetSet);

    void begin();
    void handle(); // must be called from loop()
    bool isStarted() const { return m_started; }

    // Heavy path: a data setting changed (weather/stocks/timezone). The main
    // sketch should re-apply settings AND re-fetch widget data.
    void onSettingsChanged(std::function<void()> cb) { m_onChanged = cb; }

    // Light path: a live tweak changed (brightness / cycle delay). The main
    // sketch should re-apply runtime values and request a single redraw - but
    // must NOT re-fetch data (that is what made the UI laggy).
    void onLiveApply(std::function<void()> cb) { m_onLiveApply = cb; }

private:
    WebServer m_server;
    ScreenManager *m_sm;
    WidgetSet *m_widgetSet;
    bool m_started = false;
    std::function<void()> m_onChanged;
    std::function<void()> m_onLiveApply;

    void handleRoot();
    void handleStatus();
    void handleNext();
    void handlePrev();
    void handleBrightness();
    void handleCycle();
    void handleGetSettings();
    void handleSaveSettings();
    void notifyChanged();
};

#endif // WEBPORTAL_H
