#include "ConsoleLookAndFeel.h"

namespace ui
{
juce::Font ledFont (float height, bool bold)
{
    const auto& name = skin().ledFont;
    return juce::Font (juce::FontOptions (name.isNotEmpty() ? name : juce::Font::getDefaultMonospacedFontName(), height,
                                          bold ? juce::Font::bold : juce::Font::plain));
}

juce::Font labelFont (float height, bool bold)
{
    const auto& name = skin().labelFont;
    return juce::Font (juce::FontOptions (name.isNotEmpty() ? name : juce::Font::getDefaultSansSerifFontName(), height,
                                          bold ? juce::Font::bold : juce::Font::plain));
}

juce::Rectangle<int> drawSection (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& title)
{
    const auto& s = skin();
    const float corner = s.sectionCorner, inner = juce::jmax (0.0f, corner - 1.0f);
    auto r = area.toFloat();
    g.setColour (s.panelEdge);
    g.fillRoundedRectangle (r, corner);
    g.setColour (s.recess);
    g.fillRoundedRectangle (r.reduced (1.0f), inner);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawRoundedRectangle (r.reduced (1.5f), inner, 1.0f);

    auto content = area.reduced (8);
    if (title.isNotEmpty())
    {
        auto strip = content.removeFromTop (18);
        g.setColour (s.text);
        g.setFont (labelFont (11.5f));
        g.drawText (title.toUpperCase(), strip, juce::Justification::centredLeft);
        const int tw = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), title.toUpperCase());
        g.setColour (s.textDim.withAlpha (0.5f));
        g.fillRect (strip.getX() + tw + 8, strip.getCentreY(), juce::jmax (0, strip.getWidth() - tw - 8), 1);
        content.removeFromTop (4);
    }
    return content;
}

void drawScreen (juce::Graphics& g, juce::Rectangle<float> r)
{
    const auto& s = skin();
    const float corner = s.screenCorner, inner = juce::jmax (0.0f, corner - 0.5f);
    g.setColour (s.screenEdge);
    g.fillRoundedRectangle (r, corner);
    g.setColour (s.screen);
    g.fillRoundedRectangle (r.reduced (1.5f), inner);
    if (! s.screenGlare)
        return;
    juce::ColourGradient glare (juce::Colours::white.withAlpha (0.045f), r.getX(), r.getY(),
                                juce::Colours::transparentWhite, r.getX(), r.getCentreY(), false);
    g.setGradientFill (glare);
    g.fillRoundedRectangle (r.reduced (1.5f), inner);
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
    if (! skin().screws)
        return;
    juce::ColourGradient grad (juce::Colour (0xffb8b8b8), c.x - rad, c.y - rad,
                               juce::Colour (0xff4a4a4a), c.x + rad, c.y + rad, false);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - rad, c.y - rad, rad * 2, rad * 2);
    g.setColour (juce::Colour (0xff222222));
    g.drawLine (c.x - rad * 0.6f, c.y + rad * 0.2f, c.x + rad * 0.6f, c.y - rad * 0.2f, 1.4f);
}

//==============================================================================
ConsoleLookAndFeel::ConsoleLookAndFeel() { applySkin(); }

void ConsoleLookAndFeel::applySkin()
{
    const auto& s = skin();
    setColour (juce::ResizableWindow::backgroundColourId, s.panel);
    setColour (juce::PopupMenu::backgroundColourId, s.panelDark);
    setColour (juce::PopupMenu::textColourId, s.text);
    setColour (juce::PopupMenu::headerTextColourId, s.textDim);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, s.capYellow.withAlpha (0.85f));
    setColour (juce::PopupMenu::highlightedTextColourId, s.litText);
    setColour (juce::ComboBox::textColourId, s.ledAmber);
    setColour (juce::ComboBox::arrowColourId, s.ledAmber);
    setColour (juce::Label::textColourId, s.text);
    setColour (juce::TextEditor::textColourId, s.ledAmber);
    setColour (juce::TextEditor::highlightColourId, s.ledAmber.withAlpha (0.35f));
    setColour (juce::TextEditor::highlightedTextColourId, s.text);
    setColour (juce::CaretComponent::caretColourId, s.ledAmber);
    setColour (juce::TooltipWindow::backgroundColourId, s.panelDark);
    setColour (juce::TooltipWindow::textColourId, s.text);
}

void ConsoleLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                               bool highlighted, bool down)
{
    const auto& s = skin();
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    const auto cap = s.cap ((Cap) (int) b.getProperties().getWithDefault ("cap", (int) Cap::White));
    const bool lit = b.getToggleState() || (bool) b.getProperties()["flash"];
    const float corner = s.buttonCorner, faceCorner = juce::jmax (0.0f, corner - 1.0f);
    auto face = r.reduced (2.0f);

    switch (s.buttonStyle)
    {
        case Skin::ButtonStyle::Glow:
        {
            // bezel
            g.setColour (juce::Colours::black.withAlpha (0.7f));
            g.fillRoundedRectangle (r.translated (0, 1.5f), corner);
            g.setColour (s.panelEdge);
            g.fillRoundedRectangle (r, corner);

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
            g.fillRoundedRectangle (face, faceCorner);
            break;
        }
        case Skin::ButtonStyle::Flat:
        {
            face = r;
            g.setColour (lit ? cap : s.buttonFace);
            g.fillRoundedRectangle (face, corner);
            g.setColour (lit ? cap.darker (0.4f) : s.panelEdge);
            g.drawRoundedRectangle (face.reduced (0.5f), corner, 1.0f);
            break;
        }
        case Skin::ButtonStyle::Gloss:
        case Skin::ButtonStyle::Glass:
        {
            const bool glass = s.buttonStyle == Skin::ButtonStyle::Glass;
            const auto base = lit ? cap : s.buttonFace;
            face = r.reduced (1.0f);
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.fillRoundedRectangle (r.translated (0, 1.0f), corner);
            if (lit)
            {
                g.setColour (cap.withAlpha (glass ? 0.22f : 0.3f));
                g.fillRoundedRectangle (r.expanded (2.0f), corner + 2);
            }
            g.setColour (s.panelEdge);
            g.fillRoundedRectangle (r, corner);
            juce::ColourGradient grad (base.brighter (glass ? 0.12f : 0.3f), 0, face.getY(),
                                       base.darker (glass ? 0.2f : 0.35f), 0, face.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (face, faceCorner);

            // sheen over the upper half
            auto top = face.withHeight (face.getHeight() * (glass ? 0.5f : 0.46f)).reduced (glass ? 1.0f : 0.0f, 0.0f);
            const float sheen = glass ? (lit ? 0.22f : 0.12f) : (lit ? 0.38f : 0.2f);
            juce::ColourGradient gloss (juce::Colours::white.withAlpha (sheen), 0, top.getY(),
                                        juce::Colours::white.withAlpha (glass ? 0.02f : sheen * 0.35f), 0, top.getBottom(), false);
            g.setGradientFill (gloss);
            g.fillRoundedRectangle (top, faceCorner);
            if (glass)
            {
                g.setColour (juce::Colours::white.withAlpha (lit ? 0.35f : 0.16f));
                g.drawRoundedRectangle (face.reduced (0.5f), faceCorner, 1.0f);
            }
            break;
        }
    }

    if (highlighted && ! down)
    {
        g.setColour (s.buttonText.withAlpha (0.08f));
        g.fillRoundedRectangle (face, faceCorner);
    }
    if (down)
    {
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        g.fillRoundedRectangle (face, faceCorner);
    }
    if (s.buttonStyle == Skin::ButtonStyle::Glow)
    {
        g.setColour (juce::Colours::white.withAlpha (lit ? 0.35f : 0.1f));
        g.drawLine (face.getX() + 3, face.getY() + 1, face.getRight() - 3, face.getY() + 1, 1.0f);
    }
}

void ConsoleLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    const bool lit = b.getToggleState() || (bool) b.getProperties()["flash"];
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (lit ? skin().litText : skin().buttonText.withAlpha (b.isEnabled() ? 0.85f : 0.35f));
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
    g.fillAll (skin().panelDark);
    g.setColour (skin().panelEdge);
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
        g.setColour (skin().ledRed);
        g.drawRoundedRectangle (r, skin().screenCorner, 1.6f);
    }
    else if (ed.hasKeyboardFocus (true) && ! ed.isReadOnly())
    {
        g.setColour (skin().ledAmber.withAlpha (0.6f));
        g.drawRoundedRectangle (r, skin().screenCorner, 1.2f);
    }
}

juce::Font ConsoleLookAndFeel::getLabelFont (juce::Label& l) { return labelFont (juce::jmin (12.0f, l.getHeight() * 0.8f)); }
} // namespace ui
