#pragma once

#include "ConsoleLookAndFeel.h"
#include "Calc.h"
#include <map>

namespace ui
{
juce::String str (const std::string& s);

/** What a click on a displayed value copies: "462.00 Hz" -> "462.00", "29.3 (1/64)" -> "29.3",
    anything that is not a number with at most one unit word is copied as shown. */
juce::String copyableValue (const juce::String& shown);

/** Brief "COPIED" flash shared by the tables and readouts. */
struct CopyFlash : private juce::Timer
{
    explicit CopyFlash (juce::Component& c) : owner (c) {}
    void trigger (int row, int column) { flashRow = row; flashColumn = column; owner.repaint(); startTimer (700); }
    bool isOn (int row, int column = 0) const { return isTimerRunning() && row == flashRow && column == flashColumn; }

private:
    void timerCallback() override { stopTimer(); owner.repaint(); }
    juce::Component& owner;
    int flashRow = -1, flashColumn = -1;
};

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
    void setTooltip (const juce::String& t) { editor.setTooltip (t); }
    bool isEditing() const { return editor.hasKeyboardFocus (true); }
    /** Lets the value be changed by dragging up / down and with the mouse wheel. The callback gets
        whole steps (positive = up) and whether Shift is held for fine steps. A click still edits. */
    void setNudger (std::function<void (int steps, bool fine)> nudger);

    std::function<void()> onChange;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    juce::String caption, unit, badge;
    juce::TextEditor editor;
    juce::ValueTree boundTree;
    juce::Identifier boundProp;
    float fontHeight = 17.0f;
    std::function<void (int, bool)> nudge;
    int dragStepsApplied = 0;
    bool dragged = false;
    float wheelAccum = 0;
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
    /** Shows Db instead of C# and so on. */
    void setFlats (bool flats);
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
class LedReadout final : public juce::Component, public juce::SettableTooltipClient
{
public:
    struct Line
    {
        juce::String label, value;
        juce::Colour colour = col::ledAmber;
        float scale = 1.0f; // value size multiplier
    };

    LedReadout();
    void setLines (std::vector<Line> newLines);
    void setLineHeight (float h) { lineHeight = h; repaint(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    std::vector<Line> lines;
    float lineHeight = 21.0f;
    CopyFlash flash { *this };
    /** Row of each line, as painted; empty once it falls off the screen. */
    std::vector<juce::Rectangle<float>> lineAreas() const;
    /** Index of a copyable line under the point, or -1. */
    int copyableLineAt (juce::Point<int>) const;
};

//==============================================================================
/** LED table with a recommended row (amber band) and a user-selected row (green frame).
    Scrolls with the mouse wheel when the rows do not fit. */
class LedTable final : public juce::Component, public juce::SettableTooltipClient
{
public:
    LedTable();
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
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    juce::StringArray headers;
    std::vector<float> widths;
    std::vector<juce::StringArray> rows;
    std::map<int, juce::Colour> rowColours;
    int recommended = -1, selected = -1, hover = -1;
    int firstRow = 0;          // scroll position
    float wheelAccum = 0;
    bool selectable = false;
    CopyFlash flash { *this };
    float headerHeight() const { return 22.0f; }
    std::vector<float> columnEdges (juce::Rectangle<float> area) const;
    int columnAt (juce::Point<int>) const;
    float rowHeight() const;
    int visibleRows() const;
    int maxFirstRow() const { return juce::jmax (0, (int) rows.size() - visibleRows()); }
    void scrollTo (int row);
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
/** "How to use" overlay. Text format: "# " starts a heading, "- TERM: text" is a term with a
    bold name, a line "===" starts the second column. Closes with CLOSE or Esc. */
class HelpPanel final : public juce::Component
{
public:
    HelpPanel();
    void setContent (const juce::String& title, const juce::String& text);
    std::function<void()> onClose;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void mouseDown (const juce::MouseEvent&) override {} // keeps clicks away from the module underneath

private:
    juce::String title;
    juce::StringArray columns;
    IllumButton closeButton { "CLOSE", col::capWhite, false };
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
    void setTooltip (const juce::String& t) { for (auto& b : buttons) b->setTooltip (t); }
    std::function<void (int)> onSelect;
    void resized() override;

private:
    std::vector<std::unique_ptr<IllumButton>> buttons;
    int selected = -1;
    bool vertical;
    int columns = 0;
};
} // namespace ui
