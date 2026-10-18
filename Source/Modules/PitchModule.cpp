#include "Modules.h"

using namespace ui;

namespace
{
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
      hmMode ({ "NOTE", "FREQ" }, col::capGreen, 303, false),
      hmSet ({ "ALL", "ODD", "EVEN" }, col::capWhite, 304, false),
      hmWave ({ "OFF", "SAW", "SQUARE", "TRI" }, col::capYellow, 305, false),
      hmCount ({ "16", "32", "64" }, col::capWhite, 306, false),
      spelling ({ "#", "b" }, col::capWhite, 307, false)
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
    fnFreq.bind (s, "fnFreq", "432");
    hmNote.bind (s, "hmNote", 45);
    hmFreq.bind (s, "hmFreq", "110");
    stMode.setSelected ((int) s.getProperty ("stMode", 0), false);
    hmMode.setSelected ((int) s.getProperty ("hmMode", 0), false);
    hmSet.setSelected ((int) s.getProperty ("hmSet", 0), false);
    hmWave.setSelected ((int) s.getProperty ("hmWave", 0), false);
    hmCount.setSelected ((int) s.getProperty ("hmCount", 0), false);
    stNewBpm.setText (ctx.bpmTextFor (getId()));

    for (auto* comp : std::initializer_list<juce::Component*> {
             &tabs, &tpNote, &tpSemis, &tpCents, &tpOut, &stMode, &stNote, &stFreq, &stOldBpm, &stNewBpm,
             &stOut, &fnFreq, &fnOut, &hmMode, &hmSet, &hmWave, &hmCount, &hmNote, &hmFreq, &hmTable })
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

    for (auto* f : { &tpSemis, &tpCents, &stFreq, &stOldBpm, &fnFreq, &hmFreq })
        f->onChange = [this] { refresh(); };
    stNewBpm.onChange = [this] { ctx.setBpmTextFor (getId(), stNewBpm.getText()); }; // editor calls refresh()
    for (auto* n : { &tpNote, &stNote, &hmNote })
        n->onChange = [this] { refresh(); };
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
    updateHelp();
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
    hmSet.setVisible (hm); hmWave.setVisible (hm); hmCount.setVisible (hm);
}

juce::String PitchModule::noteText (int midi) const { return str (calc::noteName (midi, flats)); }

juce::String PitchModule::noteWithCents (const calc::NoteInfo& n) const
{
    return noteText (n.nearestMidi) + "  " + str (calc::fmtCents (n.cents));
}

void PitchModule::refresh()
{
    flats = spelling.getSelected() == 1;
    for (auto* n : { &tpNote, &stNote, &hmNote })
        n->setFlats (flats);

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
        default: refreshHarmonics (v); break;
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
                          { "RESULT", "--", col::ledRed, 1.8f } });
        return;
    }
    const auto r = calc::transpose (srcHz, *semis, *cents, a4);
    if (r.target.nearestMidi < 0 || r.target.nearestMidi > 131)
    {
        v.errors.add ("Result is outside the MIDI note range (C-1 .. B9)");
        tpOut.setLines ({ { "RESULT", "OUT OF RANGE", col::ledRed, 1.8f } });
        return;
    }
    tpOut.setLines ({
        { "SOURCE", noteText (src) + "  " + str (calc::fmtHz (srcHz)) + " Hz" },
        { "SHIFT", str (calc::fmt (*semis, 2)) + " st  " + str (calc::fmtCents (*cents)) },
        { "NEW NOTE", noteText (r.target.nearestMidi), col::ledGreen, 2.0f },
        { "DEVIATION", str (calc::fmtCents (r.target.cents)), centsColour (r.target.cents) },
        { "NEW FREQUENCY", str (calc::fmtHz (r.targetFreq)) + " Hz", col::ledAmber, 1.3f },
        { "RATIO", "x" + str (calc::fmt (r.targetFreq / srcHz, 5)) },
    });
}

void PitchModule::refreshStretch (Validator& v)
{
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
        stOut.setLines ({ { "NEW NOTE", "--", col::ledRed, 2.2f }, { "NEW FREQUENCY", "--", col::ledRed } });
        microtone.setVisible (false);
        return;
    }

    const auto r = calc::stretch (*oldBpm, oldHz, *newBpm.value, calc::kMicrotoneThresholdCents, a4);
    const auto oldInfo = calc::analyse (oldHz, a4);
    if (r.note.nearestMidi < 0 || r.note.nearestMidi > 131)
    {
        v.errors.add ("Result is outside the MIDI note range (C-1 .. B9)");
        stOut.setLines ({ { "NEW NOTE", "OUT OF RANGE", col::ledRed, 2.0f } });
        microtone.setVisible (false);
        return;
    }

    stOut.setLines ({
        { "OLD", noteWithCents (oldInfo) + "  " + str (calc::fmtHz (oldHz)) + " Hz", col::ledAmber.withAlpha (0.75f) },
        { "NEW NOTE", noteText (r.note.nearestMidi), r.microtone ? col::ledRed : col::ledGreen, 2.2f },
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

void PitchModule::refreshFreq (Validator& v)
{
    const auto f = v.num (fnFreq, "Frequency", 1, 30000);
    if (! v.ok())
    {
        fnOut.setLines ({ { "NOTE", "--", col::ledRed, 2.2f } });
        return;
    }
    const auto n = calc::analyse (*f, a4);
    const double sr = ctx.hostSampleRate();
    fnOut.setLines ({
        { "NEAREST NOTE", noteText (n.nearestMidi), centsColour (n.cents) == col::ledRed ? col::ledAmber : col::ledGreen, 2.2f },
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
            hmTable.setRowColour (i, col::ledRed.withAlpha (0.85f));
}

void PitchModule::paint (juce::Graphics& g)
{
    ModuleBase::paint (g);
    static const char* inTitles[] = { "Transpose", "Stretch input", "Frequency", "Fundamental" };
    static const char* outTitles[] = { "Result", "Stretched sample", "Note", "Harmonic series (red = >10 ct off)" };
    drawSection (g, inputArea, inTitles[tab]);
    drawSection (g, outputArea, outTitles[tab]);

    g.setFont (labelFont (10.5f));
    g.setColour (col::silkscreen.withAlpha (0.85f));
    g.drawText ("NOTE NAMES", spelling.getBounds().translated (0, -16).withHeight (15), juce::Justification::centredLeft);
    g.setColour (col::silkDim.withAlpha (0.4f));
    g.fillRect (inputArea.getX() + 10, a4Field.getY() - 8, inputArea.getWidth() - 20, 1);

    if (tab == 1)
    {
        auto hint = stNewBpm.getBounds().translated (0, stNewBpm.getHeight() + 4).withHeight (34);
        g.setColour (col::silkDim);
        g.setFont (labelFont (10.0f, false));
        g.drawFittedText ("NEW BPM = top bar BPM" + juce::String (ctx.isChained() ? " (chained)" : " for this module")
                              + ". Speed change = pitch change (varispeed).",
                          hint, juce::Justification::topLeft, 3);
    }
    else if (tab == 3)
    {
        g.setFont (labelFont (10.5f));
        g.setColour (col::silkscreen.withAlpha (0.85f));
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
            return R"(# What it does
Shows what happens to the pitch of a sample when you play it at another tempo by changing its speed (varispeed), and warns when it lands between two notes.
# How to use
1. Choose the sample's original note, or type its frequency with FREQ.
2. Enter the OLD BPM the sample was made at.
3. NEW BPM is the tempo in the top bar.
4. Read the new note. A MICROTONE WARNING appears when it is 10 cents or more off a note, with the BPM that lands exactly on each neighbour note.
# Example
An A4 sample made at 120 BPM, played at 126 BPM: 462 Hz, -15.5 ct from A#4. Set 127.14 BPM to land on A#4.
===
# Terms
- VARISPEED: speed and pitch change together, like tape or a turntable. Pitch ratio = new BPM / old BPM. Time-stretch (pitch kept) is not covered here.
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
