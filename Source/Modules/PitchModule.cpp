#include "Modules.h"

using namespace ui;

namespace
{
juce::Colour centsColour (double cents)
{
    const double a = std::abs (cents);
    return a < 0.05 ? skin().ledGreen : a < calc::kMicrotoneThresholdCents - 0.05 ? skin().ledAmber : skin().ledRed;
}
} // namespace

PitchModule::PitchModule (ModuleContext& c)
    : ModuleBase ("pitch", "Pitch", c),
      tabs ({ "TRANSPOSE", "STRETCH", "FREQ > NOTE", "HARMONICS", "FREQ MAP" }, Cap::White, 301, false),
      stKind ({ "VARISPEED", "TIME-STRETCH" }, Cap::Yellow, 308, false),
      stMode ({ "NOTE", "FREQ" }, Cap::Green, 302, false),
      stScale ({ "MAJOR", "MINOR" }, Cap::White, 309, false),
      hmMode ({ "NOTE", "FREQ" }, Cap::Green, 303, false),
      hmSet ({ "ALL", "ODD", "EVEN" }, Cap::White, 304, false),
      hmWave ({ "OFF", "SAW", "SQUARE", "TRI" }, Cap::Yellow, 305, false),
      hmCount ({ "16", "32", "64" }, Cap::White, 306, false),
      spelling ({ "#", "b" }, Cap::White, 307, false),
      fmMode ({ "NOTE", "FREQ" }, Cap::Green, 310, false)
{
    auto s = state();
    a4Field.bind (s, "a4", "440");
    spelling.setSelected ((bool) s.getProperty ("flats", false) ? 1 : 0, false);
    tpNote.bind (s, "tpNote", 60);
    tpSemis.bind (s, "tpSemis", "7");
    tpCents.bind (s, "tpCents", "0");
    stNote.bind (s, "stNote", 69);
    stFreq.bind (s, "stFreq", "440");
    stOldBpm.bind (s, "stOldBpm", "120");
    stLength.bind (s, "stLength", "8");
    fmNote.bind (s, "fmNote", 45);
    fmFreq.bind (s, "fmFreq", "110");
    fnFreq.bind (s, "fnFreq", "432");
    hmNote.bind (s, "hmNote", 45);
    hmFreq.bind (s, "hmFreq", "110");
    stMode.setSelected ((int) s.getProperty ("stMode", 0), false);
    stKind.setSelected ((int) s.getProperty ("stKind", 0), false);
    stScale.setSelected ((int) s.getProperty ("stScale", 1), false);
    fmMode.setSelected ((int) s.getProperty ("fmMode", 0), false);
    for (int pc = 0; pc < 12; ++pc)
        stKey.addItem (str (calc::pitchClassName (pc)), pc + 1);
    stKey.setSelectedId (juce::jlimit (0, 11, (int) s.getProperty ("stKey", 9)) + 1, juce::dontSendNotification);
    hmMode.setSelected ((int) s.getProperty ("hmMode", 0), false);
    hmSet.setSelected ((int) s.getProperty ("hmSet", 0), false);
    hmWave.setSelected ((int) s.getProperty ("hmWave", 0), false);
    hmCount.setSelected ((int) s.getProperty ("hmCount", 0), false);
    stNewBpm.setText (ctx.bpmTextFor (getId()));

    for (auto* comp : std::initializer_list<juce::Component*> {
             &tabs, &tpNote, &tpSemis, &tpCents, &tpOut, &stMode, &stNote, &stFreq, &stOldBpm, &stNewBpm,
             &stOut, &fnFreq, &fnOut, &hmMode, &hmSet, &hmWave, &hmCount, &hmNote, &hmFreq, &hmTable,
             &stKind, &stLength, &stKey, &stScale, &stFit, &fmMode, &fmNote, &fmFreq, &fmOut, &fmMap })
        addChildComponent (comp);
    addChildComponent (microtone);
    for (auto* comp : std::initializer_list<juce::Component*> { &tabs, &a4Field, &a4Presets, &spelling })
        addAndMakeVisible (comp);

    a4Field.onChange = [this] { refresh(); };
    a4Field.setTooltip ("Tuning reference: the frequency of A4 (400 to 480 Hz)");
    a4Presets.setTooltip ("Common references");
    a4Presets.onClick = [this]
    {
        juce::PopupMenu m;
        for (double hz : { 432.0, 440.0, 442.0, 443.0 })
            m.addItem (juce::String (hz, 0) + " Hz", [this, hz] { a4Field.setText (juce::String (hz, 0)); refresh(); });
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (a4Presets));
    };
    spelling.setTooltip ("Write black keys with sharps (C#) or flats (Db)");
    spelling.onSelect = [this] (int i) { state().setProperty ("flats", i == 1, nullptr); refresh(); };

    for (auto* f : { &tpSemis, &tpCents, &stFreq, &stOldBpm, &stLength, &fnFreq, &hmFreq, &fmFreq })
        f->onChange = [this] { refresh(); };
    stNewBpm.onChange = [this] { ctx.setBpmTextFor (getId(), stNewBpm.getText()); }; // editor calls refresh()
    for (auto* n : { &tpNote, &stNote, &hmNote, &fmNote })
        n->onChange = [this] { refresh(); };
    stKind.setTooltip ("VARISPEED: pitch follows the speed, like tape. TIME-STRETCH: the pitch stays");
    stKind.onSelect = [this] (int i)
    {
        state().setProperty ("stKind", i, nullptr);
        updateVisibility();
        resized();
        refresh();
        updateHelp();
        repaint();
    };
    stScale.setTooltip ("Scale of the song for FIT TO SCALE");
    stScale.onSelect = [this] (int i) { state().setProperty ("stScale", i, nullptr); refresh(); };
    stKey.setTooltip ("Key of the song for FIT TO SCALE");
    stKey.onChange = [this] { state().setProperty ("stKey", stKey.getSelectedId() - 1, nullptr); refresh(); };
    stFit.setTooltip ("Tempos that land exactly on a note of the scale; click a value to copy it");
    stFit.setColumns ({ "NEW BPM", "NOTE", "SHIFT st" }, { 1.1f, 0.8f, 0.9f });
    fmMode.onSelect = [this] (int i) { state().setProperty ("fmMode", i, nullptr); updateVisibility(); refresh(); };
    fmMap.setTooltip ("Click to move the marker");
    fmMap.onClick = [this] (double hz)
    {
        fmMode.setSelected (1, false);
        state().setProperty ("fmMode", 1, nullptr);
        fmFreq.setText (str (calc::fmt (hz, hz < 100 ? 1 : 0)));
        updateVisibility();
        refresh();
    };
    stMode.onSelect = [this] (int i) { state().setProperty ("stMode", i, nullptr); updateVisibility(); refresh(); };
    hmMode.onSelect = [this] (int i) { state().setProperty ("hmMode", i, nullptr); updateVisibility(); refresh(); };
    hmSet.onSelect = [this] (int i) { state().setProperty ("hmSet", i, nullptr); refresh(); };
    hmCount.onSelect = [this] (int i) { state().setProperty ("hmCount", i, nullptr); refresh(); };
    hmWave.onSelect = [this] (int i)
    {
        // A waveform also picks the harmonics it contains (saw: all, square / triangle: odd).
        state().setProperty ("hmWave", i, nullptr);
        if (i > 0)
        {
            const int set = calc::harmonicSetFor ((calc::Waveform) (i - 1)) == calc::HarmonicSet::All ? 0 : 1;
            hmSet.setSelected (set, false);
            state().setProperty ("hmSet", set, nullptr);
        }
        refresh();
    };
    hmSet.setTooltip ("Which harmonics to list: all, odd only (1, 3, 5...) or even only (2, 4, 6...)");
    hmWave.setTooltip ("Adds the ideal level of each harmonic for a saw, square or triangle wave");
    hmCount.setTooltip ("How many harmonics to list (scroll the table for more)");

    stNewBpm.setFontHeight (20.0f);
    stOut.setLineHeight (24.0f);
    fnOut.setLineHeight (24.0f);
    fmOut.setLineHeight (22.0f);
    tpOut.setLineHeight (24.0f);
    hmTable.setColumns ({ "#", "FREQ Hz", "NOTE", "CENTS", "NOTE Hz" }, { 0.5f, 1.2f, 0.8f, 1.0f, 1.2f });

    tab = juce::jlimit (0, 4, (int) s.getProperty ("tab", 1));
    tabs.setSelected (tab, false);
    tabs.onSelect = [this] (int i) { selectTab (i); };
    updateVisibility();
    refresh();
}

void PitchModule::selectTab (int t)
{
    tab = juce::jlimit (0, 4, t);
    tabs.setSelected (tab, false);
    state().setProperty ("tab", tab, nullptr);
    updateVisibility();
    resized();
    refresh();
    updateHelp();
    repaint();
}

void PitchModule::updateVisibility()
{
    tpNote.setVisible (tab == 0); tpSemis.setVisible (tab == 0); tpCents.setVisible (tab == 0); tpOut.setVisible (tab == 0);

    const bool st = tab == 1, stByNote = stMode.getSelected() == 0, vs = st && ! timeStretchMode();
    stKind.setVisible (st);
    stMode.setVisible (vs); stNote.setVisible (vs && stByNote); stFreq.setVisible (vs && ! stByNote);
    stLength.setVisible (st && timeStretchMode());
    stKey.setVisible (vs); stScale.setVisible (vs); stFit.setVisible (vs);
    stOldBpm.setVisible (st); stNewBpm.setVisible (st); stOut.setVisible (st);
    if (! st) microtone.setVisible (false);

    fnFreq.setVisible (tab == 2); fnOut.setVisible (tab == 2);

    const bool hm = tab == 3, hmByNote = hmMode.getSelected() == 0;
    hmMode.setVisible (hm); hmNote.setVisible (hm && hmByNote); hmFreq.setVisible (hm && ! hmByNote); hmTable.setVisible (hm);
    hmSet.setVisible (hm); hmWave.setVisible (hm); hmCount.setVisible (hm);

    const bool fm = tab == 4, fmByNote = fmMode.getSelected() == 0;
    fmMode.setVisible (fm); fmNote.setVisible (fm && fmByNote); fmFreq.setVisible (fm && ! fmByNote);
    fmOut.setVisible (fm); fmMap.setVisible (fm);
}

juce::String PitchModule::noteText (int midi) const { return str (calc::noteName (midi, flats)); }

juce::String PitchModule::noteWithCents (const calc::NoteInfo& n) const
{
    return noteText (n.nearestMidi) + "  " + str (calc::fmtCents (n.cents));
}

void PitchModule::refresh()
{
    flats = spelling.getSelected() == 1;
    for (auto* n : { &tpNote, &stNote, &hmNote, &fmNote })
        n->setFlats (flats);
    for (int pc = 0; pc < 12; ++pc)
        stKey.changeItemText (pc + 1, str (calc::pitchClassName (pc, flats)));

    if (! stNewBpm.isEditing())
        stNewBpm.setText (ctx.bpmTextFor (getId()));
    stNewBpm.setLocked (false, ctx.isChained() ? "LINKED" : "");

    Validator v;
    const auto ref = v.num (a4Field, "A4 reference", calc::kMinA4Hz, calc::kMaxA4Hz);
    a4 = ref.value_or (calc::kA4Hz);
    const auto badge = ref ? "A4 = " + str (calc::fmt (a4, 2)) + " Hz" : juce::String ("A4 = -- Hz");
    if (badge != headerBadge)
    {
        headerBadge = badge;
        repaint();
    }

    switch (tab)
    {
        case 0: refreshTranspose (v); break;
        case 1: refreshStretch (v); break;
        case 2: refreshFreq (v); break;
        case 3: refreshHarmonics (v); break;
        default: refreshMap (v); break;
    }
    showErrors (v.errors);
}

void PitchModule::refreshTranspose (Validator& v)
{
    const auto semis = v.num (tpSemis, "Semitones", -96, 96);
    const auto cents = v.num (tpCents, "Cents", -100, 100);
    const int src = tpNote.getMidi();
    const double srcHz = calc::midiToFreq (src, a4);
    if (! v.ok())
    {
        tpOut.setLines ({ { "SOURCE", noteText (src) + "  " + str (calc::fmtHz (srcHz)) + " Hz" },
                          { "RESULT", "--", skin().ledRed, 1.8f } });
        return;
    }
    const auto r = calc::transpose (srcHz, *semis, *cents, a4);
    if (r.target.nearestMidi < 0 || r.target.nearestMidi > 131)
    {
        v.errors.add ("Result is outside the MIDI note range (C-1 .. B9)");
        tpOut.setLines ({ { "RESULT", "OUT OF RANGE", skin().ledRed, 1.8f } });
        return;
    }
    tpOut.setLines ({
        { "SOURCE", noteText (src) + "  " + str (calc::fmtHz (srcHz)) + " Hz" },
        { "SHIFT", str (calc::fmt (*semis, 2)) + " st  " + str (calc::fmtCents (*cents)) },
        { "NEW NOTE", noteText (r.target.nearestMidi), skin().ledGreen, 2.0f },
        { "DEVIATION", str (calc::fmtCents (r.target.cents)), centsColour (r.target.cents) },
        { "NEW FREQUENCY", str (calc::fmtHz (r.targetFreq)) + " Hz", skin().ledAmber, 1.3f },
        { "RATIO", "x" + str (calc::fmt (r.targetFreq / srcHz, 5)) },
    });
}

void PitchModule::refreshStretch (Validator& v)
{
    if (timeStretchMode())
    {
        const auto oldBpm = v.bpmField (stOldBpm, "Old BPM");
        const auto newBpm = bpm();
        stNewBpm.setInvalid (! newBpm.ok());
        v.add (newBpm);
        refreshTimeStretch (v, oldBpm.value_or (0), newBpm.value.value_or (0));
        return;
    }

    double oldHz = 0;
    if (stMode.getSelected() == 0)
        oldHz = calc::midiToFreq (stNote.getMidi(), a4);
    else if (auto f = v.num (stFreq, "Old frequency", 1, 30000))
        oldHz = *f;

    const auto oldBpm = v.bpmField (stOldBpm, "Old BPM");
    const auto newBpm = bpm();
    stNewBpm.setInvalid (! newBpm.ok());
    v.add (newBpm);

    if (! v.ok())
    {
        stOut.setLines ({ { "NEW NOTE", "--", skin().ledRed, 2.2f }, { "NEW FREQUENCY", "--", skin().ledRed } });
        stFit.setRows ({});
        microtone.setVisible (false);
        return;
    }
    refreshFit (*oldBpm, oldHz, *newBpm.value);

    const auto r = calc::stretch (*oldBpm, oldHz, *newBpm.value, calc::kMicrotoneThresholdCents, a4);
    const auto oldInfo = calc::analyse (oldHz, a4);
    if (r.note.nearestMidi < 0 || r.note.nearestMidi > 131)
    {
        v.errors.add ("Result is outside the MIDI note range (C-1 .. B9)");
        stOut.setLines ({ { "NEW NOTE", "OUT OF RANGE", skin().ledRed, 2.0f } });
        microtone.setVisible (false);
        return;
    }

    stOut.setLines ({
        { "OLD", noteWithCents (oldInfo), skin().ledAmber.withAlpha (0.75f) },
        { "OLD FREQUENCY", str (calc::fmtHz (oldHz)) + " Hz", skin().ledAmber.withAlpha (0.75f) },
        { "NEW NOTE", noteText (r.note.nearestMidi), r.microtone ? skin().ledRed : skin().ledGreen, 2.2f },
        { "DEVIATION", str (calc::fmtCents (r.note.cents)), centsColour (r.note.cents), 1.1f },
        { "NEW FREQUENCY", str (calc::fmtHz (r.newFreq)) + " Hz", skin().ledAmber, 1.3f },
        { "PITCH SHIFT", (r.semitones >= 0 ? "+" : "") + str (calc::fmt (r.semitones, 2)) + " st" },
        { "SPEED", "x" + str (calc::fmt (r.ratio, 4)) + " (" + str (calc::fmt (r.ratio * 100.0, 2)) + " %)" },
        { "IN KEY", calc::inScale (r.note.nearestMidi, stKey.getSelectedId() - 1, (calc::Scale) stScale.getSelected())
                        && ! r.microtone ? "YES" : "NO",
          calc::inScale (r.note.nearestMidi, stKey.getSelectedId() - 1, (calc::Scale) stScale.getSelected()) && ! r.microtone
              ? skin().ledGreen : skin().ledRed },
    });

    if (r.microtone)
    {
        const double fromLower = (r.note.exactMidi - r.lowerMidi) * 100.0;
        const double fromUpper = (r.note.exactMidi - r.upperMidi) * 100.0;
        const bool lowerIsNearest = r.note.nearestMidi == r.lowerMidi;
        const auto bpmTxt = [] (double b) { return str (calc::fmt (b, 2)) + " BPM"; };
        microtone.setMessage (
            "MICROTONE WARNING:  between " + noteText (r.lowerMidi) + " and " + noteText (r.upperMidi),
            str (calc::fmtHz (r.newFreq)) + " Hz is " + str (calc::fmtCents (fromLower)) + " from " + noteText (r.lowerMidi)
                + " and " + str (calc::fmtCents (fromUpper)) + " from " + noteText (r.upperMidi) + ".\n"
                + "Nearest note " + noteText (r.note.nearestMidi) + " -> set NEW BPM to " + bpmTxt (r.bpmForNearest) + "\n"
                + "Other neighbour " + noteText (lowerIsNearest ? r.upperMidi : r.lowerMidi) + " -> "
                + bpmTxt (lowerIsNearest ? r.bpmForUpper : r.bpmForLower),
            WarningPanel::Level::Microtone);
    }
    else
    {
        microtone.setMessage ("IN TUNE", "Within +/-" + str (calc::fmt (calc::kMicrotoneThresholdCents, 0)) + " ct of "
                                             + noteText (r.note.nearestMidi) + ". Exact at " + str (calc::fmt (r.bpmForNearest, 2)) + " BPM.",
                              WarningPanel::Level::Info);
    }
    microtone.setVisible (true);
}

void PitchModule::refreshTimeStretch (Validator& v, double oldBpm, double newBpm)
{
    const auto lengthS = v.num (stLength, "Old length", 0.001, 36000);
    if (! v.ok())
    {
        stOut.setLines ({ { "NEW LENGTH", "--", skin().ledRed, 2.0f } });
        microtone.setVisible (false);
        return;
    }

    const auto r = calc::timeStretch (oldBpm, newBpm, *lengthS * 1000.0);
    const double sr = ctx.hostSampleRate();
    const auto pct = [] (double p) { return (p >= 0.005 ? "+" : "") + str (calc::fmt (p, 2)) + " %"; };
    stOut.setLines ({
        { "TEMPO", str (calc::fmt (oldBpm, 2)) + " > " + str (calc::fmt (newBpm, 2)) + " BPM", skin().ledAmber.withAlpha (0.75f) },
        { "NEW LENGTH", str (calc::fmt (r.newLengthMs / 1000.0, 4)) + " s", skin().ledGreen, 2.0f },
        { "NEW LENGTH ms", str (calc::fmt (r.newLengthMs, 2)) + " ms", skin().ledAmber, 1.3f },
        { "SAMPLES @ " + str (calc::fmt (sr, 0)), str (calc::fmt (calc::msToSamples (r.newLengthMs, sr), 0)) },
        { "TEMPO CHANGE", pct (r.percent), r.quality == calc::StretchQuality::Clean ? skin().ledGreen
                                          : r.quality == calc::StretchQuality::Audible ? skin().ledAmber : skin().ledRed, 1.1f },
        { "STRETCH RATIO", "x" + str (calc::fmt (r.lengthRatio, 4)) + " (" + str (calc::fmt (r.lengthRatio * 100.0, 2)) + " %)" },
        { "PITCH", "unchanged", skin().ledGreen },
    });

    const auto pctTxt = str (calc::fmt (calc::kStretchCleanPercent, 0)) + " %";
    if (r.quality == calc::StretchQuality::Clean)
        microtone.setMessage ("CLEAN STRETCH", "A tempo change of up to " + pctTxt + " usually sounds natural with any good stretch algorithm.",
                              WarningPanel::Level::Info);
    else if (r.quality == calc::StretchQuality::Audible)
        microtone.setMessage ("AUDIBLE ON SOME MATERIAL",
                              "Between " + pctTxt + " and " + str (calc::fmt (calc::kStretchWarnPercent, 0)) + " % drums can lose punch and vocals can sound phasey.\n"
                              "Pick the stretch mode that matches the material (beats, tonal, complex).",
                              WarningPanel::Level::Microtone);
    else
        microtone.setMessage ("QUALITY WARNING:  extreme stretch",
                              "More than " + str (calc::fmt (calc::kStretchWarnPercent, 0)) + " % smears transients and adds metallic artefacts.\n"
                              "Try half or double time, re-record at the new tempo, or use VARISPEED if the pitch may change.",
                              WarningPanel::Level::Error);
    microtone.setVisible (true);
}

void PitchModule::refreshFit (double oldBpm, double oldHz, double newBpm)
{
    const int key = stKey.getSelectedId() - 1;
    const auto fits = calc::scaleFits (oldBpm, oldHz, newBpm, key, (calc::Scale) stScale.getSelected(), 12, a4);
    std::vector<juce::StringArray> rows;
    int nearest = -1;
    double best = 1e9;
    for (size_t i = 0; i < fits.size(); ++i)
    {
        const auto& f = fits[i];
        rows.push_back ({ str (calc::fmt (f.bpm, 2)), noteText (f.midi), (f.semitones >= 0.005 ? "+" : "") + str (calc::fmt (f.semitones, 2)) });
        if (const double d = std::abs (std::log (f.bpm / newBpm)); d < best) { best = d; nearest = (int) i; }
    }
    stFit.setRows (rows);
    stFit.setRecommendedRow (nearest);
}

void PitchModule::refreshFreq (Validator& v)
{
    const auto f = v.num (fnFreq, "Frequency", 1, 30000);
    if (! v.ok())
    {
        fnOut.setLines ({ { "NOTE", "--", skin().ledRed, 2.2f } });
        return;
    }
    const auto n = calc::analyse (*f, a4);
    const double sr = ctx.hostSampleRate();
    fnOut.setLines ({
        { "NEAREST NOTE", noteText (n.nearestMidi), centsColour (n.cents) == skin().ledRed ? skin().ledAmber : skin().ledGreen, 2.2f },
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
        f0 = calc::midiToFreq (hmNote.getMidi(), a4);
    else if (auto f = v.num (hmFreq, "Fundamental", 1, 20000))
        f0 = *f;

    const int wave = hmWave.getSelected(); // 0 = off
    if (wave > 0)
        hmTable.setColumns ({ "#", "TYPE", "FREQ Hz", "NOTE", "CENTS", "NOTE Hz", "LEVEL dB" },
                            { 0.45f, 0.7f, 1.15f, 0.75f, 0.95f, 1.15f, 1.0f });
    else
        hmTable.setColumns ({ "#", "TYPE", "FREQ Hz", "NOTE", "CENTS", "NOTE Hz" },
                            { 0.45f, 0.7f, 1.2f, 0.8f, 1.0f, 1.2f });

    if (! v.ok())
    {
        hmTable.setRows ({});
        return;
    }
    static const calc::HarmonicSet sets[] = { calc::HarmonicSet::All, calc::HarmonicSet::Odd, calc::HarmonicSet::Even };
    static const int counts[] = { 16, 32, 64 };
    std::vector<juce::StringArray> rows;
    const auto hs = calc::harmonics (f0, counts[juce::jlimit (0, 2, hmCount.getSelected())], sets[juce::jlimit (0, 2, hmSet.getSelected())], a4);
    for (auto& h : hs)
    {
        juce::StringArray row { juce::String (h.n), h.n == 1 ? "FUND" : h.n % 2 == 1 ? "ODD" : "EVEN",
                                str (calc::fmtHz (h.freq)), noteText (h.note.nearestMidi),
                                str (calc::fmtCents (h.note.cents)), str (calc::fmtHz (h.note.nearestFreq)) };
        if (wave > 0)
        {
            const double level = calc::harmonicLevel ((calc::Waveform) (wave - 1), h.n);
            row.add (level > 0 ? str (calc::fmt (calc::gainToDb (level), 1)) : juce::String ("--"));
        }
        rows.push_back (row);
    }
    hmTable.setRows (rows);
    hmTable.setRecommendedRow (! hs.empty() && hs.front().n == 1 ? 0 : -1);
    for (int i = 0; i < (int) hs.size(); ++i)
        if (std::abs (hs[(size_t) i].note.cents) >= calc::kMicrotoneThresholdCents)
            hmTable.setRowColour (i, skin().ledRed.withAlpha (0.85f));
}

void PitchModule::refreshMap (Validator& v)
{
    double hz = 0;
    if (fmMode.getSelected() == 0)
        hz = calc::midiToFreq (fmNote.getMidi(), a4);
    else if (auto f = v.num (fmFreq, "Marker frequency", 20, 20000))
        hz = *f;

    if (! v.ok() || hz < 20 || hz > 20000)
    {
        if (v.ok())
            v.errors.add ("Marker note is outside 20 Hz to 20 kHz");
        fmMap.setMarker (0);
        fmOut.setLines ({ { "MARKER", "--", skin().ledRed, 1.3f } });
        return;
    }
    fmMap.setMarker (hz);

    int fund = 0, zone = 0;
    for (const auto& i : calc::instrumentRanges())
    {
        fund += hz >= i.fundLowHz && hz <= i.fundHighHz;
        zone += hz >= i.zoneLowHz && hz <= i.zoneHighHz;
    }
    const auto n = calc::analyse (hz, a4);
    fmOut.setLines ({
        { "MARKER", str (calc::fmtHz (hz)) + " Hz", skin().ledAmber, 1.3f },
        { "NOTE", noteWithCents (n) },
        { "FUNDAMENTAL OF", juce::String (fund) + (fund == 1 ? " instrument" : " instruments"), fund > 0 ? skin().ledAmber : skin().screenText },
        { "KEY ZONE OF", juce::String (zone) + (zone == 1 ? " instrument" : " instruments"), zone > 0 ? skin().ledGreen : skin().screenText },
    });
}

void PitchModule::paint (juce::Graphics& g)
{
    ModuleBase::paint (g);
    static const char* inTitles[] = { "Transpose", "Stretch input", "Frequency", "Fundamental", "Marker" };
    static const char* outTitles[] = { "Result", "Stretched sample", "Note", "Harmonic series (red = >10 ct off)",
                                       "Instruments (amber = fundamental, green = key zone)" };
    drawSection (g, inputArea, inTitles[tab]);
    drawSection (g, outputArea, outTitles[tab]);

    g.setFont (labelFont (10.5f));
    g.setColour (skin().text.withAlpha (0.85f));
    g.drawText ("NOTE NAMES", spelling.getBounds().translated (0, -16).withHeight (15), juce::Justification::centredLeft);
    g.setColour (skin().textDim.withAlpha (0.4f));
    g.fillRect (inputArea.getX() + 10, a4Field.getY() - 8, inputArea.getWidth() - 20, 1);

    if (tab == 1)
    {
        auto hint = stNewBpm.getBounds().translated (0, stNewBpm.getHeight() + 4).withHeight (34);
        g.setColour (skin().textDim);
        g.setFont (labelFont (10.0f, false));
        g.drawFittedText ("NEW BPM = top bar BPM" + juce::String (ctx.isChained() ? " (chained)" : " for this module")
                              + (timeStretchMode() ? ". Length changes, pitch stays (time-stretch)." : ". Speed change = pitch change (varispeed)."),
                          hint, juce::Justification::topLeft, 3);
        if (! timeStretchMode())
        {
            g.setFont (labelFont (10.5f));
            g.setColour (skin().text.withAlpha (0.85f));
            g.drawText ("FIT TO SCALE: KEY / SCALE", fitArea.withHeight (15), juce::Justification::centredLeft);
        }
    }
    else if (tab == 3)
    {
        g.setFont (labelFont (10.5f));
        g.setColour (skin().text.withAlpha (0.85f));
        for (auto [group, caption] : { std::pair { &hmSet, "SERIES" }, std::pair { &hmWave, "WAVEFORM LEVELS" }, std::pair { &hmCount, "COUNT" } })
            g.drawText (caption, group->getBounds().translated (0, -16).withHeight (15), juce::Justification::centredLeft);
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

    // tuning row at the bottom of the input panel, the same on every tab
    auto tuning = in.removeFromBottom (52);
    in.removeFromBottom (10);
    spelling.setBounds (tuning.removeFromRight (76).withTrimmedTop (16));
    tuning.removeFromRight (12);
    a4Presets.setBounds (tuning.removeFromRight (30).withTrimmedTop (16));
    tuning.removeFromRight (4);
    a4Field.setBounds (tuning);
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
            stKind.setBounds (row (32));
            if (timeStretchMode())
            {
                stLength.setBounds (row (52));
            }
            else
            {
                stMode.setBounds (row (28));
                auto noteRow = row (52);
                stNote.setBounds (noteRow);
                stFreq.setBounds (noteRow);
            }
            stOldBpm.setBounds (row (52));
            stNewBpm.setBounds (row (58));
            microtone.setBounds (out.removeFromBottom (112));
            out.removeFromBottom (8);
            if (! timeStretchMode())
            {
                fitArea = out.removeFromRight (out.getWidth() * 36 / 100);
                out.removeFromRight (8);
                auto f = fitArea;
                auto keyRow = f.removeFromTop (46).withTrimmedTop (16);
                stKey.setBounds (keyRow.removeFromLeft (64));
                keyRow.removeFromLeft (6);
                stScale.setBounds (keyRow);
                f.removeFromTop (6);
                stFit.setBounds (f);
            }
            stOut.setBounds (out);
            break;
        }
        case 2:
            fnFreq.setBounds (row (52));
            fnOut.setBounds (out);
            break;
        case 4:
        {
            fmMode.setBounds (row (32));
            auto noteRow = row (52);
            fmNote.setBounds (noteRow);
            fmFreq.setBounds (noteRow);
            fmOut.setBounds (in.removeFromTop (juce::jmin (in.getHeight(), 130)));
            fmMap.setBounds (out);
            break;
        }
        default:
        {
            hmMode.setBounds (row (32));
            auto noteRow = row (52);
            hmNote.setBounds (noteRow);
            hmFreq.setBounds (noteRow);
            hmSet.setBounds (row (48).withTrimmedTop (16));   // captions drawn in paint()
            hmWave.setBounds (row (48).withTrimmedTop (16));
            hmCount.setBounds (row (48).withTrimmedTop (16));
            hmTable.setBounds (out);
            break;
        }
    }
}

juce::String PitchModule::helpText() const
{
    static constexpr const char* tuningTerms =
        "- A4 REFERENCE: the frequency of A4 that every note is tuned from (440 Hz is the standard). The ... button lists 432, 440, 442 and 443 Hz.\n"
        "- NOTE NAMES: write black keys with sharps (C#) or flats (Db).";
    switch (tab)
    {
        case 0:
            return R"(# What it does
Shifts a note by semitones and cents and shows the new note, its frequency and how far it is from the nearest note.
# How to use
1. Choose the source note.
2. Enter the shift in semitones (-96 to 96) and cents (-100 to 100).
3. Read the new note, the deviation and the frequency on the right. Click a value to copy it.
# Example
A4 + 3 semitones = C5, 523.25 Hz.
===
# Terms
- SEMITONE (st): one key on the piano; 12 make an octave.
- CENT (ct): 1/100 of a semitone.
- DEVIATION: how far the result is from the nearest note, in cents.
- A4 REFERENCE: the frequency of A4 that every note is tuned from (440 Hz is the standard). The ... button lists 432, 440, 442 and 443 Hz.
- NOTE NAMES: write black keys with sharps (C#) or flats (Db).)";
        case 1:
            if (timeStretchMode())
                return R"(# What it does
TIME-STRETCH mode: the sample follows the new tempo but keeps its pitch, like Warp or Flex Time. Shows the new length, the stretch ratio and how audible the stretch is likely to be.
# How to use
1. Choose TIME-STRETCH (VARISPEED changes the pitch instead).
2. Enter the OLD LENGTH of the sample in seconds and the OLD BPM it was made at.
3. NEW BPM is the tempo in the top bar.
4. Read the new length. Up to 10 % tempo change is CLEAN, 10 to 25 % can be AUDIBLE on some material, above 25 % a QUALITY WARNING appears.
# Example
An 8 s loop at 120 BPM played at 126 BPM: 7.619 s, +5 % tempo, stretch ratio x0.9524, clean.
===
# Terms
- TIME-STRETCH: changes the length without changing the pitch. Large changes add artefacts (smeared drums, metallic or phasey sound).
- VARISPEED: speed and pitch change together, like tape or a turntable.
- TEMPO CHANGE: new BPM / old BPM - 1, in %.
- STRETCH RATIO: new length / old length (old BPM / new BPM). Below 1 = shorter.
- HALF / DOUBLE TIME: treating 70 BPM as 140 often needs a much smaller stretch.)";
            return R"(# What it does
VARISPEED mode: shows what happens to the pitch of a sample when you play it at another tempo by changing its speed, and warns when it lands between two notes. FIT TO SCALE lists the tempos that keep it in the key of your song.
# How to use
1. Choose the sample's original note, or type its frequency with FREQ.
2. Enter the OLD BPM the sample was made at.
3. NEW BPM is the tempo in the top bar.
4. Read the new note. A MICROTONE WARNING appears when it is 10 cents or more off a note, with the BPM that lands exactly on each neighbour note.
5. FIT TO SCALE: pick the key and scale of your song. The list shows the nearest tempos that land exactly on a note of that scale; the amber row is the closest to NEW BPM. IN KEY says if the current result is a note of the scale.
# Example
An A4 sample made at 120 BPM, played at 126 BPM: 462 Hz, -15.5 ct from A#4. Set 127.14 BPM to land on A#4. In A minor, 134.70 BPM gives B4 and 106.91 BPM gives G4.
===
# Terms
- VARISPEED: speed and pitch change together, like tape or a turntable. Pitch ratio = new BPM / old BPM. TIME-STRETCH keeps the pitch.
- SEMITONE (st): one key on the piano; 12 make an octave.
- CENT (ct): 1/100 of a semitone. Around 10 ct a note starts to sound out of tune against others.
- MICROTONE: a pitch between two notes of the 12-note scale.
- A4 REFERENCE: the frequency of A4 that every note is tuned from (440 Hz is the standard). The ... button lists 432, 440, 442 and 443 Hz.
- NOTE NAMES: write black keys with sharps (C#) or flats (Db).)";
        case 2:
            return R"(# What it does
Finds the nearest note to any frequency and shows the deviation in cents, the period, the wavelength and how many samples one cycle lasts.
# How to use
1. Type a frequency in Hz, for example a resonance you found with an EQ.
2. Read the nearest note and how far off it is. Click a value to copy it.
# Example
432 Hz is A4 -31.8 ct (with A4 = 440 Hz).
===
# Terms
- CENT (ct): 1/100 of a semitone.
- PERIOD: the length of one cycle (1000 / Hz) in ms.
- WAVELENGTH: the length of one cycle in air at 20 C.
- CYCLE: samples in one cycle at the session sample rate.
- A4 REFERENCE: the frequency of A4 that every note is tuned from (440 Hz is the standard). The ... button lists 432, 440, 442 and 443 Hz.
- NOTE NAMES: write black keys with sharps (C#) or flats (Db).)";
        case 4:
            return R"(# What it does
A map of where common instruments sit in the spectrum, for EQ: the range of their fundamental (amber) and a key zone above it (green) such as a kick's click or a vocal's presence.
# How to use
1. Choose a note, or type a frequency with FREQ, or click on the map. A red marker line shows it.
2. The instruments whose fundamental or key zone contains the marker are lit. Use it to see which parts may clash at that frequency, or where to boost or cut.
The ranges are typical starting points from common EQ charts; every recording is different.
# Example
A2 (110 Hz) is in the fundamental range of the toms, bass, guitars, piano, male vocals and cello. Cutting there on one of them makes room for the others.
===
# Terms
- FUNDAMENTAL: the lowest frequency of a note, which sets its pitch. The bar spans the lowest to the highest note the instrument usually plays.
- KEY ZONE: a band above the fundamental that defines the sound (click, crack, presence, air...).
)" + juce::String (tuningTerms);
        default:
            return R"(# What it does
Lists the harmonic series of a note: the frequencies that make up its tone, the note each one is closest to and how far off it is.
# How to use
1. Choose the fundamental note, or type a frequency with FREQ.
2. SERIES lists all, only odd or only even harmonics.
3. WAVEFORM LEVELS adds each harmonic's level in an ideal saw, square or triangle wave, and picks the matching series.
4. COUNT lists 16, 32 or 64 harmonics; scroll the table with the mouse wheel. Red rows are 10 cents or more off a note.
# Example
A2 (110 Hz): harmonic 3 is E4 +2.0 ct, harmonic 7 is G5 -31.2 ct.
===
# Terms
- HARMONIC: a whole-number multiple of the fundamental (2x, 3x...). Harmonic 1 is the fundamental itself.
- ODD / EVEN: odd harmonics (3, 5, 7) give a hollow, square-like tone; even ones (2, 4, 6) add octaves and warmth.
- SAW / SQUARE / TRIANGLE: saw has every harmonic at 1/n; square has odd ones at 1/n; triangle has odd ones at 1/n^2. LEVEL dB is relative to the fundamental.
- A4 REFERENCE: the frequency of A4 that every note is tuned from (440 Hz is the standard). The ... button lists 432, 440, 442 and 443 Hz.
- NOTE NAMES: write black keys with sharps (C#) or flats (Db).)";
    }
}

//==============================================================================
juce::Rectangle<float> FrequencyMap::plotArea() const
{
    return getLocalBounds().toFloat().reduced (8.0f).withTrimmedLeft (104.0f).withTrimmedRight (10.0f).withTrimmedBottom (18.0f);
}

float FrequencyMap::hzToX (double hz) const
{
    const auto p = plotArea();
    return p.getX() + p.getWidth() * (float) (std::log (hz / 20.0) / std::log (1000.0));
}

double FrequencyMap::xToHz (float x) const
{
    const auto p = plotArea();
    return 20.0 * std::pow (1000.0, (double) ((x - p.getX()) / p.getWidth()));
}

void FrequencyMap::paint (juce::Graphics& g)
{
    drawScreen (g, getLocalBounds().toFloat());
    const auto p = plotArea();
    const auto& ranges = calc::instrumentRanges();

    g.setFont (ledFont (11.0f, false));
    for (double f : { 20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 })
    {
        const float x = hzToX (f);
        g.setColour (skin().screenText.withAlpha (0.22f));
        g.drawVerticalLine (juce::roundToInt (x), p.getY(), p.getBottom());
        g.setColour (skin().screenText);
        const auto label = f >= 1000 ? juce::String (f / 1000.0, 0) + "k" : juce::String (f, 0);
        g.drawText (label, juce::Rectangle<float> (x - 20.0f, p.getBottom() + 2.0f, 40.0f, 14.0f), juce::Justification::centred);
    }

    const float rowH = p.getHeight() / (float) ranges.size();
    for (size_t i = 0; i < ranges.size(); ++i)
    {
        const auto& r = ranges[i];
        const auto row = juce::Rectangle<float> (p.getX(), p.getY() + rowH * (float) i, p.getWidth(), rowH);
        const bool inFund = marker > 0 && marker >= r.fundLowHz && marker <= r.fundHighHz;
        const bool inZone = marker > 0 && marker >= r.zoneLowHz && marker <= r.zoneHighHz;

        g.setColour (inFund ? skin().ledAmber : inZone ? skin().ledGreen : skin().screenText);
        g.setFont (ledFont (11.5f, inFund || inZone));
        g.drawText (r.name, juce::Rectangle<float> (8.0f, row.getY(), p.getX() - 14.0f, rowH), juce::Justification::centredLeft);

        const auto bar = [&] (double lo, double hi, juce::Colour c)
        {
            const auto b = juce::Rectangle<float>::leftTopRightBottom (hzToX (lo), row.getY() + rowH * 0.22f,
                                                                       hzToX (hi), row.getBottom() - rowH * 0.22f);
            g.setColour (c);
            g.fillRoundedRectangle (b, 2.0f);
            return b;
        };
        bar (r.fundLowHz, r.fundHighHz, skin().ledAmber.withAlpha (inFund ? 0.95f : 0.6f));
        const auto z = bar (r.zoneLowHz, r.zoneHighHz, skin().ledGreen.withAlpha (inZone ? 0.85f : 0.4f));
        g.setColour (skin().screen);
        g.setFont (ledFont (10.0f, true));
        if ((float) juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), r.zone) < z.getWidth() - 4.0f)
            g.drawText (r.zone, z, juce::Justification::centred);
    }

    if (marker > 0)
    {
        const float x = hzToX (marker);
        g.setColour (skin().ledRed);
        g.fillRect (x - 1.0f, p.getY(), 2.0f, p.getHeight());
    }
}

void FrequencyMap::mouseDown (const juce::MouseEvent& e)
{
    const auto p = plotArea();
    if (onClick && e.position.x >= p.getX() && e.position.x <= p.getRight())
        onClick (juce::jlimit (20.0, 20000.0, xToHz (e.position.x)));
}
