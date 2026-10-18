#pragma once
// Muculator calculation engine. Pure C++ (no JUCE) so it can be unit tested.
// Every function assumes already-validated inputs; validation lives in parse*/validate*.

#include <optional>
#include <string>
#include <vector>

namespace calc
{
constexpr double kA4Hz      = 440.0;
constexpr int    kA4Midi    = 69;
constexpr double kMinBpm    = 1.0;
constexpr double kMaxBpm    = 999.0;
constexpr double kMicrotoneThresholdCents = 10.0;

//==============================================================================
// Input validation
struct Parsed
{
    std::optional<double> value;
    std::string error; // empty when value is set
    bool ok() const { return value.has_value(); }
};

/** Accepts "120", "120.5", "120,5", surrounding spaces. Rejects empty / NaN / inf / junk. */
Parsed parseNumber (const std::string& text, const std::string& fieldName);
/** parseNumber + range check (inclusive). */
Parsed parseInRange (const std::string& text, const std::string& fieldName, double minV, double maxV);
Parsed parseBpm (const std::string& text, const std::string& fieldName = "BPM");

//==============================================================================
// Tempo / note values
enum class NoteKind { Normal, Dotted, Triplet };

double quarterMs (double bpm);
double wholeNoteMs (double bpm);
/** Duration of 1/denominator note, e.g. denominator 8 = eighth note. */
double noteMs (double bpm, double denominator, NoteKind kind = NoteKind::Normal);
double msToHz (double ms);
double hzToMs (double hz);
std::string noteValueLabel (int denominator, NoteKind kind = NoteKind::Normal);
const std::vector<int>& delayDenominators(); // 1 .. 512

//==============================================================================
// Pitch
double midiToFreq (double midi);
double freqToMidi (double hz);
std::string noteName (int midi);      // "C#4", A4 = 69
std::string pitchClassName (int pc);  // 0..11 -> "C".."B"
int noteToMidi (int pitchClass, int octave);

struct NoteInfo
{
    double freq        = 0;
    double exactMidi   = 0;
    int    nearestMidi = 0;
    double nearestFreq = 0;
    double cents       = 0; // freq relative to nearest note, -50..+50
};
NoteInfo analyse (double hz);

struct TransposeResult
{
    double  sourceFreq = 0;
    double  targetFreq = 0;
    NoteInfo target;
};
TransposeResult transpose (double sourceHz, double semitones, double cents);

struct StretchResult
{
    double   ratio       = 1;  // playback speed factor (newBpm / oldBpm)
    double   semitones   = 0;  // pitch shift caused by the speed change
    double   oldFreq     = 0;
    double   newFreq     = 0;
    NoteInfo note;
    bool     microtone   = false;
    int      lowerMidi   = 0;  // the two notes the result sits between
    int      upperMidi   = 0;
    double   bpmForNearest = 0;
    double   bpmForLower   = 0;
    double   bpmForUpper   = 0;
};
/** Varispeed (tape-style) stretch: changing tempo by factor r changes pitch by factor r. */
StretchResult stretch (double oldBpm, double oldHz, double newBpm,
                       double thresholdCents = kMicrotoneThresholdCents);

struct Harmonic { int n; double freq; NoteInfo note; };
enum class HarmonicSet { All, Odd, Even };
/** The first `count` harmonics of the chosen set (n = 1 is the fundamental), stopping at maxHz. */
std::vector<Harmonic> harmonics (double fundamentalHz, int count, HarmonicSet set = HarmonicSet::All,
                                 double maxHz = 22000.0);

enum class Waveform { Saw, Square, Triangle };
/** Ideal amplitude of harmonic n relative to the fundamental: saw 1/n (all), square 1/n (odd),
    triangle 1/n^2 (odd). 0 when the waveform has no such harmonic. */
double harmonicLevel (Waveform w, int n);
/** The harmonic set a waveform contains. */
HarmonicSet harmonicSetFor (Waveform w);

//==============================================================================
// Reverb
enum class ReverbStyle { SmallRoom, Room, LargeRoom, Plate, Hall };
constexpr int kNumReverbStyles = 5;

struct ReverbStyleInfo
{
    const char* name;
    double preDelayTargetMs;
    double decayMinS, decayMaxS;
    double absorption;           // average Sabine absorption coefficient
};
const ReverbStyleInfo& styleInfo (ReverbStyle s);

struct ReverbRow
{
    std::string label;
    double beats       = 0; // length in quarter notes
    double totalMs     = 0;
    double preDelayMs  = 0;
    std::string preDelayLabel;
    double decayMs     = 0;
};
std::vector<ReverbRow> reverbRows (double bpm, ReverbStyle style);
int recommendedReverbRow (const std::vector<ReverbRow>& rows, ReverbStyle style);

struct RoomSize { double length = 0, width = 0, height = 0, volume = 0, surface = 0; };
/** Sabine: RT60 = 0.161 V / (S * a), room proportions 1.59 : 1.26 : 1 (Bolt). */
RoomSize roomForRt60 (double rt60Seconds, double absorption);

//==============================================================================
// Compressor
struct CompSource
{
    const char* name;
    double attackMinMs, attackMaxMs, attackRecMs;
    int    releaseDenominator;
    NoteKind releaseKind;
    const char* hint;
};
const std::vector<CompSource>& compSources();
const std::vector<int>& releaseDenominators();

//==============================================================================
// Converters
double dbToGain (double db);
double gainToDb (double gain);             // gain > 0
double qToOctaves (double q);              // q > 0
double octavesToQ (double octaves);        // octaves > 0
struct Band { double lowHz, highHz, widthHz; };
Band bandForQ (double centreHz, double q);
double msToSamples (double ms, double sampleRate);
double samplesToMs (double samples, double sampleRate);
double speedOfSound (double celsius);      // m/s
double metresToMs (double metres, double celsius);
double msToMetres (double ms, double celsius);

//==============================================================================
// Loop / bars
double barMs (double bpm, double beatsPerBar, double beatUnit);
double loopMs (double bpm, double bars, double beatsPerBar, double beatUnit);

//==============================================================================
std::string fmt (double v, int decimals);
std::string fmtMs (double ms);   // adaptive precision
std::string fmtHz (double hz);
std::string fmtCents (double c); // "+12.3 ct"
} // namespace calc
