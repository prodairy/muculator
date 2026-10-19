#include "Modules.h"

using namespace ui;

DelayModule::DelayModule (ModuleContext& c) : ModuleBase ("delay", "Delay Time", c)
{
    addAndMakeVisible (table);
    addAndMakeVisible (summary);
    addAndMakeVisible (swingField);
    addAndMakeVisible (swingOut);
    swingField.bind (state(), "swing", "60");
    swingField.setTooltip ("Share of each pair taken by the first note: 50 = straight, 66.7 = triplet feel, up to 75");
    swingField.onChange = [this] { refresh(); };
    swingField.setNudger ([this] (int steps, bool fine)
    {
        const auto p = calc::parseNumber (swingField.text(), "Swing");
        const double v = juce::jlimit (calc::kMinSwingPercent, calc::kMaxSwingPercent, p.value.value_or (60.0) + steps * (fine ? 0.1 : 1.0));
        swingField.setText (str (calc::fmt (v, fine ? 1 : 0)));
        refresh();
    });
    swingOut.setLineHeight (21.0f);
    table.setColumns ({ "NOTE", "NORMAL ms", "Hz", "DOTTED ms", "Hz", "TRIPLET ms", "Hz" },
                      { 0.8f, 1.2f, 1.0f, 1.2f, 1.0f, 1.2f, 1.0f });
    summary.setLineHeight (22.0f);
    refresh();
}

void DelayModule::refresh()
{
    const auto b = bpm();
    if (! b.ok())
    {
        showErrors ({ str (b.error) });
        table.setRows ({});
        summary.setLines ({ { {}, "Enter a valid BPM", skin().ledRed } });
        swingOut.setLines ({ { "SWING", "--", skin().ledRed } });
        return;
    }
    const double v = *b.value;
    Validator val;
    const auto sw = val.num (swingField, "Swing", calc::kMinSwingPercent, calc::kMaxSwingPercent);
    showErrors (val.errors);
    if (sw)
    {
        std::vector<LedReadout::Line> lines;
        for (int d : { 8, 16 })
        {
            const auto t = calc::swing (v, d, *sw);
            const auto n = "1/" + juce::String (d);
            lines.push_back ({ n + " LONG / SHORT", str (calc::fmtMs (t.longMs)) + " / " + str (calc::fmtMs (t.shortMs)), skin().ledAmber });
            lines.push_back ({ n + " LATE BY", str (calc::fmtMs (t.lateMs)) + " ms", skin().ledGreen });
        }
        // Tempo-synced note values that fall inside the Haas window
        juce::StringArray haas;
        for (int d : calc::delayDenominators())
            if (const double ms = calc::noteMs (v, d); ms >= calc::kHaasMinMs && ms <= calc::kHaasMaxMs)
                haas.add ("1/" + juce::String (d) + " " + str (calc::fmt (ms, 1)));
        lines.push_back ({ "HAAS", str (calc::fmt (calc::kHaasMinMs, 0)) + " - " + str (calc::fmt (calc::kHaasMaxMs, 0)) + " ms", skin().ledAmber });
        lines.push_back ({ "IN SYNC", haas.isEmpty() ? juce::String ("--") : haas.joinIntoString ("  "), skin().ledAmber });
        swingOut.setLines (lines);
    }
    else
    {
        swingOut.setLines ({ { "SWING", "--", skin().ledRed } });
    }

    std::vector<juce::StringArray> rows;
    for (int d : calc::delayDenominators())
    {
        juce::StringArray row { "1/" + juce::String (d) };
        for (auto k : { calc::NoteKind::Normal, calc::NoteKind::Dotted, calc::NoteKind::Triplet })
        {
            const double ms = calc::noteMs (v, d, k);
            row.add (str (calc::fmtMs (ms)));
            row.add (str (calc::fmtHz (calc::msToHz (ms))));
        }
        rows.push_back (row);
    }
    table.setRows (rows);
    table.setRecommendedRow (2); // quarter note = the beat

    summary.setLines ({
        { "BPM", str (calc::fmt (v, 2)), skin().ledGreen, 1.2f },
        { "1 BEAT (1/4)", str (calc::fmtMs (calc::quarterMs (v))) + " ms" },
        { "1 BAR (4/4)", str (calc::fmtMs (calc::wholeNoteMs (v))) + " ms" },
        { "BEAT RATE", str (calc::fmtHz (v / 60.0)) + " Hz" },
        { "", "D = x1.5   T = x2/3", skin().screenText },
        { "", "Hz = LFO / mod rate", skin().screenText },
    });
}

void DelayModule::paint (juce::Graphics& g)
{
    ModuleBase::paint (g);
    drawSection (g, tableArea, "Note values 1/1 - 1/512");
    drawSection (g, summaryArea, "Tempo");
    drawSection (g, swingArea, "Swing / Haas");
}

void DelayModule::layoutContent (juce::Rectangle<int> r)
{
    summaryArea = r.removeFromRight (260);
    r.removeFromRight (10);
    tableArea = r;
    swingArea = summaryArea.removeFromBottom (summaryArea.getHeight() * 58 / 100);
    summaryArea.removeFromBottom (10);
    table.setBounds (tableArea.reduced (8).withTrimmedTop (22));
    summary.setBounds (summaryArea.reduced (8).withTrimmedTop (22));
    auto sw = swingArea.reduced (8).withTrimmedTop (22);
    swingField.setBounds (sw.removeFromTop (52));
    sw.removeFromTop (8);
    swingOut.setBounds (sw);
}

juce::String DelayModule::helpText() const
{
    return R"(# What it does
Lists every note value from 1/1 to 1/512 at the current tempo: straight, dotted and triplet, in milliseconds and in Hz.
# How to use
1. )" + tempoStep() + R"(
2. Find the note value you want. The amber row is 1/4, one beat.
3. Type the ms value into a delay, or the Hz value into an LFO rate. Click a value to copy it.
4. SWING: enter how much of each pair of notes the first one takes (50 % = straight, 75 % = hardest). LONG / SHORT are the two note lengths, LATE BY is how far the off-beat note moves.
5. HAAS lists the note values between 10 and 35 ms at this tempo, for widening a sound with a short delay on one side.
# Example
120 BPM: 1/8 = 250 ms, dotted 1/8 = 375 ms, triplet 1/8 = 166.7 ms. 60 % swing on 1/16: 150 / 100 ms, off-beats 25 ms late.
===
# Terms
- DOTTED (D): 1.5 x the note. A dotted 1/8 delay gives the classic galloping echo.
- TRIPLET (T): 2/3 of the note; three of them fit in the space of two.
- Hz: how many times per second the note repeats. Use it for LFO, tremolo, auto-pan or chorus rates.
- 1 BAR: four beats in 4/4.
- SWING: the first note of each pair is longer than the second. 66.7 % is a triplet shuffle.
- HAAS EFFECT: a copy delayed by 10 to 35 ms on one side sounds wider, not like an echo. Check the mix in mono.)";
}
