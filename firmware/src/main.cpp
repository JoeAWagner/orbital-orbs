#include "Button.h"
#include "GlobalTime.h"
#include "ScreenManager.h"
#include "Settings.h"
#include "Utils.h"
#include "WebPortal.h"
#include "WidgetSet.h"
#include "version.h"
#include <ArduinoOTA.h>
#include "clockwidget/ClockWidget.h"
#include "config_helper.h"
#include "customwidget/CustomWidget.h"
#include "icons.h"
#include "weatherwidget/WeatherWidget.h"
#include "webdatawidget/WebDataWidget.h"
#include "wifiwidget/WifiWidget.h"
#include <Arduino.h>

#ifdef STOCK_TICKER_LIST
    #include "stockwidget/StockWidget.h"
#endif
#ifdef PARQET_PORTFOLIO_ID
    #include "parqetwidget/ParqetWidget.h"
#endif
#ifdef MQTT_WIDGET_HOST
    #include "mqttwidget/MQTTWidget.h"
#endif

TFT_eSPI tft = TFT_eSPI();

#ifdef WIDGET_CYCLE_DELAY
unsigned long m_widgetCycleDelay = WIDGET_CYCLE_DELAY * 1000; // Automatically cycle widgets every X seconds, set to 0 to disable
#else
unsigned long m_widgetCycleDelay = 0;
#endif
unsigned long m_widgetCycleDelayPrev = 0;

Button buttonLeft(BUTTON_LEFT);
Button buttonOK(BUTTON_OK);
Button buttonRight(BUTTON_RIGHT);

GlobalTime *globalTime; // Initialize the global time

String connectingString{""};

WifiWidget *wifiWidget{nullptr};

int connectionTimer{0};
const int connectionTimeout{10000};
bool isConnected{true};

ScreenManager *sm;
WidgetSet *widgetSet;
WebPortal *webPortal{nullptr};

// Set to true by the web panel when a setting that affects fetched data
// (weather/stocks/timezone) changes, so loop() can trigger a live refresh.
volatile bool g_forceWidgetRefresh = false;
// Set to true for a lightweight one-shot redraw of the current widget (e.g.
// after a brightness change) - no data re-fetch.
volatile bool g_redrawRequested = false;

// Re-apply persisted settings to the running device (called at boot and
// whenever a setting is changed from the web control panel).
void applyRuntimeSettings() {
    if (sm != nullptr) {
        sm->setBrightness(settings.getBrightness());
    }
    if (globalTime != nullptr) {
        globalTime->setFormat24Hour(settings.getFormat24Hour());
    }
    m_widgetCycleDelay = (unsigned long) settings.getCycleDelaySeconds() * 1000UL;
}

// Heavy path: a data setting (weather/stocks/timezone) changed. Apply cheap
// settings, force a timezone re-fetch, and flag a full widget-data refresh.
void onWebSettingsChanged() {
    applyRuntimeSettings();
    if (globalTime != nullptr) {
        globalTime->forceTimezoneUpdate();
    }
    g_forceWidgetRefresh = true;
}

// Light path: a live tweak (brightness/cycle) changed. Apply it and ask for a
// single redraw - no network re-fetch.
void onLiveSettingChanged() {
    applyRuntimeSettings();
    g_redrawRequested = true;
}

// This function should probably be moved somewhere else
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
    if (y >= tft.height())
        return 0;
    // Dim bitmap
    for (int i = 0; i < w * h; i++) {
        bitmap[i] = Utils::rgb565dim(bitmap[i], sm->getBrightness(), true);
    }
    tft.pushImage(x, y, w, h, bitmap);
    return 1;
}

/**
 * The ISR handlers must be static
 */
void isrButtonChangeLeft() { buttonLeft.isrButtonChange(); }
void isrButtonChangeMiddle() { buttonOK.isrButtonChange(); }
void isrButtonChangeRight() { buttonRight.isrButtonChange(); }

void setupButtons() {
    buttonLeft.begin();
    buttonOK.begin();
    buttonRight.begin();

    attachInterrupt(digitalPinToInterrupt(BUTTON_LEFT), isrButtonChangeLeft, CHANGE);
    attachInterrupt(digitalPinToInterrupt(BUTTON_OK), isrButtonChangeMiddle, CHANGE);
    attachInterrupt(digitalPinToInterrupt(BUTTON_RIGHT), isrButtonChangeRight, CHANGE);
}

// Draws a friendly robot face (🤖) on the currently active screen using
// graphics primitives - replaces the old embedded logo photo on the boot screen.
void drawBootRobot(ScreenManager *sm) {
    const uint32_t HEAD = TFT_LIGHTGREY;
    const uint32_t DARK = TFT_DARKGREY;
    const uint32_t EYE = TFT_CYAN;

    // Antenna
    sm->fillRect(117, 34, 6, 20, DARK);
    sm->fillCircle(120, 31, 7, TFT_RED);

    // Ears / side bolts (drawn first so the head overlaps their inner edge)
    sm->fillRect(50, 96, 12, 32, DARK);
    sm->fillRect(178, 96, 12, 32, DARK);

    // Head - a rounded square built from two rects plus four corner circles
    sm->fillRect(76, 52, 88, 120, HEAD);
    sm->fillRect(60, 68, 120, 88, HEAD);
    sm->fillCircle(76, 68, 16, HEAD);
    sm->fillCircle(164, 68, 16, HEAD);
    sm->fillCircle(76, 156, 16, HEAD);
    sm->fillCircle(164, 156, 16, HEAD);

    // Eyes (glowing cyan with pupil + highlight)
    sm->fillCircle(97, 108, 15, EYE);
    sm->fillCircle(143, 108, 15, EYE);
    sm->fillCircle(97, 108, 6, TFT_BLACK);
    sm->fillCircle(143, 108, 6, TFT_BLACK);
    sm->fillCircle(93, 104, 3, TFT_WHITE);
    sm->fillCircle(139, 104, 3, TFT_WHITE);

    // Mouth grille
    sm->fillRect(90, 140, 60, 16, DARK);
    sm->drawLine(105, 140, 105, 156, TFT_BLACK);
    sm->drawLine(120, 140, 120, 156, TFT_BLACK);
    sm->drawLine(135, 140, 135, 156, TFT_BLACK);
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("Starting up...");

    settings.begin();

    TJpgDec.setSwapBytes(true); // JPEG rendering setup
    TJpgDec.setCallback(tft_output);
    setupButtons();

    sm = new ScreenManager(tft);
    sm->fillAllScreens(TFT_BLACK);
    sm->setFontColor(TFT_WHITE);

    sm->selectScreen(0);
    sm->drawCentreString("Welcome", ScreenCenterX, ScreenCenterY, 29);

    sm->selectScreen(1);
    sm->drawCentreString("Info Orbs", ScreenCenterX, ScreenCenterY - 62, 22);
    sm->drawCentreString("by brett.tech", ScreenCenterX, ScreenCenterY - 25, 16);
    sm->setFontColor(TFT_SKYBLUE);
    sm->drawCentreString("+ web panel", ScreenCenterX, ScreenCenterY + 8, 18);
    sm->setFontColor(TFT_RED);
    sm->drawCentreString("version: " FW_VERSION, ScreenCenterX, ScreenCenterY + 45, 14);

    sm->selectScreen(2);
    drawBootRobot(sm);

    widgetSet = new WidgetSet(sm);

#ifdef GC9A01_DRIVER
    Serial.println("GC9A01 Driver");
#endif
#if HARDWARE == WOKWI
    Serial.println("Wokwi Build");
#endif

    pinMode(BUSY_PIN, OUTPUT);
    Serial.println("Connecting to WiFi");

    wifiWidget = new WifiWidget(*sm);
    wifiWidget->setup();

    globalTime = GlobalTime::getInstance();

    widgetSet->add(new ClockWidget(*sm));
#ifdef PARQET_PORTFOLIO_ID
    widgetSet->add(new ParqetWidget(*sm));
#endif
#ifdef STOCK_TICKER_LIST
    widgetSet->add(new StockWidget(*sm));
#endif
    widgetSet->add(new WeatherWidget(*sm));
    widgetSet->add(new CustomWidget(*sm));
#ifdef WEB_DATA_WIDGET_URL
    widgetSet->add(new WebDataWidget(*sm, WEB_DATA_WIDGET_URL));
#endif
#ifdef WEB_DATA_STOCK_WIDGET_URL
    widgetSet->add(new WebDataWidget(*sm, WEB_DATA_STOCK_WIDGET_URL));
#endif
#ifdef MQTT_WIDGET_HOST
    widgetSet->add(new MQTTWidget(*sm, MQTT_WIDGET_HOST, MQTT_WIDGET_PORT));
#endif

    applyRuntimeSettings();

    m_widgetCycleDelayPrev = millis();
}

void checkCycleWidgets() {
    if (m_widgetCycleDelay > 0 && (m_widgetCycleDelayPrev == 0 || (millis() - m_widgetCycleDelayPrev) >= m_widgetCycleDelay)) {
        widgetSet->next();
        m_widgetCycleDelayPrev = millis();
    }
}

void checkButtons() {
    // Reset cycle timer whenever a button is pressed
    if (buttonLeft.pressedShort()) {
        // Left short press cycles widgets backward
        Serial.println("Left button short pressed -> switch to prev Widget");
        m_widgetCycleDelayPrev = millis();
        widgetSet->prev();
    } else if (buttonRight.pressedShort()) {
        // Right short press cycles widgets forward
        Serial.println("Right button short pressed -> switch to next Widget");
        m_widgetCycleDelayPrev = millis();
        widgetSet->next();
    } else {
        ButtonState leftState = buttonLeft.getState();
        ButtonState middleState = buttonOK.getState();
        ButtonState rightState = buttonRight.getState();

        // Everying else that is not BTN_NOTHING will be forwarded to the current widget
        if (leftState != BTN_NOTHING) {
            Serial.printf("Left button pressed, state=%d\n", leftState);
            m_widgetCycleDelayPrev = millis();
            widgetSet->buttonPressed(BUTTON_LEFT, leftState);
        } else if (middleState != BTN_NOTHING) {
            Serial.printf("Middle button pressed, state=%d\n", middleState);
            m_widgetCycleDelayPrev = millis();
            widgetSet->buttonPressed(BUTTON_OK, middleState);
        } else if (rightState != BTN_NOTHING) {
            Serial.printf("Right button pressed, state=%d\n", rightState);
            m_widgetCycleDelayPrev = millis();
            widgetSet->buttonPressed(BUTTON_RIGHT, rightState);
        }
    }
}

void loop() {
    if (wifiWidget->isConnected() == false) {
        wifiWidget->update();
        wifiWidget->draw();
        widgetSet->setClearScreensOnDrawCurrent(); // Clear screen after wifiWidget
        delay(100);
    } else {
        // WiFi is up: start the web control panel once, then service it every loop
        if (webPortal == nullptr) {
            webPortal = new WebPortal(sm, widgetSet);
            webPortal->onSettingsChanged(onWebSettingsChanged);
            webPortal->onLiveApply(onLiveSettingChanged);
            webPortal->begin();
            // Also enable PlatformIO/espota wireless flashing (pio run -t upload
            // --upload-port <ip>). The browser /update endpoint works independently.
            ArduinoOTA.setHostname("info-orbs");
            ArduinoOTA.begin();
            Serial.println("ArduinoOTA ready");
        }
        webPortal->handle();
        ArduinoOTA.handle();

        // A setting affecting fetched data changed via the web panel -> refresh now
        if (g_forceWidgetRefresh) {
            g_forceWidgetRefresh = false;
            widgetSet->forceUpdateAll();
            widgetSet->setClearScreensOnDrawCurrent();
        }

        // A live tweak (e.g. brightness) asked for a single lightweight redraw
        if (g_redrawRequested) {
            g_redrawRequested = false;
            widgetSet->drawCurrent(true);
        }

        if (!widgetSet->initialUpdateDone()) {
            widgetSet->initializeAllWidgetsData();
        }
        globalTime->updateTime();

        checkButtons();

        widgetSet->updateCurrent();
        widgetSet->updateBrightnessByTime(globalTime->getHour24());
        widgetSet->drawCurrent();

        checkCycleWidgets();
    }
}
