#include "CustomWidget.h"

#include "Settings.h"
#include "Utils.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>

CustomWidget::CustomWidget(ScreenManager &manager) : Widget(manager) {}

void CustomWidget::setup() {
}

bool CustomWidget::fetchValue(const String &url, const String &field, String &out) {
    HTTPClient http;
    http.begin(url);
    int code = http.GET();
    if (code <= 0) {
        http.end();
        return false;
    }
    String body = http.getString();
    http.end();

    if (field.length() == 0) {
        // No JSON field requested - show the raw response (trimmed/truncated)
        body.trim();
        if (body.length() > 180) {
            body = body.substring(0, 180);
        }
        out = body;
        return true;
    }

    JsonDocument doc;
    if (deserializeJson(doc, body)) {
        return false;
    }
    // Walk a dot-path like "data.amount" into the JSON
    JsonVariant v = doc.as<JsonVariant>();
    int start = 0;
    while (start <= field.length()) {
        int dot = field.indexOf('.', start);
        String token = (dot < 0) ? field.substring(start) : field.substring(start, dot);
        v = v[token];
        if (dot < 0) {
            break;
        }
        start = dot + 1;
    }
    if (v.isNull()) {
        return false;
    }
    out = v.as<String>();
    return true;
}

void CustomWidget::update(bool force) {
    String title = settings.getCustomTitle();
    String url = settings.getCustomUrl();
    String text = settings.getCustomText();
    String field = settings.getCustomField();

    if (title != m_title) {
        m_title = title;
        m_changed = true;
    }

    bool configChanged = (url != m_loadedUrl) || (text != m_loadedText);
    bool timeToRefresh = (m_delayPrev == 0) || (millis() - m_delayPrev) >= m_delay;

    if (!force && !configChanged && !timeToRefresh) {
        return;
    }

    String newValue;
    if (url.length() > 0) {
        setBusy(true);
        if (!fetchValue(url, field, newValue)) {
            newValue = "fetch error";
        }
        setBusy(false);
    } else {
        newValue = text; // static text (empty -> placeholder shown in draw)
    }

    m_loadedUrl = url;
    m_loadedText = text;
    m_delayPrev = millis();

    if (newValue != m_value) {
        m_value = newValue;
        m_changed = true;
    }
}

void CustomWidget::draw(bool force) {
    if (!m_changed && !force) {
        return;
    }
    m_changed = false;

    m_manager.selectScreen(2);
    m_manager.fillScreen(TFT_BLACK);
    m_manager.setFontColor(TFT_SKYBLUE);
    m_manager.drawCentreString(m_title, ScreenCenterX, 62, 20);
    m_manager.setFontColor(TFT_WHITE);
    String v = m_value.length() > 0 ? m_value : "(set in web panel)";
    m_manager.drawFittedString(v, ScreenCenterX, ScreenCenterY + 20, 200, 110, Align::MiddleCenter);
}

void CustomWidget::buttonPressed(uint8_t buttonId, ButtonState state) {
}

String CustomWidget::getName() {
    return "Custom";
}
