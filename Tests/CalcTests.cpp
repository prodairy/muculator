#include "Calc.h"

#include <cmath>
#include <cstdio>

static int failures = 0, checks = 0;

static void near (const char* what, double got, double want, double tol = 1e-6)
{
    ++checks;
    if (std::abs (got - want) > tol)
    {
        ++failures;
        std::printf ("FAIL %s: got %.9f want %.9f\n", what, got, want);
    }
}

static void truth (const char* what, bool cond)
{
    ++checks;
    if (! cond) { ++failures; std::printf ("FAIL %s\n", what); }
}

int main()
{
    using namespace calc;

    // --- parsing / validation
    truth ("empty rejected",    ! parseNumber ("", "X").ok());
    truth ("spaces rejected",   ! parseNumber ("   ", "X").ok());
    truth ("junk rejected",     ! parseNumber ("12abc", "X").ok());
    truth ("inf rejected",      ! parseNumber ("1e999", "X").ok());
    truth ("nan rejected",      ! parseNumber ("nan", "X").ok());
    truth ("0 bpm rejected",    ! parseBpm ("0").ok());
    truth ("neg bpm rejected",  ! parseBpm ("-120").ok());
    truth ("huge bpm rejected", ! parseBpm ("5000").ok());
    near  ("comma decimal",     parseNumber (" 120,5 ", "X").value.value_or (0), 120.5);
    near  ("bpm ok",            parseBpm ("128").value.value_or (0), 128);

    // --- tempo
    near ("quarter@120", noteMs (120, 4), 500);
    near ("dotted 1/8@120", noteMs (120, 8, NoteKind::Dotted), 375);
    near ("triplet 1/4@120", noteMs (120, 4, NoteKind::Triplet), 1000.0 / 3.0);
    near ("1/512@120", noteMs (120, 512), 3.90625);
    near ("hz of 500ms", msToHz (500), 2);

    // --- pitch
    near ("A4", midiToFreq (69), 440);
    near ("C4", midiToFreq (60), 261.6255653, 1e-6);
    truth ("C4 name", noteName (60) == "C4");
    truth ("A#-1 name", noteName (10) == "A#-1");
    truth ("C-1 name", noteName (0) == "C-1");
    near ("noteToMidi A4", noteToMidi (9, 4), 69);
    {
        auto n = analyse (450);
        truth ("450 -> A4", n.nearestMidi == 69);
        near ("450 cents", n.cents, 1200 * std::log2 (450.0 / 440.0), 1e-9);
    }
    {
        auto t = transpose (440, 3, 0);
        truth ("A4+3 = C5", noteName (t.target.nearestMidi) == "C5");
        near ("A4+3 freq", t.targetFreq, 523.2511306, 1e-6);
    }

    // --- stretch
    {
        auto s = stretch (120, 440, 126);
        near ("ratio", s.ratio, 1.05);
        near ("semis", s.semitones, 0.84467, 1e-4);
        near ("newF", s.newFreq, 462);
        truth ("nearest A#4", noteName (s.note.nearestMidi) == "A#4");
        truth ("microtone flagged", s.microtone);
        truth ("between A4 and A#4", noteName (s.lowerMidi) == "A4" && noteName (s.upperMidi) == "A#4");
        near ("bpm for A#4", s.bpmForNearest, 120 * midiToFreq (70) / 440, 1e-9);
        near ("bpm for A4", s.bpmForLower, 120, 1e-9);
        // Re-stretching at the suggested BPM must land exactly on the note.
        auto s2 = stretch (120, 440, s.bpmForNearest);
        near ("suggested bpm lands on note", s2.note.cents, 0, 1e-6);
        truth ("suggested bpm no warning", ! s2.microtone);
    }
    {
        auto s = stretch (120, 440, 240);
        truth ("double speed = A5", noteName (s.note.nearestMidi) == "A5");
        truth ("no microtone at octave", ! s.microtone);
        near ("octave semis", s.semitones, 12);
    }
    {
        // ~9 cents: below threshold
        const double bpm = 120 * std::pow (2.0, 9.0 / 1200.0);
        auto s = stretch (120, 440, bpm);
        truth ("9 ct no warning", ! s.microtone);
        const double bpm10 = 120 * std::pow (2.0, 10.0 / 1200.0);
        truth ("10 ct warning", stretch (120, 440, bpm10).microtone);
    }

    // --- harmonics
    {
        auto h = harmonics (110, 16);
        truth ("16 harmonics", h.size() == 16);
        truth ("H2 = A3", noteName (h[1].note.nearestMidi) == "A3");
        truth ("H3 = E4", noteName (h[2].note.nearestMidi) == "E4");
        near ("H3 cents ~ +1.96", h[2].note.cents, 1.955, 1e-3);
        near ("H7 cents ~ -31.2", h[6].note.cents, -31.174, 1e-3);
        truth ("capped at 22k", harmonics (5000, 16).size() == 4);

        auto odd = harmonics (110, 16, HarmonicSet::Odd);
        truth ("16 odd harmonics", odd.size() == 16);
        truth ("odd: 1, 3, 5", odd[0].n == 1 && odd[1].n == 3 && odd[2].n == 5);
        truth ("odd: last is 31", odd.back().n == 31);
        auto even = harmonics (110, 16, HarmonicSet::Even);
        truth ("even: 2, 4 .. 32", even[0].n == 2 && even[1].n == 4 && even.back().n == 32);
        near ("even H2 freq", even[0].freq, 220);
        truth ("64 harmonics", harmonics (110, 64).size() == 64);
        truth ("odd capped at 22k", harmonics (5000, 16, HarmonicSet::Odd).size() == 2); // 5k, 15k (25k is cut)

        near ("saw H3", harmonicLevel (Waveform::Saw, 3), 1.0 / 3.0);
        near ("saw H2", harmonicLevel (Waveform::Saw, 2), 0.5);
        near ("square H3", harmonicLevel (Waveform::Square, 3), 1.0 / 3.0);
        near ("square H2 absent", harmonicLevel (Waveform::Square, 2), 0.0);
        near ("triangle H3", harmonicLevel (Waveform::Triangle, 3), 1.0 / 9.0);
        near ("triangle H4 absent", harmonicLevel (Waveform::Triangle, 4), 0.0);
        near ("fundamental = 1", harmonicLevel (Waveform::Triangle, 1), 1.0);
        truth ("square is odd", harmonicSetFor (Waveform::Square) == HarmonicSet::Odd);
        truth ("saw is all", harmonicSetFor (Waveform::Saw) == HarmonicSet::All);
    }

    // --- note spelling and tuning reference
    truth ("C#4 flat = Db4", noteName (61, true) == "Db4");
    truth ("A#3 flat = Bb3", noteName (58, true) == "Bb3");
    truth ("naturals unchanged", noteName (60, true) == "C4" && noteName (64, true) == "E4");
    truth ("sharps default", noteName (61) == "C#4");
    near ("A4 @432", midiToFreq (69, 432), 432);
    near ("A5 @442", midiToFreq (81, 442), 884);
    near ("freqToMidi @432", freqToMidi (432, 432), 69);
    {
        auto n = analyse (440, 432); // 440 Hz is +31.8 ct above A4 = 432
        truth ("440 @432 -> A4", n.nearestMidi == 69);
        near ("440 @432 cents", n.cents, 1200 * std::log2 (440.0 / 432.0), 1e-9);
        near ("note Hz @432", n.nearestFreq, 432);
        auto t = transpose (432, 12, 0, 432);
        truth ("transpose @432 = A5", noteName (t.target.nearestMidi) == "A5");
        near ("transpose @432 in tune", t.target.cents, 0, 1e-9);
        auto s = stretch (120, 432, 126, kMicrotoneThresholdCents, 432);
        near ("stretch @432 lands on A#4 bpm", stretch (120, 432, s.bpmForNearest, kMicrotoneThresholdCents, 432).note.cents, 0, 1e-6);
        auto h = harmonics (432, 2, HarmonicSet::All, 432);
        near ("harmonic @432 in tune", h[1].note.cents, 0, 1e-9);
    }

    // --- reverb
    {
        auto rows = reverbRows (120, ReverbStyle::Hall);
        truth ("8 rows", rows.size() == 8);
        truth ("hall -> 1/1", rows[(size_t) recommendedReverbRow (rows, ReverbStyle::Hall)].label == "1/1");
        truth ("small room -> 1/4 or 1/8",
               [&] { auto r = reverbRows (120, ReverbStyle::SmallRoom);
                     auto l = r[(size_t) recommendedReverbRow (r, ReverbStyle::SmallRoom)].label;
                     return l == "1/4" || l == "1/8"; }());
        for (auto& r : rows)
        {
            near ("total = pre + decay", r.preDelayMs + r.decayMs, r.totalMs, 1e-9);
            truth ("pre <= 25%", r.preDelayMs <= r.totalMs * 0.25 + 1e-9);
        }
        auto room = roomForRt60 (2.0, 0.2);
        near ("sabine roundtrip", 0.161 * room.volume / (room.surface * 0.2), 2.0, 1e-9);
    }

    // --- converters
    near ("-6 dB", dbToGain (-6), 0.5011872336, 1e-9);
    near ("x2 dB", gainToDb (2), 6.0205999, 1e-6);
    near ("Q 1.414 = 1 oct", qToOctaves (std::sqrt (2.0)), 1.0, 1e-9);
    near ("1 oct = Q 1.414", octavesToQ (1), std::sqrt (2.0), 1e-9);
    near ("roundtrip Q", octavesToQ (qToOctaves (4.3)), 4.3, 1e-9);
    {
        auto b = bandForQ (1000, 2);
        near ("bandwidth = f/Q", b.widthHz, 500, 1e-9);
        near ("geometric centre", std::sqrt (b.lowHz * b.highHz), 1000, 1e-9);
    }
    near ("ms->samples", msToSamples (10, 48000), 480);
    near ("c @20C", speedOfSound (20), 343.2, 0.1);
    near ("1 m -> ms", metresToMs (1, 20), 2.914, 0.001);
    near ("ms -> m roundtrip", msToMetres (metresToMs (3.7, 20), 20), 3.7, 1e-9);

    // --- loop
    near ("4 bars @120 4/4", loopMs (120, 4, 4, 4), 8000);
    near ("1 bar 6/8 @120", barMs (120, 6, 8), 1500);

    // --- formatting
    truth ("no -0", fmt (-0.0001, 2) == "0.00");
    truth ("cents sign", fmtCents (12.34) == "+12.3 ct");

    std::printf ("%d/%d checks passed\n", checks - failures, checks);
    return failures == 0 ? 0 : 1;
}
