#include "Widgets.h"

namespace ui
{
juce::String str (const std::string& s) { return juce::String::fromUTF8 (s.c_str()); }

//==============================================================================
IllumButton::IllumButton (const juce::String& text, juce::Colour cap, bool toggles)
    : juce::TextButton (text)
{
    setCap (cap);
    setClickingTogglesState (toggles);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    ticker.owner = this;
}

void IllumButton::setCap (juce::Colour c)
{
    getProperties().set ("cap", (juce::int64) c.getARGB());
    repaint();
}

void IllumButton::flash()
{
    getProperties().set ("flash", true);
    flashUntil = juce::Time::getMillisecondCounter() + 110;
    ticker.startTimer (30);
    repaint();
}

void IllumButton::Ticker::timerCallback()
{
    if (juce::Time::getMillisecondCounter() >= owner->flashUntil)
    {
        owner->getProperties().set ("flash", false);
        owner->repaint();
        stopTimer();
    }
}

//==============================================================================
ChainButton::ChainButton() : IllumButton ({}, col::capYellow, true)
{
    setTooltip ("CHAIN: when lit, every module uses the same BPM");
}

void ChainButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    getLookAndFeel().drawButtonBackground (g, *this, {}, highlighted, down);

    const bool lit = getToggleState();
    auto r = getLocalBounds().toFloat().reduced (getWidth() * 0.2f, getHeight() * 0.3f);
    const float lw = r.getWidth() * 0.55f, lh = r.getHeight() * 0.62f, stroke = juce::jmax (2.0f, r.getHeight() * 0.13f);
    const auto c = lit ? juce::Colour (0xff151515) : col::silkscreen.withAlpha (0.75f);

    g.setColour (c);
    juce::Rectangle<float> left (r.getX(), r.getCentreY() - lh * 0.5f, lw, lh);
    juce::Rectangle<float> right (r.getRight() - lw, r.getCentreY() - lh * 0.5f, lw, lh);
    if (lit)
    {
        g.drawRoundedRectangle (left, lh * 0.5f, stroke);
        g.drawRoundedRectangle (right, lh * 0.5f, stroke);
    }
    else
    {
        // broken chain: links pulled apart and slightly tilted
        juce::Path p;
        p.addRoundedRectangle (left.translated (-r.getWidth() * 0.08f, 0), lh * 0.5f);
        p.applyTransform (juce::AffineTransform::rotation (-0.25f, left.getCentreX(), left.getCentreY()));
        g.strokePath (p, juce::PathStrokeType (stroke));
        juce::Path q;
        q.addRoundedRectangle (right.translated (r.getWidth() * 0.08f, 0), lh * 0.5f);
        q.applyTransform (juce::AffineTransform::rotation (-0.25f, right.getCentreX(), right.getCentreY()));
        g.strokePath (q, juce::PathStrokeType (stroke));
    }

    g.setFont (labelFont (9.0f));
    g.drawText ("CHAIN", getLocalBounds().removeFromBottom (13), juce::Justification::centred);
}

//==============================================================================
LedField::LedField (const juce::String& cap, const juce::String& u, const juce::String& allowed)
    : caption (cap), unit (u)
{
    editor.setJustification (juce::Justification::centred);
    editor.setFont (ledFont (fontHeight));
    editor.setSelectAllWhenFocused (true);
    editor.setInputRestrictions (16, allowed);
    editor.setIndents (4, 0);
    editor.setColour (juce::TextEditor::textColourId, col::ledAmber);
    editor.setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    editor.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    editor.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    editor.onTextChange = [this]
    {
        if (boundTree.isValid())
            boundTree.setProperty (boundProp, editor.getText(), nullptr);
        if (onChange) onChange();
    };
    editor.onReturnKey = [this] { editor.giveAwayKeyboardFocus(); };
    editor.onEscapeKey = [this] { editor.giveAwayKeyboardFocus(); };
    addAndMakeVisible (editor);
}

void LedField::bind (juce::ValueTree tree, const juce::Identifier& prop, const juce::String& defaultText)
{
    boundTree = tree;
    boundProp = prop;
    if (! tree.hasProperty (prop))
        tree.setProperty (prop, defaultText, nullptr);
    editor.setText (tree.getProperty (prop).toString(), juce::dontSendNotification);
    editor.applyColourToAllText (editor.findColour (juce::TextEditor::textColourId));
}

void LedField::setText (const juce::String& t)
{
    if (editor.getText() == t)
        return;
    editor.setText (t, juce::dontSendNotification);
    editor.applyColourToAllText (editor.findColour (juce::TextEditor::textColourId));
    if (boundTree.isValid())
        boundTree.setProperty (boundProp, t, nullptr);
}

void LedField::setInvalid (bool b)
{
    if ((bool) editor.getProperties()["invalid"] == b)
        return;
    editor.getProperties().set ("invalid", b);
    editor.setColour (juce::TextEditor::textColourId, b ? col::ledRed : col::ledAmber);
    editor.applyColourToAllText (b ? col::ledRed : col::ledAmber);
    editor.repaint();
}

void LedField::setLocked (bool locked, const juce::String& b)
{
    badge = locked ? b : juce::String();
    editor.setReadOnly (locked);
    editor.setCaretVisible (! locked);
    editor.setAlpha (locked ? 0.75f : 1.0f);
    editor.setMouseCursor (locked ? juce::MouseCursor::NormalCursor : juce::MouseCursor::IBeamCursor);
    repaint();
}

void LedField::setFontHeight (float h)
{
    fontHeight = h;
    editor.setFont (ledFont (h));
    editor.applyFontToAllText (ledFont (h));
    resized();
}

void LedField::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();
    auto top = r.removeFromTop (15);
    g.setFont (labelFont (10.5f));
    g.setColour (col::silkscreen.withAlpha (0.85f));
    g.drawText (caption.toUpperCase(), top, juce::Justification::centredLeft);
    if (badge.isNotEmpty())
    {
        g.setColour (col::capYellow);
        g.drawText (badge, top, juce::Justification::centredRight);
    }
    if (unit.isNotEmpty())
    {
        g.setFont (labelFont (10.5f, false));
        g.setColour (col::silkDim);
        g.drawText (unit, r.removeFromRight (34), juce::Justification::centred);
    }
}

void LedField::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (16);
    if (unit.isNotEmpty())
        r.removeFromRight (34);
    editor.setBounds (r);
    const int pad = juce::jmax (0, (r.getHeight() - (int) fontHeight) / 2 - 2);
    editor.setBorder ({ pad, 2, 0, 2 });
}

//==============================================================================
NotePicker::NotePicker (const juce::String& cap) : caption (cap)
{
    for (int i = 0; i < 12; ++i)
        pitch.addItem (str (calc::pitchClassName (i)), i + 1);
    for (int o = -1; o <= 9; ++o)
        octave.addItem (juce::String (o), o + 2);
    pitch.onChange = octave.onChange = [this] { changed(); };
    addAndMakeVisible (pitch);
    addAndMakeVisible (octave);
    setMidi (69);
}

void NotePicker::bind (juce::ValueTree tree, const juce::Identifier& prop, int defaultMidi)
{
    boundTree = tree;
    boundProp = prop;
    setMidi ((int) tree.getProperty (prop, defaultMidi));
}

int NotePicker::getMidi() const
{
    return calc::noteToMidi (juce::jmax (0, pitch.getSelectedId() - 1), octave.getSelectedId() - 2);
}

void NotePicker::setMidi (int midi)
{
    midi = juce::jlimit (0, 131, midi);
    pitch.setSelectedId (midi % 12 + 1, juce::dontSendNotification);
    octave.setSelectedId (juce::jlimit (-1, 9, midi / 12 - 1) + 2, juce::dontSendNotification);
}

void NotePicker::setFlats (bool flats)
{
    for (int i = 0; i < 12; ++i)
        pitch.changeItemText (i + 1, str (calc::pitchClassName (i, flats)));
    pitch.setSelectedId (pitch.getSelectedId(), juce::dontSendNotification);
    pitch.repaint();
}

void NotePicker::changed()
{
    if (boundTree.isValid())
        boundTree.setProperty (boundProp, getMidi(), nullptr);
    if (onChange) onChange();
}

void NotePicker::paint (juce::Graphics& g)
{
    g.setFont (labelFont (10.5f));
    g.setColour (col::silkscreen.withAlpha (0.85f));
    g.drawText (caption.toUpperCase(), getLocalBounds().removeFromTop (15), juce::Justification::centredLeft);
}

void NotePicker::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (16);
    pitch.setBounds (r.removeFromLeft (r.getWidth() * 55 / 100));
    r.removeFromLeft (6);
    octave.setBounds (r);
}

//==============================================================================
void LedReadout::setLines (std::vector<Line> l)
{
    lines = std::move (l);
    repaint();
}

void LedReadout::paint (juce::Graphics& g)
{
    drawScreen (g, getLocalBounds().toFloat());
    auto r = getLocalBounds().reduced (10, 6).toFloat();
    for (auto& line : lines)
    {
        const float h = lineHeight * juce::jmax (1.0f, line.scale * 0.85f);
        auto row = r.removeFromTop (h);
        if (row.getBottom() > (float) getHeight())
            break;
        if (line.label.isNotEmpty())
        {
            g.setColour (col::silkDim);
            g.setFont (ledFont (lineHeight * 0.55f, false));
            g.drawText (line.label, row, juce::Justification::centredLeft);
        }
        g.setColour (line.colour);
        g.setFont (ledFont (lineHeight * 0.72f * line.scale));
        if (line.label.isEmpty())
            g.drawFittedText (line.value, row.toNearestInt(), juce::Justification::centredLeft, 2, 0.7f);
        else
            g.drawText (line.value, row, juce::Justification::centredRight);
    }
}

//==============================================================================
void LedTable::setColumns (juce::StringArray h, std::vector<float> w)
{
    headers = std::move (h);
    widths = std::move (w);
    repaint();
}

void LedTable::setRows (std::vector<juce::StringArray> r)
{
    rows = std::move (r);
    rowColours.clear();
    firstRow = juce::jlimit (0, maxFirstRow(), firstRow);
    repaint();
}

void LedTable::setRowColour (int row, juce::Colour c) { rowColours[row] = c; repaint(); }

float LedTable::rowHeight() const
{
    if (rows.empty()) return 20.0f;
    return juce::jlimit (18.0f, 30.0f, ((float) getHeight() - headerHeight() - 8.0f) / (float) rows.size());
}

int LedTable::visibleRows() const
{
    return juce::jmax (1, (int) (((float) getHeight() - headerHeight() - 8.0f) / rowHeight()));
}

void LedTable::scrollTo (int row)
{
    row = juce::jlimit (0, maxFirstRow(), row);
    if (row != firstRow)
    {
        firstRow = row;
        hover = -1;
        repaint();
    }
}

int LedTable::rowAt (juce::Point<int> p) const
{
    const float y = (float) p.y - 4.0f - headerHeight();
    if (y < 0) return -1;
    const int visible = (int) (y / rowHeight());
    if (visible >= visibleRows()) return -1;
    const int r = firstRow + visible;
    return r < (int) rows.size() ? r : -1;
}

void LedTable::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (maxFirstRow() == 0)
    {
        Component::mouseWheelMove (e, w);
        return;
    }
    // Trackpads send many small deltas: add them up and move one row per step.
    wheelAccum += w.deltaY * (w.isReversed ? -1.0f : 1.0f);
    const float step = w.isSmooth ? 0.04f : 0.0f;
    int rowsToMove = 0;
    if (step == 0.0f)
    {
        rowsToMove = wheelAccum > 0 ? -3 : 3;
        wheelAccum = 0;
    }
    else
    {
        while (wheelAccum >= step)  { --rowsToMove; wheelAccum -= step; }
        while (wheelAccum <= -step) { ++rowsToMove; wheelAccum += step; }
    }
    scrollTo (firstRow + rowsToMove);
}

void LedTable::paint (juce::Graphics& g)
{
    drawScreen (g, getLocalBounds().toFloat());
    auto area = getLocalBounds().toFloat().reduced (6.0f, 4.0f);
    const bool scrolls = maxFirstRow() > 0;
    const auto barArea = scrolls ? area.removeFromRight (6.0f).withTrimmedLeft (3.0f) : juce::Rectangle<float>();

    float total = 0;
    for (auto w : widths) total += w;
    std::vector<float> xs;
    float x = area.getX();
    for (auto w : widths) { xs.push_back (x); x += area.getWidth() * w / total; }
    xs.push_back (area.getRight());

    auto header = area.removeFromTop (headerHeight());
    g.setFont (labelFont (10.5f));
    g.setColour (col::silkDim);
    for (int c = 0; c < headers.size() && c + 1 < (int) xs.size(); ++c)
        g.drawText (headers[c], juce::Rectangle<float> (xs[(size_t) c], header.getY(), xs[(size_t) c + 1] - xs[(size_t) c], header.getHeight()).reduced (3, 0),
                    c == 0 ? juce::Justification::centredLeft : juce::Justification::centredRight);
    g.setColour (col::silkDim.withAlpha (0.3f));
    g.drawHorizontalLine ((int) header.getBottom(), area.getX(), area.getRight());

    const float rh = rowHeight();
    const int lastRow = juce::jmin ((int) rows.size(), firstRow + visibleRows());
    if (scrolls)
    {
        const auto track = barArea.withY (area.getY()).withHeight (rh * (float) visibleRows());
        g.setColour (col::silkDim.withAlpha (0.15f));
        g.fillRoundedRectangle (track, 1.5f);
        const float n = (float) rows.size();
        g.setColour (col::ledAmber.withAlpha (0.6f));
        g.fillRoundedRectangle (track.withY (track.getY() + track.getHeight() * (float) firstRow / n)
                                     .withHeight (track.getHeight() * (float) visibleRows() / n), 1.5f);
    }
    for (int i = firstRow; i < lastRow; ++i)
    {
        auto row = area.removeFromTop (rh);
        if (i == recommended)
        {
            g.setColour (col::ledAmber.withAlpha (0.16f));
            g.fillRect (row);
            g.setColour (col::ledAmber);
            g.fillRect (row.withWidth (3.0f));
        }
        else if (i % 2 == 1)
        {
            g.setColour (juce::Colours::white.withAlpha (0.025f));
            g.fillRect (row);
        }
        if (selectable && i == hover)
        {
            g.setColour (juce::Colours::white.withAlpha (0.06f));
            g.fillRect (row);
        }
        if (i == selected)
        {
            g.setColour (col::ledGreen.withAlpha (0.9f));
            g.drawRect (row.reduced (0.5f), 1.5f);
        }

        const auto baseColour = rowColours.count (i) ? rowColours[i]
                              : i == recommended ? col::ledAmber
                              : i == selected ? col::ledGreen
                              : col::ledAmber.withAlpha (0.72f);
        g.setFont (ledFont (juce::jmin (15.5f, rh * 0.68f), i == recommended || i == selected));
        const auto& cells = rows[(size_t) i];
        for (int c = 0; c < cells.size() && c + 1 < (int) xs.size(); ++c)
        {
            g.setColour (c == 0 ? baseColour.brighter (0.15f) : baseColour);
            g.drawText (cells[c], juce::Rectangle<float> (xs[(size_t) c], row.getY(), xs[(size_t) c + 1] - xs[(size_t) c], rh).reduced (5, 0),
                        c == 0 ? juce::Justification::centredLeft : juce::Justification::centredRight);
        }
    }
}

void LedTable::mouseDown (const juce::MouseEvent& e)
{
    if (! selectable) return;
    const int r = rowAt (e.getPosition());
    if (r >= 0)
    {
        selected = r;
        repaint();
        if (onRowClicked) onRowClicked (r);
    }
}

void LedTable::mouseMove (const juce::MouseEvent& e)
{
    if (! selectable) return;
    const int r = rowAt (e.getPosition());
    if (r != hover) { hover = r; repaint(); }
    setMouseCursor (r >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void LedTable::mouseExit (const juce::MouseEvent&) { hover = -1; repaint(); }

//==============================================================================
void WarningPanel::setMessage (const juce::String& t, const juce::String& b, Level l)
{
    if (t == title && b == body && l == level)
        return;
    title = t; body = b; level = l;
    if (hasMessage() && level != Level::Info) startTimer (450); else stopTimer();
    blinkOn = true;
    setVisible (hasMessage());
    repaint();
}

void WarningPanel::paint (juce::Graphics& g)
{
    if (! hasMessage()) return;
    const auto c = level == Level::Error ? col::ledRed : level == Level::Microtone ? col::ledAmber : col::ledBlue;
    auto r = getLocalBounds().toFloat();

    g.setColour (c.withAlpha (level == Level::Microtone ? 0.16f : 0.1f));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (c.withAlpha (blinkOn ? 1.0f : 0.45f));
    g.drawRoundedRectangle (r.reduced (1.0f), 4.0f, level == Level::Microtone ? 2.5f : 1.5f);

    auto inner = getLocalBounds().reduced (10, 6);
    const int ledSize = juce::jmin (14, inner.getHeight());
    drawLed (g, inner.removeFromLeft (ledSize).withSizeKeepingCentre (ledSize, ledSize).toFloat(), c, blinkOn);
    inner.removeFromLeft (10);

    const bool big = level == Level::Microtone;
    if (title.isNotEmpty())
    {
        g.setColour (c);
        g.setFont (labelFont (big ? 17.0f : 12.5f));
        auto titleArea = body.isEmpty() ? inner : inner.removeFromTop (big ? 24 : 17);
        g.drawText (title, titleArea, juce::Justification::centredLeft);
    }
    if (body.isNotEmpty())
    {
        g.setColour (c.interpolatedWith (juce::Colours::white, 0.35f));
        g.setFont (ledFont (big ? 14.5f : 12.0f, false));
        g.drawFittedText (body, inner, juce::Justification::topLeft, big ? 5 : 3, 0.8f);
    }
}

//==============================================================================
HelpPanel::HelpPanel()
{
    setWantsKeyboardFocus (true);
    closeButton.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (closeButton);
}

void HelpPanel::setContent (const juce::String& t, const juce::String& text)
{
    title = t;
    columns.clear();
    for (auto rest = text;; rest = rest.fromFirstOccurrenceOf ("\n===\n", false, false))
    {
        columns.add (rest.upToFirstOccurrenceOf ("\n===\n", false, false));
        if (! rest.contains ("\n===\n"))
            break;
    }
    repaint();
}

bool HelpPanel::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey)
    {
        if (onClose) onClose();
        return true;
    }
    return false;
}

void HelpPanel::resized()
{
    closeButton.setBounds (getLocalBounds().reduced (14).removeFromTop (26).removeFromRight (80));
}

void HelpPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::panelDark);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (col::ledBlue.withAlpha (0.7f));
    g.drawRoundedRectangle (r.reduced (1.0f), 6.0f, 1.5f);

    auto area = getLocalBounds().reduced (20, 14);
    auto top = area.removeFromTop (26);
    g.setColour (col::ledBlue);
    g.setFont (labelFont (15.0f));
    g.drawText ("HOW TO USE:  " + title.toUpperCase(), top, juce::Justification::centredLeft);
    area.removeFromTop (10);

    const int n = juce::jmax (1, columns.size());
    const int gap = 30;
    const int colW = (area.getWidth() - gap * (n - 1)) / n;
    for (int c = 0; c < columns.size(); ++c)
    {
        juce::AttributedString as;
        as.setWordWrap (juce::AttributedString::byWord);
        as.setLineSpacing (2.0f);
        const auto body = col::silkscreen.withAlpha (0.88f);
        for (auto line : juce::StringArray::fromLines (columns[c]))
        {
            if (line.startsWith ("# "))
            {
                as.append ("\n", labelFont (6.0f, false), body);
                as.append (line.substring (2).toUpperCase() + "\n", labelFont (13.0f), col::capYellow);
            }
            else if (line.startsWith ("- ") && line.contains (": "))
            {
                as.append (line.substring (2).upToFirstOccurrenceOf (": ", true, false), labelFont (14.0f), col::silkscreen);
                as.append (line.fromFirstOccurrenceOf (": ", false, false) + "\n", labelFont (14.0f, false), body);
            }
            else
            {
                as.append (line + "\n", labelFont (14.0f, false), body);
            }
        }
        juce::TextLayout layout;
        layout.createLayout (as, (float) colW);
        auto colArea = area.withX (area.getX() + c * (colW + gap)).withWidth (colW);
        layout.draw (g, colArea.toFloat());
        if (c > 0)
        {
            g.setColour (col::silkDim.withAlpha (0.3f));
            g.fillRect (colArea.getX() - gap / 2, colArea.getY(), 1, colArea.getHeight());
        }
    }
}

//==============================================================================
ButtonGroup::ButtonGroup (juce::StringArray labels, juce::Colour cap, int radioId, bool v) : vertical (v)
{
    for (int i = 0; i < labels.size(); ++i)
    {
        auto b = std::make_unique<IllumButton> (labels[i], cap, true);
        b->setRadioGroupId (radioId);
        b->onClick = [this, i]
        {
            if (buttons[(size_t) i]->getToggleState() && selected != i)
            {
                selected = i;
                if (onSelect) onSelect (i);
            }
        };
        addAndMakeVisible (*b);
        buttons.push_back (std::move (b));
    }
}

void ButtonGroup::setSelected (int index, bool notify)
{
    index = juce::jlimit (0, (int) buttons.size() - 1, index);
    selected = index;
    buttons[(size_t) index]->setToggleState (true, juce::dontSendNotification);
    if (notify && onSelect) onSelect (index);
}

void ButtonGroup::resized()
{
    const int n = (int) buttons.size();
    if (n == 0) return;
    const int cols = columns > 0 ? columns : (vertical ? 1 : n);
    const int rowsN = (n + cols - 1) / cols;
    const int gap = 6;
    const float cw = (float) (getWidth() - gap * (cols - 1)) / (float) cols;
    const float ch = (float) (getHeight() - gap * (rowsN - 1)) / (float) rowsN;
    for (int i = 0; i < n; ++i)
    {
        const int c = i % cols, r = i / cols;
        buttons[(size_t) i]->setBounds (juce::Rectangle<float> (c * (cw + gap), r * (ch + gap), cw, ch).toNearestInt());
    }
}
} // namespace ui
