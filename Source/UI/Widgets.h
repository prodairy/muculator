#pragma once

#include "ConsoleLookAndFeel.h"
#include "Calc.h"
#include <map>

namespace ui
{
juce::String str (const std::string& s);

//==============================================================================
/** Square illuminated console button. */
class IllumButton : public juce::TextButton
{
public:
    IllumButton (const juce::String& text, juce::Colour cap, bool toggles = true);
    void setCap (juce::Colour c);
    /** Lights the button briefly (used for TAP). */
    void flash();

private:
    juce::uint32 flashUntil = 0;
    struct Ticker : juce::Timer { IllumButton* owner = nullptr; void timerCallback() override; } ticker;
};

/** CHAIN toggle with a drawn chain-link icon. */
class ChainButton final : public IllumButton
{
public:
    ChainButton();
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
};

//==============================================================================
/** Caption + LED text entry + unit. Text is persisted to a ValueTree property when bound. */
class LedField final : public juce::Component
{
public:
    LedField (const juce::String& caption, const juce::String& unit = {}, const juce::String& allowedChars = "0123456789.,-");

    void bind (juce::ValueTree tree, const juce::Identifier& prop, const juce::String& defaultText);
    juce::String getText() const { return editor.getText(); }
    std::string text() const { return editor.getText().toStdString(); }
    void setText (const juce::String& t);   // does not fire onChange
    void setInvalid (bool shouldBeInvalid);
    void setLocked (bool locked, const juce::String& badge = {});
    void setCaption (const juce::String& c) { caption = c; repaint(); }
    void setFontHeight (float h);
    bool isEditing() const { return editor.hasKeyboardFocus (true); }

    std::function<void()> onChange;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::String caption, unit, badge;
    juce::TextEditor editor;
    juce::ValueTree boundTree;
    juce::Identifier boundProp;
    float fontHeight = 17.0f;
};

//==============================================================================
/** Note name + octave selector. */
class NotePicker final : public juce::Component
{
public:
    explicit NotePicker (const juce::String& caption);
    void bind (juce::ValueTree tree, const juce::Identifier& prop, int defaultMidi);
    int getMidi() const;
    void setMidi (int midi);
    std::function<void()> onChange;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::String caption;
    juce::ComboBox pitch, octave;
    juce::ValueTree boundTree;
    juce::Identifier boundProp;
    void changed();
};

//==============================================================================
/** Black screen with label/value lines. */
class LedReadout final : public juce::Component
{
public:
    struct Line
    {
        juce::String label, value;
        juce::Colour colour = col::ledAmber;
        float scale = 1.0f; // value size multiplier
    };

    void setLines (std::vector<Line> newLines);
    void setLineHeight (float h) { lineHeight = h; repaint(); }
    void paint (juce::Graphics&) override;

private:
    std::vector<Line> lines;
    float lineHeight = 21.0f;
};

//==============================================================================
/** LED table with a recommended row (amber band) and a user-selected row (green frame). */
class LedTable final : public juce::Component
{
public:
    void setColumns (juce::StringArray headers, std::vector<float> relativeWidths);
    void setRows (std::vector<juce::StringArray> rows);
    void setRecommendedRow (int r)   { recommended = r; repaint(); }
    void setSelectedRow (int r)      { selected = r; repaint(); }
    void setSelectable (bool s)      { selectable = s; }
    void setRowColour (int row, juce::Colour c);
    int getSelectedRow() const       { return selected; }
    std::function<void (int)> onRowClicked;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    juce::StringArray headers;
    std::vector<float> widths;
    std::vector<juce::StringArray> rows;
    std::map<int, juce::Colour> rowColours;
    int recommended = -1, selected = -1, hover = -1;
    bool selectable = false;
    float headerHeight() const { return 22.0f; }
    float rowHeight() const;
    int rowAt (juce::Point<int>) const;
};

//==============================================================================
/** Warning lamp + text. Hidden when the message is empty. */
class WarningPanel final : public juce::Component, private juce::Timer
{
public:
    enum class Level { Error, Microtone, Info };
    void setMessage (const juce::String& title, const juce::String& body, Level level = Level::Error);
    void clear() { setMessage ({}, {}); }
    bool hasMessage() const { return title.isNotEmpty() || body.isNotEmpty(); }
    void paint (juce::Graphics&) override;

private:
    juce::String title, body;
    Level level = Level::Error;
    bool blinkOn = true;
    void timerCallback() override { blinkOn = ! blinkOn; repaint(); }
};

//==============================================================================
/** Bundles an IllumButton radio group. */
class ButtonGroup final : public juce::Component
{
public:
    ButtonGroup (juce::StringArray labels, juce::Colour cap, int radioId, bool vertical);
    void setSelected (int index, bool notify);
    int getSelected() const { return selected; }
    IllumButton& button (int i) { return *buttons[(size_t) i]; }
    int size() const { return (int) buttons.size(); }
    void setColumns (int c) { columns = c; resized(); }
    std::function<void (int)> onSelect;
    void resized() override;

private:
    std::vector<std::unique_ptr<IllumButton>> buttons;
    int selected = -1;
    bool vertical;
    int columns = 0;
};
} // namespace ui
