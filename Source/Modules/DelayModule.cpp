#include "Modules.h"

using namespace ui;

DelayModule::DelayModule (ModuleContext& c) : ModuleBase ("delay", "Delay Time", c)
{
    addAndMakeVisible (table);
    addAndMakeVisible (summary);
    table.setColumns ({ "NOTE", "NORMAL ms", "Hz", "DOTTED ms", "Hz", "TRIPLET ms", "Hz" },
                      { 0.8f, 1.2f, 1.0f, 1.2f, 1.0f, 1.2f, 1.0f });
    summary.setLineHeight (24.0f);
    refresh();
}

void DelayModule::refresh()
{
    const auto b = bpm();
    if (! b.ok())
    {
        showErrors ({ str (b.error) });
        table.setRows ({});
        summary.setLines ({ { {}, "Enter a valid BPM", col::ledRed } });
        return;
    }
    showErrors ({});
    const double v = *b.value;

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
        { "BPM", str (calc::fmt (v, 2)), col::ledGreen, 1.2f },
        { "1 BEAT (1/4)", str (calc::fmtMs (calc::quarterMs (v))) + " ms" },
        { "1 BAR (4/4)", str (calc::fmtMs (calc::wholeNoteMs (v))) + " ms" },
        { "BEAT RATE", str (calc::fmtHz (v / 60.0)) + " Hz" },
        { "", "D = x1.5   T = x2/3", col::silkDim },
        { "", "Hz = LFO / mod rate", col::silkDim },
    });
}

void DelayModule::paint (juce::Graphics& g)
{
    ModuleBase::paint (g);
    drawSection (g, tableArea, "Note values 1/1 - 1/512");
    drawSection (g, summaryArea, "Tempo");
}

void DelayModule::layoutContent (juce::Rectangle<int> r)
{
    summaryArea = r.removeFromRight (220);
    r.removeFromRight (10);
    tableArea = r;
    table.setBounds (tableArea.reduced (8).withTrimmedTop (22));
    summary.setBounds (summaryArea.reduced (8).withTrimmedTop (22));
}
