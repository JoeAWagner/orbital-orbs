#ifndef CUSTOM_WIDGET_H
#define CUSTOM_WIDGET_H

#include "Widget.h"

// A user-defined screen configured entirely from the web control panel.
// Shows a title plus either static text or a value fetched from a URL (with an
// optional dot-path JSON field). Everything is editable live - no reflashing.
class CustomWidget : public Widget {
public:
    CustomWidget(ScreenManager &manager);
    void setup() override;
    void update(bool force = false) override;
    void draw(bool force = false) override;
    void buttonPressed(uint8_t buttonId, ButtonState state) override;
    String getName() override;

private:
    bool fetchValue(const String &url, const String &field, String &out);

    String m_title;
    String m_value;
    String m_loadedUrl;  // URL currently reflected in m_value
    String m_loadedText; // static text currently reflected in m_value
    bool m_changed = true;

    unsigned long m_delay = 300000; // re-fetch every 5 minutes
    unsigned long m_delayPrev = 0;
};

#endif // CUSTOM_WIDGET_H
