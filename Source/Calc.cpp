#include "Calc.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <locale>
#include <sstream>

namespace calc
{
//==============================================================================
Parsed parseNumber (const std::string& text, const std::string& fieldName)
{
    std::string s;
    for (char c : text)
        s += (c == ',' ? '.' : c);

    const auto first = s.find_first_not_of (" \t\r\n");
    if (first == std::string::npos)
        return { std::nullopt, fieldName + ": field is empty" };
    s = s.substr (first, s.find_last_not_of (" \t\r\n") - first + 1);

    std::istringstream in (s);
    in.imbue (std::locale::classic());
    double v = 0;
    in >> v;
    if (in.fail() || ! (in >> std::ws).eof())
        return { std::nullopt, fieldName + ": \"" + s + "\" is not a number" };
    if (! std::isfinite (v))
        return { std::nullopt, fieldName + ": value is not finite" };
    return { v, {} };
}

Parsed parseInRange (const std::string& text, const std::string& fieldName, double minV, double maxV)
{
    auto p = parseNumber (text, fieldName);
    if (! p.ok())
        return p;
    if (*p.value < minV || *p.value > maxV)
        return { std::nullopt, fieldName + " must be between " + fmt (minV, minV == std::floor (minV) ? 0 : 3)
                                   + " and " + fmt (maxV, maxV == std::floor (maxV) ? 0 : 3) };
    return p;
}

Parsed parseBpm (const std::string& text, const std::string& fieldName)
{
    auto p = parseNumber (text, fieldName);
    if (! p.ok())
        return p;
    if (*p.value <= 0)
        return { std::nullopt, fieldName + " must be greater than 0" };
    return parseInRange (text, fieldName, kMinBpm, kMaxBpm);
}

//==============================================================================
double quarterMs (double bpm)   { return 60000.0 / bpm; }
double wholeNoteMs (double bpm) { return 240000.0 / bpm; }

double noteMs (double bpm, double denominator, NoteKind kind)
{
    const double base = wholeNoteMs (bpm) / denominator;
    switch (kind)
    {
        case NoteKind::Dotted:  return base * 1.5;
        case NoteKind::Triplet: return base * 2.0 / 3.0;
        case NoteKind::Normal:  break;
    }
    return base;
}

double msToHz (double ms) { return 1000.0 / ms; }
double hzToMs (double hz) { return 1000.0 / hz; }

std::string noteValueLabel (int denominator, NoteKind kind)
{
    std::string s = "1/" + std::to_string (denominator);
    if (kind == NoteKind::Dotted)  s += "D";
    if (kind == NoteKind::Triplet) s += "T";
    return s;
}

const std::vector<int>& delayDenominators()
{
    static const std::vector<int> d { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512 };
    return d;
}

//==============================================================================
double midiToFreq (double midi) { return kA4Hz * std::pow (2.0, (midi - kA4Midi) / 12.0); }
double freqToMidi (double hz)   { return kA4Midi + 12.0 * std::log2 (hz / kA4Hz); }

std::string pitchClassName (int pc)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return names[((pc % 12) + 12) % 12];
}

std::string noteName (int midi)
{
    const int octave = (int) std::floor (midi / 12.0) - 1;
    return pitchClassName (midi) + std::to_string (octave);
}

int noteToMidi (int pitchClass, int octave) { return (octave + 1) * 12 + pitchClass; }

NoteInfo analyse (double hz)
{
    NoteInfo n;
    n.freq        = hz;
    n.exactMidi   = freqToMidi (hz);
    n.nearestMidi = (int) std::lround (n.exactMidi);
    n.nearestFreq = midiToFreq (n.nearestMidi);
    n.cents       = (n.exactMidi - n.nearestMidi) * 100.0;
    return n;
}

TransposeResult transpose (double sourceHz, double semitones, double cents)
{
    TransposeResult r;
    r.sourceFreq = sourceHz;
    r.targetFreq = sourceHz * std::pow (2.0, (semitones + cents / 100.0) / 12.0);
    r.target     = analyse (r.targetFreq);
    return r;
}

StretchResult stretch (double oldBpm, double oldHz, double newBpm, double thresholdCents)
{
    StretchResult r;
    r.ratio     = newBpm / oldBpm;
    r.semitones = 12.0 * std::log2 (r.ratio);
    r.oldFreq   = oldHz;
    r.newFreq   = oldHz * r.ratio;
    r.note      = analyse (r.newFreq);

    // Compare against the value as displayed (0.1 ct) so the warning never contradicts the readout.
    const double shownCents = std::round (std::abs (r.note.cents) * 10.0) / 10.0;
    r.microtone = shownCents >= thresholdCents;

    if (r.note.cents >= 0) { r.lowerMidi = r.note.nearestMidi;     r.upperMidi = r.note.nearestMidi + 1; }
    else                   { r.lowerMidi = r.note.nearestMidi - 1; r.upperMidi = r.note.nearestMidi; }

    const auto bpmFor = [&] (int midi) { return oldBpm * midiToFreq (midi) / oldHz; };
    r.bpmForNearest = bpmFor (r.note.nearestMidi);
    r.bpmForLower   = bpmFor (r.lowerMidi);
    r.bpmForUpper   = bpmFor (r.upperMidi);
    return r;
}

std::vector<Harmonic> harmonics (double fundamentalHz, int count, double maxHz)
{
    std::vector<Harmonic> out;
    for (int n = 1; n <= count; ++n)
    {
        const double f = fundamentalHz * n;
        if (f > maxHz)
            break;
        out.push_back ({ n, f, analyse (f) });
    }
    return out;
}

//==============================================================================
const ReverbStyleInfo& styleInfo (ReverbStyle s)
{
    static const ReverbStyleInfo infos[kNumReverbStyles] = {
        { "SMALL ROOM",  8.0, 0.2, 0.6, 0.30 },
        { "ROOM",       15.0, 0.5, 1.2, 0.25 },
        { "LARGE ROOM", 25.0, 1.0, 2.0, 0.20 },
        { "PLATE",      12.0, 1.0, 2.5, 0.15 },
        { "HALL",       35.0, 1.8, 3.5, 0.18 },
    };
    return infos[std::clamp ((int) s, 0, kNumReverbStyles - 1)];
}

std::vector<ReverbRow> reverbRows (double bpm, ReverbStyle style)
{
    struct Len { const char* label; double beats; };
    static const Len lengths[] = {
        { "4 BARS", 16.0 }, { "2 BARS", 8.0 }, { "1/1", 4.0 }, { "1/2", 2.0 },
        { "1/4", 1.0 },     { "1/8", 0.5 },    { "1/16", 0.25 }, { "1/32", 0.125 },
    };
    static const int preDenoms[] = { 64, 128, 256, 512 };

    const auto& info = styleInfo (style);

    // Musical pre-delay: the note division closest (log scale) to the style's typical pre-delay.
    int bestIdx = 0;
    double bestErr = 1e9;
    for (int i = 0; i < 4; ++i)
    {
        const double err = std::abs (std::log (noteMs (bpm, preDenoms[i]) / info.preDelayTargetMs));
        if (err < bestErr) { bestErr = err; bestIdx = i; }
    }

    std::vector<ReverbRow> rows;
    for (const auto& len : lengths)
    {
        ReverbRow r;
        r.label   = len.label;
        r.beats   = len.beats;
        r.totalMs = quarterMs (bpm) * len.beats;

        // Never let pre-delay eat more than a quarter of the total tail.
        int idx = bestIdx;
        while (idx < 3 && noteMs (bpm, preDenoms[idx]) > r.totalMs * 0.25)
            ++idx;
        r.preDelayMs    = std::min (noteMs (bpm, preDenoms[idx]), r.totalMs * 0.25);
        r.preDelayLabel = noteValueLabel (preDenoms[idx]);
        r.decayMs       = r.totalMs - r.preDelayMs;
        rows.push_back (r);
    }
    return rows;
}

int recommendedReverbRow (const std::vector<ReverbRow>& rows, ReverbStyle style)
{
    const auto& info = styleInfo (style);
    const double targetMs = std::sqrt (info.decayMinS * info.decayMaxS) * 1000.0;
    int best = 0;
    double bestErr = 1e9;
    for (size_t i = 0; i < rows.size(); ++i)
    {
        const double err = std::abs (std::log (rows[i].totalMs / targetMs));
        if (err < bestErr) { bestErr = err; best = (int) i; }
    }
    return best;
}

RoomSize roomForRt60 (double rt60Seconds, double absorption)
{
    constexpr double pl = 1.59, pw = 1.26, ph = 1.0;
    constexpr double vUnit = pl * pw * ph;
    constexpr double sUnit = 2.0 * (pl * pw + pl * ph + pw * ph);
    const double k = rt60Seconds * absorption * sUnit / (0.161 * vUnit);

    RoomSize r;
    r.length  = pl * k;
    r.width   = pw * k;
    r.height  = ph * k;
    r.volume  = vUnit * k * k * k;
    r.surface = sUnit * k * k;
    return r;
}

//==============================================================================
const std::vector<CompSource>& compSources()
{
    static const std::vector<CompSource> s {
        { "KICK",       15, 30, 20, 8,  NoteKind::Normal, "Slow attack keeps the beater click; release resets before the next 8th." },
        { "SNARE",       5, 15, 10, 8,  NoteKind::Normal, "Let the stick transient through; release by the next 8th keeps it snappy." },
        { "HI-HAT",      1,  5,  2, 16, NoteKind::Normal, "Fast attack tames spikes; short release follows 16th grooves." },
        { "TOMS",       10, 25, 15, 8,  NoteKind::Normal, "Medium attack keeps the hit; release returns between fills." },
        { "BASS",        5, 20, 10, 4,  NoteKind::Normal, "Medium attack, beat-length release for an even, sustained level." },
        { "VOCAL",       3, 10,  5, 4,  NoteKind::Normal, "Fast-ish attack for consistency; quarter-note release breathes with phrases." },
        { "AC. GUITAR",  5, 15, 10, 8,  NoteKind::Normal, "Keep the pick attack; 8th release follows strumming." },
        { "EL. GUITAR", 10, 25, 15, 8,  NoteKind::Normal, "Already dense; gentle attack, 8th release." },
        { "PIANO/KEYS", 10, 30, 20, 4,  NoteKind::Normal, "Preserve hammer attack; longer release avoids pumping on sustain." },
        { "SYNTH PAD",  20, 50, 30, 2,  NoteKind::Normal, "Slow everything; half-note release for smooth movement." },
        { "DRUM BUS",   10, 30, 30, 8,  NoteKind::Normal, "Slow attack for punch; 8th release makes the bus groove/pump in time." },
        { "MIX BUS",    10, 30, 30, 4,  NoteKind::Normal, "Glue: slow attack, quarter-note (or auto) release, 1-2 dB GR." },
    };
    return s;
}

const std::vector<int>& releaseDenominators()
{
    static const std::vector<int> d { 1, 2, 4, 8, 16, 32, 64 };
    return d;
}

//==============================================================================
double dbToGain (double db)   { return std::pow (10.0, db / 20.0); }
double gainToDb (double gain) { return 20.0 * std::log10 (gain); }

double qToOctaves (double q)
{
    return 2.0 / std::log (2.0) * std::asinh (1.0 / (2.0 * q));
}

double octavesToQ (double octaves)
{
    const double p = std::pow (2.0, octaves);
    return std::sqrt (p) / (p - 1.0);
}

Band bandForQ (double centreHz, double q)
{
    const double n = qToOctaves (q);
    Band b;
    b.lowHz   = centreHz * std::pow (2.0, -n / 2.0);
    b.highHz  = centreHz * std::pow (2.0,  n / 2.0);
    b.widthHz = b.highHz - b.lowHz;
    return b;
}

double msToSamples (double ms, double sampleRate)      { return ms * sampleRate / 1000.0; }
double samplesToMs (double samples, double sampleRate) { return samples * 1000.0 / sampleRate; }
double speedOfSound (double celsius)                   { return 331.3 * std::sqrt (1.0 + celsius / 273.15); }
double metresToMs (double metres, double celsius)      { return metres / speedOfSound (celsius) * 1000.0; }
double msToMetres (double ms, double celsius)          { return ms / 1000.0 * speedOfSound (celsius); }

//==============================================================================
double barMs (double bpm, double beatsPerBar, double beatUnit)
{
    return beatsPerBar * quarterMs (bpm) * 4.0 / beatUnit;
}

double loopMs (double bpm, double bars, double beatsPerBar, double beatUnit)
{
    return bars * barMs (bpm, beatsPerBar, beatUnit);
}

//==============================================================================
std::string fmt (double v, int decimals)
{
    char buf[64];
    std::snprintf (buf, sizeof (buf), "%.*f", std::max (0, decimals), v);
    std::string s (buf);
    if (s == "-0" || s.rfind ("-0.", 0) == 0)
    {
        // avoid "-0.00"
        bool allZero = true;
        for (char c : s) if (c >= '1' && c <= '9') allZero = false;
        if (allZero) s = s.substr (1);
    }
    return s;
}

std::string fmtMs (double ms)
{
    const double a = std::abs (ms);
    return fmt (ms, a < 10 ? 3 : a < 100 ? 2 : 1);
}

std::string fmtHz (double hz)
{
    const double a = std::abs (hz);
    return fmt (hz, a < 1 ? 4 : a < 10 ? 3 : a < 1000 ? 2 : 1);
}

std::string fmtCents (double c)
{
    const std::string s = fmt (c, 1);
    return (c >= 0.05 ? "+" : "") + s + " ct";
}
} // namespace calc
