#include "Modules.h"

using namespace ui;

namespace
{
/** Number for an input field: up to maxDecimals, trailing zeros stripped. */
juce::String clean (double v, int maxDecimals)
{
    auto s = str (calc::fmt (v, maxDecimals));
    if (s.containsChar ('.'))
        s = s.trimCharactersAtEnd ("0").trimCharactersAtEnd (".");
    return s == "-0" ? "0" : s;
}

/** Closest straight/dotted/triplet note value to a duration, for context. */
juce::String nearestNoteValue (double ms, double bpm)
{
    juce::String best;
    double bestErr = 1e9;
    for (int d : calc::delayDenominators())
        for (auto k : { calc::NoteKind::Normal, calc::NoteKind::Dotted, calc::NoteKind::Triplet })
        {
            const double err = std::abs (std::log (ms / calc::noteMs (bpm, d, k)));
            if (err < bestErr) { bestErr = err; best = str (calc::noteValueLabel (d, k)); }
        }
    return bestErr < 0.03 ? best : "~" + best;
}
} // namespace

ConverterModule::ConverterModule (ModuleContext& c) : ModuleBase ("convert", "Converters", c)
{
    auto s = state();
    dbField.bind (s, "db", "-6");
    gainField.bind (s, "gain", "");
    eqCentre.bind (s, "eqCentre", "1000");
    eqQ.bind (s, "eqQ", "1.41");
    eqOct.bind (s, "eqOct", "");
    tMs.bind (s, "tMs", "250");
    tHz.bind (s, "tHz", "");
    tSamples.bind (s, "tSamples", "");
    tRate.bind (s, "tRate", clean (ctx.hostSampleRate(), 0));
    dMetres.bind (s, "dMetres", "3");
    dMs.bind (s, "dMs", "");
    dTemp.bind (s, "dTemp", "20");
    fsDuration.bind (s, "fsDuration", "180");
    fsRate.bind (s, "fsRate", clean (ctx.hostSampleRate(), 0));
    fsChannels.bind (s, "fsChannels", "2");
    fsBits.setSelected (juce::jlimit (0, 2, (int) s.getProperty ("fsBits", 1)), false);

    for (auto* comp : std::initializer_list<juce::Component*> {
             &dbField, &gainField, &gainOut, &eqCentre, &eqQ, &eqOct, &eqOut,
             &tMs, &tHz, &tSamples, &tRate, &tHostRate, &tOut, &dMetres, &dMs, &dTemp, &dOut,
             &fsDuration, &fsRate, &fsChannels, &fsBits, &fsOut })
        addAndMakeVisible (comp);

    dbField.onChange   = [this] { updateGain (Src::A); publishErrors(); };
    gainField.onChange = [this] { updateGain (Src::B); publishErrors(); };
    eqCentre.onChange  = [this] { updateEq (Src::A); publishErrors(); };
    eqQ.onChange       = [this] { updateEq (Src::B); publishErrors(); };
    eqOct.onChange     = [this] { updateEq (Src::C); publishErrors(); };
    tMs.onChange       = [this] { updateTime (Src::A); publishErrors(); };
    tHz.onChange       = [this] { updateTime (Src::B); publishErrors(); };
    tSamples.onChange  = [this] { updateTime (Src::C); publishErrors(); };
    tRate.onChange     = [this]
    {
        // Typing a rate stops following the host; HOST SR (lit) follows it again.
        tHostRate.setToggleState (false, juce::dontSendNotification);
        state().setProperty ("tRateHost", false, nullptr);
        updateTime (Src::None);
        publishErrors();
    };
    dMetres.onChange   = [this] { updateDistance (Src::A); publishErrors(); };
    dMs.onChange       = [this] { updateDistance (Src::B); publishErrors(); };
    dTemp.onChange     = [this] { updateDistance (Src::None); publishErrors(); };
    tHostRate.setToggleState ((bool) s.getProperty ("tRateHost", true), juce::dontSendNotification);
    tHostRate.onClick  = [this]
    {
        state().setProperty ("tRateHost", tHostRate.getToggleState(), nullptr);
        updateTime (Src::None);
        publishErrors();
    };
    tHostRate.setTooltip ("Lit: follow the host / audio device sample rate");
    for (auto* f : { &fsDuration, &fsRate, &fsChannels })
        f->onChange = [this] { updateFileSize(); publishErrors(); };
    fsBits.onSelect = [this] (int i) { state().setProperty ("fsBits", i, nullptr); updateFileSize(); publishErrors(); };
    fsBits.setTooltip ("Bit depth: 16 (CD), 24 (recording), 32 (float)");
    fsDuration.setTooltip ("Length of the audio in seconds (3:00 = 180 s)");
    tHostRate.setVisible (ctx.hasHost());

    for (auto* r : { &gainOut, &eqOut, &tOut, &dOut, &fsOut })
        r->setLineHeight (20.0f);

    updateGain (Src::A);
    updateEq (Src::B);
    updateTime (Src::A);
    updateDistance (Src::A);
    updateFileSize();
    publishErrors();
}

void ConverterModule::refresh()
{
    // Time and distance depend on BPM / sample rate; keep the other panels as typed.
    updateTime (Src::None);
    updateDistance (Src::None);
    publishErrors();
}

void ConverterModule::publishErrors()
{
    juce::StringArray all;
    all.addArray (gainErr); all.addArray (eqErr); all.addArray (timeErr); all.addArray (distErr); all.addArray (sizeErr);
    showErrors (all);
}

void ConverterModule::updateGain (Src src)
{
    Validator v;
    double db = 0;
    gainField.setInvalid (false);
    dbField.setInvalid (false);
    if (src == Src::B)
    {
        auto p = calc::parseNumber (gainField.text(), "Gain");
        if (p.ok() && *p.value <= 0)
            p = { std::nullopt, *p.value >= 0 ? "Gain 0 = -inf dB (silence)" : "Gain must be greater than 0" };
        else if (p.ok() && *p.value > 1e6)
            p = { std::nullopt, "Gain is too large" };
        if (auto g = v.read (gainField, p))
        {
            db = calc::gainToDb (*g);
            dbField.setText (clean (db, 3));
        }
    }
    else if (auto d = v.num (dbField, "dB", -200, 200))
    {
        db = *d;
        gainField.setText (clean (calc::dbToGain (db), 6));
    }

    gainErr = v.errors;
    if (! v.ok()) { gainOut.setLines ({ { "", "--", skin().ledRed } }); return; }
    const double g = calc::dbToGain (db);
    gainOut.setLines ({
        { "AMPLITUDE", "x" + str (calc::fmt (g, 4)) + "  (" + str (calc::fmt (g * 100.0, 2)) + " %)", skin().ledGreen },
        { "POWER", "x" + str (calc::fmt (g * g, 4)) },
        { "", "+6 dB = x2   -6 dB = x0.5   +20 dB = x10", skin().screenText },
    });
}

void ConverterModule::updateEq (Src src)
{
    Validator v;
    const auto centre = v.num (eqCentre, "Centre", 1, 30000);
    std::optional<double> q;
    eqQ.setInvalid (false);
    eqOct.setInvalid (false);
    if (src == Src::C)
    {
        if (auto oct = v.num (eqOct, "Bandwidth", 0.01, 10))
        {
            q = calc::octavesToQ (*oct);
            eqQ.setText (clean (*q, 4));
        }
    }
    else if ((q = v.num (eqQ, "Q", 0.05, 100)))
        eqOct.setText (clean (calc::qToOctaves (*q), 4));

    eqErr = v.errors;
    if (! v.ok()) { eqOut.setLines ({ { "", "--", skin().ledRed } }); return; }
    const auto band = calc::bandForQ (*centre, *q);
    eqOut.setLines ({
        { "BANDWIDTH", str (calc::fmtHz (band.widthHz)) + " Hz", skin().ledGreen },
        { "LOW -3 dB", str (calc::fmtHz (band.lowHz)) + " Hz" },
        { "HIGH -3 dB", str (calc::fmtHz (band.highHz)) + " Hz" },
    });
}

void ConverterModule::updateTime (Src src)
{
    if (ctx.hasHost() && tHostRate.getToggleState())
        tRate.setText (clean (ctx.hostSampleRate(), 0));

    Validator v;
    const auto sr = v.num (tRate, "Sample rate", 1000, 768000);
    for (auto* f : { &tMs, &tHz, &tSamples }) f->setInvalid (false);

    std::optional<double> ms;
    if (src == Src::B)
    {
        if (auto hz = v.num (tHz, "Frequency", 0.0001, 1e6))
        {
            ms = calc::hzToMs (*hz);
            tMs.setText (clean (*ms, 4));
        }
    }
    else if (src == Src::C)
    {
        if (auto smp = v.num (tSamples, "Samples", 0.001, 1e9); smp && sr)
        {
            ms = calc::samplesToMs (*smp, *sr);
            tMs.setText (clean (*ms, 4));
            tHz.setText (clean (calc::msToHz (*ms), 4));
        }
    }
    else
    {
        ms = v.num (tMs, "Time", 0.0001, 1e7);
    }

    if (ms && src != Src::B && src != Src::C)
        tHz.setText (clean (calc::msToHz (*ms), 4));
    if (ms && sr && src != Src::C)
        tSamples.setText (clean (calc::msToSamples (*ms, *sr), 2));
    if (ms && src == Src::B && ! sr)
        tSamples.setText ({});

    timeErr = v.errors;
    if (! v.ok() || ! ms) { tOut.setLines ({ { "", "--", skin().ledRed } }); return; }

    std::vector<LedReadout::Line> lines {
        { "SECONDS", str (calc::fmt (*ms / 1000.0, 5)) + " s", skin().ledGreen },
        { "SAMPLES", str (calc::fmt (calc::msToSamples (*ms, *sr), 2)) + " @ " + str (calc::fmt (*sr, 0)) },
    };
    if (const auto b = bpm(); b.ok())
        lines.push_back ({ "@ " + str (calc::fmt (*b.value, 1)) + " BPM", nearestNoteValue (*ms, *b.value) });
    tOut.setLines (lines);
}

void ConverterModule::updateDistance (Src src)
{
    Validator v;
    const auto temp = v.num (dTemp, "Air temp", -40, 60);
    dMetres.setInvalid (false);
    dMs.setInvalid (false);
    std::optional<double> metres;
    if (src == Src::B)
    {
        if (auto ms = v.num (dMs, "Delay", 0, 1e6); ms && temp)
        {
            metres = calc::msToMetres (*ms, *temp);
            dMetres.setText (clean (*metres, 4));
        }
    }
    else if ((metres = v.num (dMetres, "Distance", 0, 100000)) && temp)
        dMs.setText (clean (calc::metresToMs (*metres, *temp), 4));

    distErr = v.errors;
    if (! v.ok() || ! metres) { dOut.setLines ({ { "", "--", skin().ledRed } }); return; }
    const double ms = calc::metresToMs (*metres, *temp);
    const double sr = ctx.hostSampleRate();
    dOut.setLines ({
        { "FEET", str (calc::fmt (*metres * 3.28084, 3)) + " ft", skin().ledGreen },
        { "SAMPLES", str (calc::fmt (calc::msToSamples (ms, sr), 1)) + " @ " + str (calc::fmt (sr, 0)) },
        { "SPEED OF SOUND", str (calc::fmt (calc::speedOfSound (*temp), 1)) + " m/s" },
    });
}

void ConverterModule::updateFileSize()
{
    Validator v;
    const auto seconds = v.num (fsDuration, "Duration", 0.001, 1e7);
    const auto sr = v.num (fsRate, "Sample rate", 1000, 768000);
    const auto ch = v.num (fsChannels, "Channels", 1, 64);
    sizeErr = v.errors;
    if (! v.ok()) { fsOut.setLines ({ { "", "--", skin().ledRed } }); return; }

    static const int depths[] = { 16, 24, 32 };
    const int bits = depths[juce::jlimit (0, 2, fsBits.getSelected())];
    const int channels = (int) std::round (*ch);
    const double bytes = calc::audioBytes (*seconds, *sr, bits, channels);
    const double perMinute = calc::audioBytes (60.0, *sr, bits, channels);
    fsOut.setLines ({
        { "SIZE", str (calc::fmt (bytes / 1e6, 2)) + " MB", skin().ledGreen },
        { "BINARY", str (calc::fmt (bytes / 1048576.0, 2)) + " MiB" },
        { "PER MINUTE", str (calc::fmt (perMinute / 1e6, 2)) + " MB" },
        { "DATA RATE", str (calc::fmt (perMinute / 60.0 * 8.0 / 1000.0, 0)) + " kbit/s" },
    });
}

void ConverterModule::paint (juce::Graphics& g)
{
    ModuleBase::paint (g);
    drawSection (g, cells[0], "dB  <>  linear gain");
    drawSection (g, cells[1], "EQ  Q  <>  octaves  <>  Hz");
    drawSection (g, cells[2], "ms  <>  Hz  <>  samples");
    drawSection (g, cells[3], "Distance  <>  ms  (mic / speaker)");
    drawSection (g, cells[4], "Audio file size (WAV / AIFF)");
}

void ConverterModule::layoutContent (juce::Rectangle<int> r)
{
    auto top = r.removeFromTop ((r.getHeight() - 10) / 2);
    r.removeFromTop (10);
    cells[0] = top.removeFromLeft ((top.getWidth() - 10) / 2);
    top.removeFromLeft (10);
    cells[1] = top;
    const int third = (r.getWidth() - 20) / 3;
    cells[2] = r.removeFromLeft (third);
    r.removeFromLeft (10);
    cells[3] = r.removeFromLeft (third);
    r.removeFromLeft (10);
    cells[4] = r;

    const auto layout = [] (juce::Rectangle<int> cell, std::initializer_list<juce::Component*> fields, LedReadout& out)
    {
        auto a = cell.reduced (10).withTrimmedTop (22);
        auto row = a.removeFromTop (52);
        const int n = (int) fields.size();
        const int w = (row.getWidth() - (n - 1) * 8) / n;
        for (auto* f : fields) { f->setBounds (row.removeFromLeft (w)); row.removeFromLeft (8); }
        a.removeFromTop (8);
        out.setBounds (a);
    };
    layout (cells[0], { &dbField, &gainField }, gainOut);
    layout (cells[1], { &eqCentre, &eqQ, &eqOct }, eqOut);
    layout (cells[2], { &tMs, &tHz, &tSamples }, tOut);
    layout (cells[3], { &dMetres, &dMs, &dTemp }, dOut);
    layout (cells[4], { &fsDuration, &fsRate, &fsChannels }, fsOut);
    {
        auto a = fsOut.getBounds();
        fsBits.setBounds (a.removeFromTop (28));
        a.removeFromTop (6);
        fsOut.setBounds (a);
    }

    // sample rate row lives inside the time readout area's top
    auto outArea = tOut.getBounds();
    auto srRow = outArea.removeFromTop (52);
    if (ctx.hasHost())
    {
        tHostRate.setBounds (srRow.removeFromRight (78).withTrimmedTop (16));
        srRow.removeFromRight (8);
    }
    tRate.setBounds (srRow);
    outArea.removeFromTop (6);
    tOut.setBounds (outArea);
}

juce::String ConverterModule::helpText() const
{
    return R"(# What it does
Five converters in one place. Type in any field of a panel and the other fields of that panel follow; the file size panel works out the size of an audio file.
# How to use
- dB <> LINEAR GAIN: a level change as a multiplier.
- EQ: Q <> bandwidth in octaves, with the -3 dB edges in Hz.
- ms <> Hz <> SAMPLES: times at a sample rate. )" + juce::String (ctx.hasHost() ? "HOST SR (lit) follows the session; typing a rate turns it off."
                                                                              : "Type the sample rate of your project.") + R"(
- DISTANCE <> ms: how long sound takes to travel, for lining up mics and speakers.
- FILE SIZE: how big uncompressed audio gets, from its duration in seconds, sample rate, channels and bit depth.
Click a value to copy it.
# Example
-6 dB = x0.501. Q 1.41 is about 1 octave. 10 ms = 480 samples at 48 kHz. A mic 3 m further away hears the source 8.7 ms later (20 C). A 3-minute stereo song at 48 kHz / 24 bit is 51.84 MB.
===
# Terms
- dB: a ratio on a log scale. +6 dB is about twice the amplitude, +20 dB is ten times.
- Q: how narrow an EQ band is. Higher Q = narrower band. Q 1.41 is about one octave wide.
- BANDWIDTH: the width between the two -3 dB points, in octaves.
- SAMPLE RATE: samples per second (44100, 48000, 96000...).
- SPEED OF SOUND: about 343 m/s at 20 C, a little faster in warm air.
- BIT DEPTH: bits per sample. 16 = CD, 24 = recording, 32 = floating point.
- MB / MiB: 1 MB = 1,000,000 bytes; 1 MiB = 1,048,576 bytes (some systems call this MB). The file header adds a few bytes.)";
}
