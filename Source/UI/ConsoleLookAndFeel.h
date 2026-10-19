#pragma once

#include "Skin.h"

namespace ui
{
juce::Font ledFont (float height, bool bold = true);
juce::Font labelFont (float height, bool bold = true);

/** Draws a recessed section with a silkscreen title strip. Returns the inner area. */
juce::Rectangle<int> drawSection (juce::Graphics&, juce::Rectangle<int> area, const juce::String& title);
/** Black LED screen with an inner bevel. */
void drawScreen (juce::Graphics&, juce::Rectangle<float> area);
/** Glowing LED dot. */
void drawLed (juce::Graphics&, juce::Rectangle<float> area, juce::Colour colour, bool lit);
/** Corner screw; draws nothing when the skin has none. */
void drawScrew (juce::Graphics&, juce::Point<float> centre, float radius);

/** Draws the widgets with the current skin (see Skin.h); the name comes from the first skin. */
class ConsoleLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    ConsoleLookAndFeel();
    /** Re-reads the skin's colours; call before sendLookAndFeelChange() after a skin change. */
    void applySkin();

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;

    void fillTextEditorBackground (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int w, int h, juce::TextEditor&) override;

    juce::Font getLabelFont (juce::Label&) override;
};
} // namespace ui
