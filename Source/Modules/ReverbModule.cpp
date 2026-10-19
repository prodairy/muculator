#include "Modules.h"

using namespace ui;

ReverbModule::ReverbModule (ModuleContext& c)
    : ModuleBase ("reverb", "Reverb Time", c),
      styles ({ "SMALL ROOM", "ROOM", "LARGE ROOM", "PLATE", "HALL" }, Cap::Blue, 101, true)
{
    addAndMakeVisible (styles);
    addAndMakeVisible (table);
    addAndMakeVisible (room);

    table.setColumns ({ "LENGTH", "PRE-DELAY", "DECAY", "TOTAL", "TOTAL s", "" }, { 1.0f, 1.5f, 1.3f, 1.3f, 1.0f, 0.7f });
    table.setSelectable (true);
    table.onRowClicked = [this] (int r)
    {
        state().setProperty ("row", r, nullptr);
        refresh();
    };

    styles.setSelected ((int) state().getProperty ("style", 4), false);
    styles.onSelect = [this] (int s)
    {
        state().setProperty ("style", s, nullptr);
        state().setProperty ("row", -1, nullptr); // back to the recommendation for the new style
        refresh();
    };
    refresh();
}

void ReverbModule::refresh()
{
    const auto b = bpm();
    if (! b.ok())
    {
        showErrors ({ str (b.error) });
        table.setRows ({});
        room.setLines ({ { {}, "Enter a valid BPM in the top bar", skin().ledRed } });
        return;
    }
    showErrors ({});

    const auto style = (calc::ReverbStyle) juce::jlimit (0, calc::kNumReverbStyles - 1, styles.getSelected());
    const auto& info = calc::styleInfo (style);
    const auto rows = calc::reverbRows (*b.value, style);
    const int rec = calc::recommendedReverbRow (rows, style);
    int sel = (int) state().getProperty ("row", -1);
    if (sel < 0 || sel >= (int) rows.size())
        sel = rec;

    std::vector<juce::StringArray> cells;
    for (int i = 0; i < (int) rows.size(); ++i)
    {
        const auto& r = rows[(size_t) i];
        cells.push_back ({ str (r.label),
                           str (calc::fmtMs (r.preDelayMs)) + " (" + str (r.preDelayLabel) + ")",
                           str (calc::fmtMs (r.decayMs)),
                           str (calc::fmtMs (r.totalMs)),
                           str (calc::fmt (r.totalMs / 1000.0, 2)),
                           i == rec ? "< REC" : "" });
    }
    table.setRows (cells);
    table.setRecommendedRow (rec);
    table.setSelectedRow (sel);

    const auto& r = rows[(size_t) sel];
    const double rt = r.decayMs / 1000.0;
    const auto dims = calc::roomForRt60 (rt, info.absorption);
    constexpr double ft = 3.28084;
    const double totalS = r.totalMs / 1000.0;
    const bool inRange = totalS >= info.decayMinS * 0.9 && totalS <= info.decayMaxS * 1.1;

    std::vector<LedReadout::Line> lines {
        { "SELECTED", str (r.label) + "  " + juce::String (info.name) + (sel == rec ? "  (REC)" : ""), skin().ledGreen },
        { "PRE-DELAY / DECAY", str (calc::fmtMs (r.preDelayMs)) + " / " + str (calc::fmtMs (r.decayMs)) + " ms" },
        { "ROOM  L x W x H", str (calc::fmt (dims.length, 1)) + " x " + str (calc::fmt (dims.width, 1)) + " x "
                                + str (calc::fmt (dims.height, 1)) + " m", skin().ledAmber, 1.15f },
        { "", str (calc::fmt (dims.length * ft, 0)) + " x " + str (calc::fmt (dims.width * ft, 0)) + " x "
                  + str (calc::fmt (dims.height * ft, 0)) + " ft   VOL " + str (calc::fmt (dims.volume, 0)) + " m3", skin().ledAmber.withAlpha (0.7f) },
        { "TYPICAL " + juce::String (info.name),
          str (calc::fmt (info.decayMinS, 1)) + " - " + str (calc::fmt (info.decayMaxS, 1)) + " s",
          inRange ? skin().ledAmber.withAlpha (0.7f) : skin().ledRed },
    };
    if (style == calc::ReverbStyle::Plate)
        lines.push_back ({ "", "Plate = metal sheet; size shown is the equivalent room", skin().ledBlue });
    else if (! inRange)
        lines.push_back ({ "", "Selected length is outside the typical range for this style", skin().ledRed });
    room.setLines (lines);
}

void ReverbModule::paint (juce::Graphics& g)
{
    ModuleBase::paint (g);
    drawSection (g, styleArea, "Style");
    drawSection (g, tableArea, "Times @ BPM  (click a row)");
    drawSection (g, roomArea, "Room estimate (Sabine)");
}

void ReverbModule::layoutContent (juce::Rectangle<int> r)
{
    styleArea = r.removeFromLeft (170);
    r.removeFromLeft (10);
    roomArea = r.removeFromBottom (180);
    r.removeFromBottom (10);
    tableArea = r;

    auto s = styleArea.reduced (8);
    s.removeFromTop (22);
    styles.setBounds (s.withHeight (juce::jmin (s.getHeight(), 5 * 52)));
    table.setBounds (tableArea.reduced (8).withTrimmedTop (22));
    room.setBounds (roomArea.reduced (8).withTrimmedTop (22));
}

juce::String ReverbModule::helpText() const
{
    return R"(# What it does
Turns the tempo into reverb settings that breathe with the song: pre-delay, decay and total length for five reverb styles, plus a rough room size.
# How to use
1. )" + tempoStep() + R"(
2. Pick a style on the left.
3. The amber row is the recommended length for that style. Click any row to pick another.
4. Type PRE-DELAY and DECAY into your reverb plugin. Click a value to copy it.
# Example
128 BPM, HALL: the recommended 1/1 row lasts 1875 ms. Pre-delay is a 1/64 note (29.3 ms) and decay is the rest (1845.7 ms).
===
# Terms
- PRE-DELAY: the gap between the dry sound and the start of the reverb. It keeps the source clear. Here it is a 1/64 to 1/512 note and never more than 25 % of the total.
- DECAY: how long the tail rings after the pre-delay (called decay, size or RT60 on most reverbs).
- TOTAL: pre-delay + decay. It equals the note length of the row, so the tail ends on the grid.
- ROOM SIZE: a room that would ring this long, from the Sabine formula. For orientation only.
- PLATE: a reverb made with a metal sheet; its room size is only an equivalent.)";
}
