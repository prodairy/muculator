#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ui
{
namespace col
{
    const juce::Colour panel      { 0xff3a3d42 };
    const juce::Colour panelDark  { 0xff2a2c30 };
    const juce::Colour panelEdge  { 0xff1a1b1e };
    const juce::Colour recess     { 0xff202225 };
    const juce::Colour screen     { 0xff0a0b0c };
    const juce::Colour silkscreen { 0xffe6e4dc };
    const juce::Colour silkDim    { 0xff9a9890 };
    const juce::Colour ledRed     { 0xffff3b2e };
    const juce::Colour ledAmber   { 0xffffb020 };
    const juce::Colour ledGreen   { 0xff3dff86 };
    const juce::Colour ledBlue    { 0xff48a8ff };
    const juce::Colour capRed     { 0xffe0342a };
    const juce::Colour capYellow  { 0xfff2c200 };
    const juce::Colour capGreen   { 0xff27c24c };
    const juce::Colour capBlue    { 0xff2f86ff };
    const juce::Colour capWhite   { 0xffeeeeea };
    const juce::Colour capOrange  { 0xffff7a1a };
}

juce::Font ledFont (float height, bool bold = true);
juce::Font labelFont (float height, bool bold = true);

/** Draws a recessed console-style section with a silkscreen title strip. Returns the inner area. */
juce::Rectangle<int> drawSection (juce::Graphics&, juce::Rectangle<int> area, const juce::String& title);
/** Black LED screen with an inner bevel. */
void drawScreen (juce::Graphics&, juce::Rectangle<float> area);
/** Glowing LED dot. */
void drawLed (juce::Graphics&, juce::Rectangle<float> area, juce::Colour colour, bool lit);
void drawScrew (juce::Graphics&, juce::Point<float> centre, float radius);

class ConsoleLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    ConsoleLookAndFeel();

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
