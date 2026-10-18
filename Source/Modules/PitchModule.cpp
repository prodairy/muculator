#include "Modules.h"

using namespace ui;

namespace
{
juce::String noteStr (int midi) { return str (calc::noteName (midi)); }

juce::String noteWithCents (const calc::NoteInfo& n)
{
    return noteStr (n.nearestMidi) + "  " + str (calc::fmtCents (n.cents));
}

juce::Colour centsColour (double cents)
{
    const double a = std::abs (cents);
    return a < 0.05 ? col::ledGreen : a < calc::kMicrotoneThresholdCents - 0.05 ? col::ledAmber : col::ledRed;
}
} // namespace

PitchModule::PitchModule (ModuleContext& c)
    : ModuleBase ("pitch", "Pitch", c),
      tabs ({ "TRANSPOSE", "STRETCH", "FREQ > NOTE", "HARMONICS" }, col::capWhite, 301, false),
      stMode ({ "NOTE", "FREQ" }, col::capGreen, 302, false),
      hmMode ({ "NOTE", "FREQ" }, col::capGreen, 303, false)
{
    headerBadge = "A4 = " + str (calc::fmt (calc::kA4Hz, 2)) + " Hz";

    auto s = state();
    tpNote.bind (s, "tpNote", 60);
    tpSemis.bind (s, "tpSemis", "7");
    tpCents.bind (s, "tpCents", "0");
    stNote.bind (s, "stNote", 69);
    stFreq.bind (s, "stFreq", "440");
    stOldBpm.bind (s, "stOldBpm", "120");
    fnFreq.bind (s, "fnFreq", "432");
    hmNote.bind (s, "hmNote", 45);
    hmFreq.bind (s, "hmFreq", "110");
    stMode.setSelected ((int) s.getProperty ("stMode", 0), false);
    hmMode.setSelected ((int) s.getProperty ("hmMode", 0), false);
    stNewBpm.setText (ctx.bpmTextFor (getId()));

    for (auto* comp : std::initializer_list<juce::Component*> {
             &tabs, &tpNote, &tpSemis, &tpCents, &tpOut, &stMode, &stNote, &stFreq, &stOldBpm, &stNewBpm,
             &stOut, &fnFreq, &fnOut, &hmMode, &hmNote, &hmFreq, &hmTable })
        addChildComponent (comp);
    addChildComponent (microtone);
    tabs.setVisible (true);

    for (auto* f : { &tpSemis, &tpCents, &stFreq, &stOldBpm, &fnFreq, &hmFreq })
        f->onChange = [this] { refresh(); };
    stNewBpm.onChange = [this] { ctx.setBpmTextFor (getId(), stNewBpm.getText()); }; // editor calls refresh()
    for (auto* n : { &tpNote, &stNote, &hmNote })
        n->onChange = [this] { refresh(); };
    stMode.onSelect = [this] (int i) { state().setProperty ("stMode", i, nullptr); updateVisibility(); refresh(); };
    hmMode.onSelect = [this] (int i) { state().setProperty ("hmMode", i, nullptr); updateVisibility(); refresh(); };

    stNewBpm.setFontHeight (20.0f);
    stOut.setLineHeight (24.0f);
    fnOut.setLineHeight (24.0f);
    tpOut.setLineHeight (24.0f);
    hmTable.setColumns ({ "#", "FREQ Hz", "NOTE", "CENTS", "NOTE Hz" }, { 0.5f, 1.2f, 0.8f, 1.0f, 1.2f });

    tab = (int) s.getProperty ("tab", 1);
    tabs.setSelected (tab, false);
    tabs.onSelect = [this] (int i) { selectTab (i); };
    updateVisibility();
    refresh();
}

void PitchModule::selectTab (int t)
{
    tab = juce::jlimit (0, 3, t);
    tabs.setSelected (tab, false);
    state().setProperty ("tab", tab, nullptr);
    updateVisibility();
    resized();
    refresh();
    repaint();
}

void PitchModule::updateVisibility()
{
    tpNote.setVisible (tab == 0); tpSemis.setVisible (tab == 0); tpCents.setVisible (tab == 0); tpOut.setVisible (tab == 0);

    const bool st = tab == 1, stByNote = stMode.getSelected() == 0;
    stMode.setVisible (st); stNote.setVisible (st && stByNote); stFreq.setVisible (st && ! stByNote);
    stOldBpm.setVisible (st); stNewBpm.setVisible (st); stOut.setVisible (st);
    if (! st) microtone.setVisible (false);

    fnFreq.setVisible (tab == 2); fnOut.setVisible (tab == 2);

    const bool hm = tab == 3, hmByNote = hmMode.getSelected() == 0;
    hmMode.setVisible (hm); hmNote.setVisible (hm && hmByNote); hmFreq.setVisible (hm && ! hmByNote); hmTable.setVisible (hm);
}

void PitchModule::refresh()
{
    if (! stNewBpm.isEditing())
        stNewBpm.setText (ctx.bpmTextFor (getId()));
    stNewBpm.setLocked (false, ctx.isChained() ? "LINKED" : "");

    Validator v;
    switch (tab)
    {
        case 0: refreshTranspose (v); break;
        case 1: refreshStretch (v); break;
        case 2: refreshFreq (v); break;
        default: refreshHarmonics (v); break;
    }
    showErrors (v.errors);
}

void PitchModule::refreshTranspose (Validator& v)
{
    const auto semis = v.num (tpSemis, "Semitones", -96, 96);
    const auto cents = v.num (tpCents, "Cents", -100, 100);
    const int src = tpNote.getMidi();
    const double srcHz = calc::midiToFreq (src);
    if (! v.ok())
    {
        tpOut.setLines ({ { "SOURCE", noteStr (src) + "  " + str (calc::fmtHz (srcHz)) + " Hz" },
                          { "RESULT", "--", col::ledRed, 1.8f } });
        return;
    }
    const auto r = calc::transpose (srcHz, *semis, *cents);
    if (r.target.nearestMidi < 0 || r.target.nearestMidi > 131)
    {
        v.errors.add ("Result is outside the MIDI note range (C-1 .. B9)");
        tpOut.setLines ({ { "RESULT", "OUT OF RANGE", col::ledRed, 1.8f } });
        return;
    }
    tpOut.setLines ({
        { "SOURCE", noteStr (src) + "  " + str (calc::fmtHz (srcHz)) + " Hz" },
        { "SHIFT", str (calc::fmt (*semis, 2)) + " st  " + str (calc::fmtCents (*cents)) },
        { "NEW NOTE", noteStr (r.target.nearestMidi), col::ledGreen, 2.0f },
        { "DEVIATION", str (calc::fmtCents (r.target.cents)), centsColour (r.target.cents) },
        { "NEW FREQUENCY", str (calc::fmtHz (r.targetFreq)) + " Hz", col::ledAmber, 1.3f },
        { "RATIO", "x" + str (calc::fmt (r.targetFreq / srcHz, 5)) },
    });
}

void PitchModule::refreshStretch (Validator& v)
{
    double oldHz = 0;
    if (stMode.getSelected() == 0)
        oldHz = calc::midiToFreq (stNote.getMidi());
    else if (auto f = v.num (stFreq, "Old frequency", 1, 30000))
        oldHz = *f;

    const auto oldBpm = v.bpmField (stOldBpm, "Old BPM");
    const auto newBpm = bpm();
    stNewBpm.setInvalid (! newBpm.ok());
    v.add (newBpm);

    if (! v.ok())
    {
        stOut.setLines ({ { "NEW NOTE", "--", col::ledRed, 2.2f }, { "NEW FREQUENCY", "--", col::ledRed } });
        microtone.setVisible (false);
        return;
    }

    const auto r = calc::stretch (*oldBpm, oldHz, *newBpm.value);
    const auto oldInfo = calc::analyse (oldHz);
    if (r.note.nearestMidi < 0 || r.note.nearestMidi > 131)
    {
        v.errors.add ("Result is outside the MIDI note range (C-1 .. B9)");
        stOut.setLines ({ { "NEW NOTE", "OUT OF RANGE", col::ledRed, 2.0f } });
        microtone.setVisible (false);
        return;
    }

    stOut.setLines ({
        { "OLD", noteWithCents (oldInfo) + "  " + str (calc::fmtHz (oldHz)) + " Hz", col::ledAmber.withAlpha (0.75f) },
        { "NEW NOTE", noteStr (r.note.nearestMidi), r.microtone ? col::ledRed : col::ledGreen, 2.2f },
        { "DEVIATION", str (calc::fmtCents (r.note.cents)), centsColour (r.note.cents), 1.1f },
        { "NEW FREQUENCY", str (calc::fmtHz (r.newFreq)) + " Hz", col::ledAmber, 1.3f },
        { "PITCH SHIFT", (r.semitones >= 0 ? "+" : "") + str (calc::fmt (r.semitones, 2)) + " st" },
        { "SPEED", "x" + str (calc::fmt (r.ratio, 4)) + "  (" + str (calc::fmt (r.ratio * 100.0, 2)) + " %)" },
    });

    if (r.microtone)
    {
        const double fromLower = (r.note.exactMidi - r.lowerMidi) * 100.0;
        const double fromUpper = (r.note.exactMidi - r.upperMidi) * 100.0;
        const bool lowerIsNearest = r.note.nearestMidi == r.lowerMidi;
        const auto bpmTxt = [] (double b) { return str (calc::fmt (b, 2)) + " BPM"; };
        microtone.setMessage (
            "MICROTONE WARNING:  between " + noteStr (r.lowerMidi) + " and " + noteStr (r.upperMidi),
            str (calc::fmtHz (r.newFreq)) + " Hz is " + str (calc::fmtCents (fromLower)) + " from " + noteStr (r.lowerMidi)
                + " and " + str (calc::fmtCents (fromUpper)) + " from " + noteStr (r.upperMidi) + ".\n"
                + "Nearest note " + noteStr (r.note.nearestMidi) + " -> set NEW BPM to " + bpmTxt (r.bpmForNearest) + "\n"
                + "Other neighbour " + noteStr (lowerIsNearest ? r.upperMidi : r.lowerMidi) + " -> "
                + bpmTxt (lowerIsNearest ? r.bpmForUpper : r.bpmForLower),
            WarningPanel::Level::Microtone);
    }
    else
    {
        microtone.setMessage ("IN TUNE", "Within +/-" + str (calc::fmt (calc::kMicrotoneThresholdCents, 0)) + " ct of "
                                             + noteStr (r.note.nearestMidi) + ". Exact at " + str (calc::fmt (r.bpmForNearest, 2)) + " BPM.",
                              WarningPanel::Level::Info);
    }
    microtone.setVisible (true);
}

void PitchModule::refreshFreq (Validator& v)
{
    const auto f = v.num (fnFreq, "Frequency", 1, 30000);
    if (! v.ok())
    {
        fnOut.setLines ({ { "NOTE", "--", col::ledRed, 2.2f } });
        return;
    }
    const auto n = calc::analyse (*f);
    const double sr = ctx.hostSampleRate();
    fnOut.setLines ({
        { "NEAREST NOTE", noteStr (n.nearestMidi), centsColour (n.cents) == col::ledRed ? col::ledAmber : col::ledGreen, 2.2f },
        { "DEVIATION", str (calc::fmtCents (n.cents)), centsColour (n.cents), 1.1f },
        { "NOTE FREQUENCY", str (calc::fmtHz (n.nearestFreq)) + " Hz" },
        { "MIDI (exact)", str (calc::fmt (n.exactMidi, 3)) },
        { "PERIOD", str (calc::fmtMs (1000.0 / *f)) + " ms" },
        { "WAVELENGTH (20C)", str (calc::fmt (calc::speedOfSound (20) / *f, 3)) + " m" },
        { "CYCLE @ " + str (calc::fmt (sr, 0)), str (calc::fmt (sr / *f, 2)) + " samples" },
    });
}

void PitchModule::refreshHarmonics (Validator& v)
{
    double f0 = 0;
    if (hmMode.getSelected() == 0)
        f0 = calc::midiToFreq (hmNote.getMidi());
    else if (auto f = v.num (hmFreq, "Fundamental", 1, 20000))
        f0 = *f;

    if (! v.ok())
    {
        hmTable.setRows ({});
        return;
    }
    std::vector<juce::StringArray> rows;
    const auto hs = calc::harmonics (f0, 16);
    for (auto& h : hs)
        rows.push_back ({ juce::String (h.n), str (calc::fmtHz (h.freq)), noteStr (h.note.nearestMidi),
                          str (calc::fmtCents (h.note.cents)), str (calc::fmtHz (h.note.nearestFreq)) });
    hmTable.setRows (rows);
    hmTable.setRecommendedRow (0);
    for (int i = 0; i < (int) hs.size(); ++i)
        if (std::abs (hs[(size_t) i].note.cents) >= calc::kMicrotoneThresholdCents)
            hmTable.setRowColour (i, col::ledRed.withAlpha (0.85f));
}

void PitchModule::paint (juce::Graphics& g)
{
    ModuleBase::paint (g);
    static const char* inTitles[] = { "Transpose", "Stretch input", "Frequency", "Fundamental" };
    static const char* outTitles[] = { "Result", "Stretched sample", "Note", "Harmonic series (red = >10 ct off)" };
    drawSection (g, inputArea, inTitles[tab]);
    drawSection (g, outputArea, outTitles[tab]);

    if (tab == 1)
    {
        auto hint = stNewBpm.getBounds().translated (0, stNewBpm.getHeight() + 4).withHeight (34);
        g.setColour (col::silkDim);
        g.setFont (labelFont (10.0f, false));
        g.drawFittedText ("NEW BPM = top bar BPM" + juce::String (ctx.isChained() ? " (chained)" : " for this module")
                              + ". Speed change = pitch change (varispeed).",
                          hint, juce::Justification::topLeft, 3);
    }
}

void PitchModule::layoutContent (juce::Rectangle<int> r)
{
    tabs.setBounds (r.removeFromTop (38).withTrimmedRight (r.getWidth() / 3));
    r.removeFromTop (10);
    inputArea = r.removeFromLeft (270);
    r.removeFromLeft (10);
    outputArea = r;

    auto in = inputArea.reduced (10).withTrimmedTop (22);
    auto out = outputArea.reduced (8).withTrimmedTop (22);
    const auto row = [&in] (int h) { auto a = in.removeFromTop (h); in.removeFromTop (10); return a; };

    switch (tab)
    {
        case 0:
            tpNote.setBounds (row (52));
            tpSemis.setBounds (row (52));
            tpCents.setBounds (row (52));
            tpOut.setBounds (out);
            break;
        case 1:
        {
            stMode.setBounds (row (32));
            auto noteRow = row (52);
            stNote.setBounds (noteRow);
            stFreq.setBounds (noteRow);
            stOldBpm.setBounds (row (52));
            stNewBpm.setBounds (row (58));
            microtone.setBounds (out.removeFromBottom (128));
            out.removeFromBottom (8);
            stOut.setBounds (out);
            break;
        }
        case 2:
            fnFreq.setBounds (row (52));
            fnOut.setBounds (out);
            break;
        default:
        {
            hmMode.setBounds (row (32));
            auto noteRow = row (52);
            hmNote.setBounds (noteRow);
            hmFreq.setBounds (noteRow);
            hmTable.setBounds (out);
            break;
        }
    }
}
