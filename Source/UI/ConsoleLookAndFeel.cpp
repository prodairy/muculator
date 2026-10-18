#include "ConsoleLookAndFeel.h"

namespace ui
{
juce::Font ledFont (float height, bool bold)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), height,
                                          bold ? juce::Font::bold : juce::Font::plain));
}

juce::Font labelFont (float height, bool bold)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), height,
                                          bold ? juce::Font::bold : juce::Font::plain));
}

juce::Rectangle<int> drawSection (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& title)
{
    auto r = area.toFloat();
    g.setColour (col::panelEdge);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (col::recess);
    g.fillRoundedRectangle (r.reduced (1.0f), 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawRoundedRectangle (r.reduced (1.5f), 4.0f, 1.0f);

    auto inner = area.reduced (8);
    if (title.isNotEmpty())
    {
        auto strip = inner.removeFromTop (18);
        g.setColour (col::silkscreen);
        g.setFont (labelFont (11.5f));
        g.drawText (title.toUpperCase(), strip, juce::Justification::centredLeft);
        const int tw = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), title.toUpperCase());
        g.setColour (col::silkDim.withAlpha (0.5f));
        g.fillRect (strip.getX() + tw + 8, strip.getCentreY(), juce::jmax (0, strip.getWidth() - tw - 8), 1);
        inner.removeFromTop (4);
    }
    return inner;
}

void drawScreen (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (col::screen);
    g.fillRoundedRectangle (r.reduced (1.5f), 2.5f);
    juce::ColourGradient glare (juce::Colours::white.withAlpha (0.045f), r.getX(), r.getY(),
                                juce::Colours::transparentWhite, r.getX(), r.getCentreY(), false);
    g.setGradientFill (glare);
    g.fillRoundedRectangle (r.reduced (1.5f), 2.5f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawLine (r.getX() + 3, r.getBottom() - 0.5f, r.getRight() - 3, r.getBottom() - 0.5f, 1.0f);
}

void drawLed (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, bool lit)
{
    if (lit)
    {
        g.setColour (c.withAlpha (0.25f));
        g.fillEllipse (r.expanded (r.getWidth() * 0.45f));
    }
    g.setColour (lit ? c : c.darker (2.2f).withSaturation (0.4f));
    g.fillEllipse (r);
    g.setColour (juce::Colours::white.withAlpha (lit ? 0.6f : 0.12f));
    g.fillEllipse (r.reduced (r.getWidth() * 0.32f).translated (-r.getWidth() * 0.12f, -r.getHeight() * 0.12f));
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (r, 1.0f);
}

void drawScrew (juce::Graphics& g, juce::Point<float> c, float rad)
{
    juce::ColourGradient grad (juce::Colour (0xffb8b8b8), c.x - rad, c.y - rad,
                               juce::Colour (0xff4a4a4a), c.x + rad, c.y + rad, false);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - rad, c.y - rad, rad * 2, rad * 2);
    g.setColour (juce::Colour (0xff222222));
    g.drawLine (c.x - rad * 0.6f, c.y + rad * 0.2f, c.x + rad * 0.6f, c.y - rad * 0.2f, 1.4f);
}

//==============================================================================
ConsoleLookAndFeel::ConsoleLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, col::panel);
    setColour (juce::PopupMenu::backgroundColourId, col::panelDark);
    setColour (juce::PopupMenu::textColourId, col::silkscreen);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::ledAmber.withAlpha (0.85f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    setColour (juce::ComboBox::textColourId, col::ledAmber);
    setColour (juce::ComboBox::arrowColourId, col::ledAmber);
    setColour (juce::Label::textColourId, col::silkscreen);
    setColour (juce::TextEditor::textColourId, col::ledAmber);
    setColour (juce::TextEditor::highlightColourId, col::ledAmber.withAlpha (0.35f));
    setColour (juce::TextEditor::highlightedTextColourId, juce::Colours::white);
    setColour (juce::CaretComponent::caretColourId, col::ledAmber);
    setColour (juce::TooltipWindow::backgroundColourId, col::panelDark);
    setColour (juce::TooltipWindow::textColourId, col::silkscreen);
}

void ConsoleLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                           bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    const auto cap = juce::Colour ((juce::uint32) (juce::int64) b.getProperties().getWithDefault ("cap", (juce::int64) col::capWhite.getARGB()));
    const bool lit = b.getToggleState() || (bool) b.getProperties()["flash"];
    const float corner = 3.5f;

    // bezel
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.fillRoundedRectangle (r.translated (0, 1.5f), corner);
    g.setColour (col::panelEdge);
    g.fillRoundedRectangle (r, corner);

    auto face = r.reduced (2.0f);
    if (lit)
    {
        g.setColour (cap.withAlpha (0.28f));
        g.fillRoundedRectangle (r.expanded (2.5f), corner + 2);
        juce::ColourGradient grad (cap.brighter (0.45f), face.getCentreX(), face.getCentreY(),
                                   cap.darker (0.15f), face.getX(), face.getY(), true);
        g.setGradientFill (grad);
    }
    else
    {
        juce::ColourGradient grad (cap.darker (1.7f).withMultipliedSaturation (0.55f), face.getX(), face.getY(),
                                   cap.darker (2.4f).withMultipliedSaturation (0.45f), face.getX(), face.getBottom(), false);
        g.setGradientFill (grad);
    }
    g.fillRoundedRectangle (face, corner - 1);

    if (highlighted && ! down)
    {
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.fillRoundedRectangle (face, corner - 1);
    }
    if (down)
    {
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        g.fillRoundedRectangle (face, corner - 1);
    }
    g.setColour (juce::Colours::white.withAlpha (lit ? 0.35f : 0.1f));
    g.drawLine (face.getX() + 3, face.getY() + 1, face.getRight() - 3, face.getY() + 1, 1.0f);
}

void ConsoleLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    const bool lit = b.getToggleState() || (bool) b.getProperties()["flash"];
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (lit ? juce::Colour (0xff141414) : col::silkscreen.withAlpha (b.isEnabled() ? 0.85f : 0.35f));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (4, 2), juce::Justification::centred, 2, 0.8f);
}

juce::Font ConsoleLookAndFeel::getTextButtonFont (juce::TextButton&, int h)
{
    return labelFont (juce::jlimit (9.5f, 13.0f, h * 0.42f));
}

void ConsoleLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    drawScreen (g, juce::Rectangle<float> (0, 0, (float) w, (float) h));
    juce::Path arrow;
    const float ax = (float) w - 13.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId).withAlpha (box.isEnabled() ? 0.9f : 0.3f));
    g.fillPath (arrow);
}

juce::Font ConsoleLookAndFeel::getComboBoxFont (juce::ComboBox& b) { return ledFont (juce::jmin (16.0f, b.getHeight() * 0.6f)); }

void ConsoleLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (4, 1, box.getWidth() - 22, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (juce::Justification::centred);
}

juce::Font ConsoleLookAndFeel::getPopupMenuFont() { return ledFont (15.0f); }

void ConsoleLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (col::panelDark);
    g.setColour (col::panelEdge);
    g.drawRect (0, 0, w, h);
}

void ConsoleLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int w, int h, juce::TextEditor&)
{
    drawScreen (g, juce::Rectangle<float> (0, 0, (float) w, (float) h));
}

void ConsoleLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int w, int h, juce::TextEditor& ed)
{
    const auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    if ((bool) ed.getProperties()["invalid"])
    {
        g.setColour (col::ledRed);
        g.drawRoundedRectangle (r, 3.0f, 1.6f);
    }
    else if (ed.hasKeyboardFocus (true) && ! ed.isReadOnly())
    {
        g.setColour (col::ledAmber.withAlpha (0.6f));
        g.drawRoundedRectangle (r, 3.0f, 1.2f);
    }
}

juce::Font ConsoleLookAndFeel::getLabelFont (juce::Label& l) { return labelFont (juce::jmin (12.0f, l.getHeight() * 0.8f)); }
} // namespace ui
