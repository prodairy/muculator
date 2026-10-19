#include "Modules.h"

using namespace ui;

namespace
{
juce::String minSec (double ms)
{
    const auto totalCs = (juce::int64) std::llround (ms / 10.0);
    const auto m = totalCs / 6000, s = (totalCs / 100) % 60, cs = totalCs % 100;
    return juce::String (m) + ":" + juce::String (s).paddedLeft ('0', 2) + "." + juce::String (cs).paddedLeft ('0', 2);
}
} // namespace

LoopModule::LoopModule (ModuleContext& c) : ModuleBase ("loop", "Loop / Bar Length", c)
{
    auto s = state();
    bars.bind (s, "bars", "4");
    beats.bind (s, "beats", "4");
    rate.bind (s, "rate", juce::String (ctx.hostSampleRate(), 0));

    for (int u : { 2, 4, 8, 16 })
        beatUnit.addItem (juce::String (u), u);
    beatUnit.setSelectedId ((int) s.getProperty ("unit", 4), juce::dontSendNotification);
    beatUnit.onChange = [this] { state().setProperty ("unit", beatUnit.getSelectedId(), nullptr); refresh(); };

    for (auto* comp : std::initializer_list<juce::Component*> { &bars, &beats, &rate, &beatUnit, &hostRate, &out, &table })
        addAndMakeVisible (comp);
    for (auto* f : { &bars, &beats })
        f->onChange = [this] { refresh(); };
    // Typing a rate stops following the host; HOST SR (lit) follows it again.
    rate.onChange = [this] { hostRate.setToggleState (false, juce::dontSendNotification); state().setProperty ("rateHost", false, nullptr); refresh(); };
    hostRate.setToggleState ((bool) s.getProperty ("rateHost", true), juce::dontSendNotification);
    hostRate.onClick = [this] { state().setProperty ("rateHost", hostRate.getToggleState(), nullptr); refresh(); };
    hostRate.setTooltip ("Lit: follow the host / audio device sample rate");

    out.setLineHeight (26.0f);
    table.setColumns ({ "BARS", "MS", "SECONDS", "SAMPLES", "M:SS.cc" }, { 0.55f, 1.3f, 1.0f, 1.4f, 1.25f });
    refresh();
}

void LoopModule::refresh()
{
    if (hostRate.getToggleState())
        rate.setText (juce::String (ctx.hostSampleRate(), 0));

    Validator v;
    const auto b = bpm();
    v.add (b);
    const auto nBars = v.num (bars, "Bars", 0.0625, 4096);
    const auto nBeats = v.num (beats, "Beats / bar", 1, 64);
    const auto sr = v.num (rate, "Sample rate", 1000, 768000);
    const double unit = juce::jmax (1, beatUnit.getSelectedId());
    showErrors (v.errors);

    if (! v.ok())
    {
        out.setLines ({ { "LENGTH", "--", skin().ledRed, 1.6f } });
        table.setRows ({});
        return;
    }

    const double ms = calc::loopMs (*b.value, *nBars, *nBeats, unit);
    const double samples = calc::msToSamples (ms, *sr);
    out.setLines ({
        { "SIGNATURE", juce::String (*nBeats, 0) + "/" + juce::String ((int) unit) + "  @ " + str (calc::fmt (*b.value, 2)) + " BPM", skin().ledGreen },
        { "MILLISECONDS", str (calc::fmt (ms, 2)) + " ms", skin().ledAmber, 1.4f },
        { "SECONDS", str (calc::fmt (ms / 1000.0, 4)) + " s", skin().ledAmber, 1.4f },
        { "SAMPLES", str (calc::fmt (samples, 1)), skin().ledAmber, 1.4f },
        { "1 BAR", str (calc::fmtMs (calc::barMs (*b.value, *nBeats, unit))) + " ms" },
        { "TOTAL BEATS", str (calc::fmt (*nBars * *nBeats, 2)) },
        { "", std::abs (samples - std::round (samples)) > 1e-6 ? "Sample count is fractional: loop will drift unless trimmed"
                                                               : "Sample-accurate loop length", std::abs (samples - std::round (samples)) > 1e-6 ? skin().ledRed.withAlpha (0.85f) : skin().screenText },
    });

    std::vector<juce::StringArray> rows;
    int rec = -1, i = 0;
    for (double n : { 1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0 })
    {
        const double m = calc::loopMs (*b.value, n, *nBeats, unit);
        if (std::abs (n - *nBars) < 1e-9) rec = i;
        rows.push_back ({ juce::String ((int) n), str (calc::fmt (m, 1)), str (calc::fmt (m / 1000.0, 2)),
                          str (calc::fmt (calc::msToSamples (m, *sr), 0)), minSec (m) });
        ++i;
    }
    table.setRows (rows);
    table.setRecommendedRow (rec);
}

void LoopModule::paint (juce::Graphics& g)
{
    ModuleBase::paint (g);
    drawSection (g, inputArea, "Loop");
    drawSection (g, outArea, "Length");
    drawSection (g, tableArea, "Common lengths");

    if (! beatUnit.getBounds().isEmpty())
    {
        g.setFont (labelFont (10.5f));
        g.setColour (skin().text.withAlpha (0.85f));
        g.drawText ("BEAT UNIT", beatUnit.getBounds().translated (0, -16).withHeight (15), juce::Justification::centredLeft);
    }
}

void LoopModule::layoutContent (juce::Rectangle<int> r)
{
    inputArea = r.removeFromLeft (230);
    r.removeFromLeft (10);
    outArea = r.removeFromLeft (r.getWidth() * 40 / 100);
    r.removeFromLeft (10);
    tableArea = r;

    auto in = inputArea.reduced (10).withTrimmedTop (22);
    const auto row = [&in] (int h) { auto a = in.removeFromTop (h); in.removeFromTop (10); return a; };
    bars.setBounds (row (52));
    beats.setBounds (row (52));
    beatUnit.setBounds (row (52).withTrimmedTop (16));
    auto srRow = row (52);
    hostRate.setBounds (srRow.removeFromRight (74).withTrimmedTop (16));
    srRow.removeFromRight (6);
    rate.setBounds (srRow);

    out.setBounds (outArea.reduced (8).withTrimmedTop (22));
    table.setBounds (tableArea.reduced (8).withTrimmedTop (22));
}

juce::String LoopModule::helpText() const
{
    return R"(# What it does
Turns bars and a time signature into an exact length in milliseconds, seconds and samples, so loops and edits stay on the grid.
# How to use
1. Enter the number of bars and the beats per bar, then pick the beat unit (4 = quarter notes, 8 = eighth notes).
2. HOST SR (lit) uses the session sample rate; type a rate to use another one.
3. Read the length. The table lists common bar counts; click a value to copy it.
A red line means the sample count is not a whole number, so the loop will drift unless you trim it.
# Example
4 bars of 4/4 at 120 BPM = 8000 ms = 384000 samples at 48 kHz.
===
# Terms
- TIME SIGNATURE: beats per bar / beat unit, for example 4/4 or 6/8.
- BAR: one measure of the time signature.
- SAMPLE-ACCURATE: a whole number of samples, so the loop repeats without drifting.
- M:SS.cc: minutes, seconds and hundredths of a second.)";
}
