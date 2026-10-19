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

LoopModule::LoopModule (ModuleContext& c)
    : ModuleBase ("loop", "Loop / Bar Length", c),
      mode ({ "BARS > LENGTH", "LENGTH > BPM" }, Cap::Green, 601, false),
      lengthUnit ({ "SEC", "MS", "SAMPLES" }, Cap::White, 602, false)
{
    auto s = state();
    bars.bind (s, "bars", "4");
    beats.bind (s, "beats", "4");
    rate.bind (s, "rate", juce::String (ctx.hostSampleRate(), 0));
    length.bind (s, "length", "8");
    unitIndex = juce::jlimit (0, 2, (int) s.getProperty ("lengthUnit", 0));
    lengthUnit.setSelected (unitIndex, false);
    lengthUnitChanged (unitIndex);

    for (int u : { 2, 4, 8, 16 })
        beatUnit.addItem (juce::String (u), u);
    beatUnit.setSelectedId ((int) s.getProperty ("unit", 4), juce::dontSendNotification);
    beatUnit.onChange = [this] { state().setProperty ("unit", beatUnit.getSelectedId(), nullptr); refresh(); };

    for (auto* comp : std::initializer_list<juce::Component*> { &mode, &bars, &beats, &rate, &beatUnit, &hostRate, &out, &table })
        addAndMakeVisible (comp);
    for (auto* comp : std::initializer_list<juce::Component*> { &length, &lengthUnit, &useBpm })
        addChildComponent (comp);
    for (auto* f : { &bars, &beats, &length })
        f->onChange = [this] { refresh(); };
    // Typing a rate stops following the host; HOST SR (lit) follows it again.
    rate.onChange = [this] { hostRate.setToggleState (false, juce::dontSendNotification); state().setProperty ("rateHost", false, nullptr); refresh(); };
    hostRate.setToggleState ((bool) s.getProperty ("rateHost", true), juce::dontSendNotification);
    hostRate.onClick = [this] { state().setProperty ("rateHost", hostRate.getToggleState(), nullptr); refresh(); };
    hostRate.setTooltip ("Lit: follow the host / audio device sample rate");
    hostRate.setVisible (ctx.hasHost());

    mode.setTooltip ("BARS > LENGTH: how long bars last at the tempo. LENGTH > BPM: the tempo of a loop of known length");
    mode.onSelect = [this] (int i) { selectMode (i); };
    lengthUnit.setTooltip ("Unit of the loop length; switching converts the value");
    lengthUnit.onSelect = [this] (int i) { lengthUnitChanged (i); refresh(); };
    useBpm.setTooltip ("Set the tempo to this BPM");
    useBpm.onClick = [this]
    {
        if (foundBpm > 0)
            ctx.setBpmTextFor (getId(), str (calc::fmt (foundBpm, 2)));
    };

    out.setLineHeight (26.0f);
    selectMode ((int) s.getProperty ("mode", 0));
}

void LoopModule::selectMode (int m)
{
    m = juce::jlimit (0, 1, m);
    mode.setSelected (m, false);
    state().setProperty ("mode", m, nullptr);
    length.setVisible (m == 1);
    lengthUnit.setVisible (m == 1);
    useBpm.setVisible (m == 1);
    if (m == 0)
        table.setColumns ({ "BARS", "MS", "SECONDS", "SAMPLES", "M:SS.cc" }, { 0.55f, 1.3f, 1.0f, 1.4f, 1.25f });
    else
        table.setColumns ({ "BARS", "BPM", "ROUNDED", "MS AT ROUNDED" }, { 0.55f, 1.0f, 0.9f, 1.7f });
    resized();
    refresh();
    updateHelp();
    repaint();
}

void LoopModule::lengthUnitChanged (int newUnit)
{
    // Keep the same length in the new unit when the old value can be read.
    const auto p = calc::parseNumber (length.text(), "Loop length");
    const auto srP = calc::parseInRange (rate.text(), "Sample rate", 1000, 768000);
    if (p.ok() && *p.value > 0 && newUnit != unitIndex && (srP.ok() || (unitIndex != 2 && newUnit != 2)))
    {
        const double sr = srP.value.value_or (48000.0);
        const double ms = unitIndex == 0 ? *p.value * 1000.0 : unitIndex == 1 ? *p.value : calc::samplesToMs (*p.value, sr);
        const double v = newUnit == 0 ? ms / 1000.0 : newUnit == 1 ? ms : calc::msToSamples (ms, sr);
        auto t = str (calc::fmt (v, newUnit == 2 ? 0 : newUnit == 1 ? 3 : 5));
        if (t.containsChar ('.'))
            t = t.trimCharactersAtEnd ("0").trimCharactersAtEnd (".");
        length.setText (t);
        state().setProperty ("length", t, nullptr);
    }
    unitIndex = newUnit;
    state().setProperty ("lengthUnit", newUnit, nullptr);
    static const char* units[] = { "s", "ms", "smp" };
    length.setUnit (units[newUnit]);
}
void LoopModule::refresh()
{
    if (ctx.hasHost() && hostRate.getToggleState())
        rate.setText (juce::String (ctx.hostSampleRate(), 0));

    Validator v;
    const auto nBars = v.num (bars, "Bars", 0.0625, 4096);
    const auto nBeats = v.num (beats, "Beats / bar", 1, 64);
    const auto sr = v.num (rate, "Sample rate", 1000, 768000);
    const double unit = juce::jmax (1, beatUnit.getSelectedId());

    if (mode.getSelected() == 1)
        refreshBpm (v, nBars.value_or (0), nBeats.value_or (0), unit, sr);
    else
        refreshLength (v, bpm(), nBars.value_or (0), nBeats.value_or (0), unit, sr.value_or (0));
    showErrors (v.errors);
}

std::optional<double> LoopModule::lengthMs (Validator& v, std::optional<double> sr)
{
    switch (lengthUnit.getSelected())
    {
        case 1:
            return v.num (length, "Loop length", 0.01, 3.6e7);
        case 2:
        {
            const auto smp = v.num (length, "Loop length", 1, 1e11);
            if (smp && sr) return calc::samplesToMs (*smp, *sr);
            return std::nullopt;
        }
        default:
            if (const auto s = v.num (length, "Loop length", 0.00001, 36000)) return *s * 1000.0;
            return std::nullopt;
    }
}

void LoopModule::refreshLength (Validator& v, const calc::Parsed& b, double nBars, double nBeats, double unit, double sr)
{
    v.add (b);
    if (! v.ok())
    {
        out.setLines ({ { "LENGTH", "--", skin().ledRed, 1.6f } });
        table.setRows ({});
        return;
    }

    const double ms = calc::loopMs (*b.value, nBars, nBeats, unit);
    const double samples = calc::msToSamples (ms, sr);
    const bool fractional = std::abs (samples - std::round (samples)) > 1e-6;
    out.setLines ({
        { "SIGNATURE", juce::String (nBeats, 0) + "/" + juce::String ((int) unit) + "  @ " + str (calc::fmt (*b.value, 2)) + " BPM", skin().ledGreen },
        { "MILLISECONDS", str (calc::fmt (ms, 2)) + " ms", skin().ledAmber, 1.4f },
        { "SECONDS", str (calc::fmt (ms / 1000.0, 4)) + " s", skin().ledAmber, 1.4f },
        { "SAMPLES", str (calc::fmt (samples, 1)), skin().ledAmber, 1.4f },
        { "1 BAR", str (calc::fmtMs (calc::barMs (*b.value, nBeats, unit))) + " ms" },
        { "TOTAL BEATS", str (calc::fmt (nBars * nBeats, 2)) },
        { "", fractional ? "Sample count is fractional: loop will drift unless trimmed" : "Sample-accurate loop length",
          fractional ? skin().ledRed.withAlpha (0.85f) : skin().screenText },
    });

    std::vector<juce::StringArray> rows;
    int rec = -1, i = 0;
    for (double n : { 1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0 })
    {
        const double m = calc::loopMs (*b.value, n, nBeats, unit);
        if (std::abs (n - nBars) < 1e-9) rec = i;
        rows.push_back ({ juce::String ((int) n), str (calc::fmt (m, 1)), str (calc::fmt (m / 1000.0, 2)),
                          str (calc::fmt (calc::msToSamples (m, sr), 0)), minSec (m) });
        ++i;
    }
    table.setRows (rows);
    table.setRecommendedRow (rec);
}

void LoopModule::refreshBpm (Validator& v, double nBars, double nBeats, double unit, std::optional<double> sr)
{
    const auto ms = lengthMs (v, sr);
    foundBpm = 0;
    useBpm.setEnabled (false);
    if (! v.ok() || ! ms)
    {
        out.setLines ({ { "TEMPO", "--", skin().ledRed, 1.8f } });
        table.setRows ({});
        return;
    }

    const auto inRange = [] (double b) { return b >= calc::kMinBpm && b <= calc::kMaxBpm; };
    const auto lengthAt = [&] (double b, double n) { return calc::loopMs (b, n, nBeats, unit); };
    // Length of the loop at a rounded tempo and how far that is from the real length.
    const auto atRounded = [&] (double b, double n)
    {
        const double diff = lengthAt (b, n) - *ms;
        return str (calc::fmt (lengthAt (b, n), 1)) + " (" + (diff >= 0.05 ? "+" : "") + str (calc::fmt (diff, 1)) + ")";
    };

    const double found = calc::loopBpm (*ms, nBars, nBeats, unit);
    const juce::String sig = juce::String (nBars, std::abs (nBars - std::round (nBars)) < 1e-9 ? 0 : 2) + " bars of "
                           + juce::String (nBeats, 0) + "/" + juce::String ((int) unit);
    if (! inRange (found))
    {
        v.errors.add ("Tempo is outside 1 to 999 BPM: check the length and the number of bars");
        out.setLines ({ { "LOOP", sig, skin().ledGreen }, { "TEMPO", "OUT OF RANGE", skin().ledRed, 1.8f } });
    }
    else
    {
        foundBpm = found;
        useBpm.setEnabled (true);
        const double rounded = std::round (found);
        std::vector<LedReadout::Line> lines {
            { "LOOP", sig, skin().ledGreen },
            { "TEMPO", str (calc::fmt (found, 2)) + " BPM", skin().ledAmber, 1.8f },
            { "EXACT", str (calc::fmt (found, 4)) + " BPM" },
            { "LENGTH", str (calc::fmt (*ms / 1000.0, 4)) + " s" },
            { "HALF / DOUBLE", str (calc::fmt (found / 2.0, 2)) + " / " + str (calc::fmt (found * 2.0, 2)) },
        };
        if (sr)
            lines.insert (lines.begin() + 4, { "SAMPLES", str (calc::fmt (calc::msToSamples (*ms, *sr), 0)) });
        if (std::abs (lengthAt (rounded, nBars) - *ms) < 0.05) // shown as +0.0 ms
            lines.push_back ({ "", "Whole-number tempo: the loop sits on the grid", skin().screenText });
        else
            lines.push_back ({ "AT " + str (calc::fmt (rounded, 0)) + " BPM", atRounded (rounded, nBars) + " ms", skin().ledRed.withAlpha (0.85f) });
        out.setLines (lines);
    }

    std::vector<juce::StringArray> rows;
    int rec = -1, i = 0;
    for (double n : { 1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0 })
    {
        const double b = calc::loopBpm (*ms, n, nBeats, unit);
        if (std::abs (n - nBars) < 1e-9) rec = i;
        if (inRange (b) && inRange (std::round (b)))
            rows.push_back ({ juce::String ((int) n), str (calc::fmt (b, 2)), str (calc::fmt (std::round (b), 0)), atRounded (std::round (b), n) });
        else
            rows.push_back ({ juce::String ((int) n), "--", "--", "--" });
        ++i;
    }
    table.setRows (rows);
    table.setRecommendedRow (rec);
}

void LoopModule::paint (juce::Graphics& g)
{
    ModuleBase::paint (g);
    const bool toBpm = mode.getSelected() == 1;
    drawSection (g, inputArea, "Loop");
    drawSection (g, outArea, toBpm ? "Tempo" : "Length");
    drawSection (g, tableArea, toBpm ? "Tempo for other bar counts" : "Common lengths");

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
    mode.setBounds (row (32));
    if (mode.getSelected() == 1)
    {
        length.setBounds (row (52));
        lengthUnit.setBounds (row (28));
    }
    bars.setBounds (row (52));
    beats.setBounds (row (52));
    beatUnit.setBounds (row (52).withTrimmedTop (16));
    auto srRow = row (52);
    if (ctx.hasHost())
    {
        hostRate.setBounds (srRow.removeFromRight (74).withTrimmedTop (16));
        srRow.removeFromRight (6);
    }
    rate.setBounds (srRow);

    auto o = outArea.reduced (8).withTrimmedTop (22);
    if (mode.getSelected() == 1)
    {
        useBpm.setBounds (o.removeFromBottom (36));
        o.removeFromBottom (8);
    }
    out.setBounds (o);
    table.setBounds (tableArea.reduced (8).withTrimmedTop (22));
}

juce::String LoopModule::helpText() const
{
    const juce::String rateStep = ctx.hasHost() ? "HOST SR (lit) uses the session sample rate; type a rate to use another one."
                                                : "Type the sample rate of your project.";
    if (mode.getSelected() == 1)
        return R"(# What it does
Finds the tempo of a loop from its length and number of bars. Handy for loops from sample packs that do not say their BPM.
# How to use
1. Choose LENGTH > BPM.
2. Enter the loop length and pick its unit: SEC, MS or SAMPLES (your sampler or DAW shows one of these). Switching the unit converts the value.
3. Enter the number of bars and the time signature. )" + rateStep + R"(
4. Read the TEMPO. USE AS TEMPO sets it as the BPM. If the tempo is not a whole number, AT ... BPM shows how long the loop would be at the rounded tempo. The table lists the tempo for other bar counts; click a value to copy it.
# Example
A loop of 7.742 s that is 4 bars of 4/4 = 124.0 BPM. If you are not sure of the bar count, pick the row with a likely tempo (2 bars = 62 BPM, 8 bars = 248 BPM).
===
# Terms
- BAR: one measure of the time signature.
- TIME SIGNATURE: beats per bar / beat unit, for example 4/4 or 6/8.
- HALF / DOUBLE: the same loop felt at half or double time.
- SAMPLES: the length counted in samples at the sample rate.
- ROUNDED: the nearest whole-number tempo, and the loop length at it in ms (the difference in brackets).)";

    return R"(# What it does
Turns bars and a time signature into an exact length in milliseconds, seconds and samples, so loops and edits stay on the grid. LENGTH > BPM does the opposite: it finds the tempo of a loop.
# How to use
1. Enter the number of bars and the beats per bar, then pick the beat unit (4 = quarter notes, 8 = eighth notes).
2. )" + rateStep + R"(
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
