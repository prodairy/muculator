#pragma once

#include "ModuleBase.h"

//==============================================================================
class ReverbModule final : public ModuleBase
{
public:
    explicit ReverbModule (ModuleContext&);
    void refresh() override;
    void paint (juce::Graphics&) override;

private:
    void layoutContent (juce::Rectangle<int>) override;
    juce::String helpText() const override;
    ui::ButtonGroup styles;
    ui::LedTable table;
    ui::LedReadout room;
    juce::Rectangle<int> styleArea, tableArea, roomArea;
};

//==============================================================================
class DelayModule final : public ModuleBase
{
public:
    explicit DelayModule (ModuleContext&);
    void refresh() override;
    void paint (juce::Graphics&) override;

private:
    void layoutContent (juce::Rectangle<int>) override;
    juce::String helpText() const override;
    ui::LedTable table;
    ui::LedReadout summary, swingOut;
    ui::LedField swingField { "Swing", "%" };
    juce::Rectangle<int> tableArea, summaryArea, swingArea;
};

//==============================================================================
class CompressorModule final : public ModuleBase
{
public:
    explicit CompressorModule (ModuleContext&);
    void refresh() override;
    void paint (juce::Graphics&) override;

private:
    void layoutContent (juce::Rectangle<int>) override;
    juce::String helpText() const override;
    ui::ButtonGroup sources;
    ui::LedReadout result;
    ui::LedTable releaseTable;
    ui::WarningPanel advice;
    juce::Rectangle<int> sourceArea, resultArea, tableArea;
};

//==============================================================================
/** Instrument frequency ranges on a log axis (20 Hz to 20 kHz) with an optional marker line. */
class FrequencyMap final : public juce::Component, public juce::SettableTooltipClient
{
public:
    /** Hz of the marker line; 0 hides it. */
    void setMarker (double hz) { marker = hz; repaint(); }
    /** Called with the frequency under a click. */
    std::function<void (double)> onClick;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    double marker = 0;
    juce::Rectangle<float> plotArea() const;
    float hzToX (double hz) const;
    double xToHz (float x) const;
};

//==============================================================================
class PitchModule final : public ModuleBase
{
public:
    explicit PitchModule (ModuleContext&);
    void refresh() override;
    void paint (juce::Graphics&) override;
    void selectTab (int);

private:
    void layoutContent (juce::Rectangle<int>) override;
    juce::String helpText() const override;
    void refreshTranspose (Validator&);
    void refreshStretch (Validator&);
    void refreshFreq (Validator&);
    void refreshHarmonics (Validator&);
    void refreshMap (Validator&);
    void refreshTimeStretch (Validator&, double oldBpm, double newBpm);
    void refreshFit (double oldBpm, double oldHz, double newBpm);
    bool timeStretchMode() const { return stKind.getSelected() == 1; }
    void updateVisibility();
    juce::String noteText (int midi) const;
    juce::String noteWithCents (const calc::NoteInfo&) const;


    ui::ButtonGroup tabs;
    int tab = 0;

    // transpose
    ui::NotePicker tpNote { "Source note" };
    ui::LedField tpSemis { "Semitones", "st" }, tpCents { "Cents", "ct" };
    ui::LedReadout tpOut;

    // stretch
    ui::ButtonGroup stKind, stMode, stScale;
    ui::LedField stLength { "Old length", "s" };
    juce::ComboBox stKey;
    ui::LedTable stFit;
    ui::NotePicker stNote { "Old note" };
    ui::LedField stFreq { "Old frequency", "Hz" };
    ui::LedField stOldBpm { "Old BPM" }, stNewBpm { "New BPM" };
    ui::LedReadout stOut;
    ui::WarningPanel microtone;

    // freq -> note
    ui::LedField fnFreq { "Frequency", "Hz" };
    ui::LedReadout fnOut;

    // harmonics
    ui::ButtonGroup hmMode, hmSet, hmWave, hmCount;
    ui::NotePicker hmNote { "Fundamental note" };
    ui::LedField hmFreq { "Fundamental", "Hz" };
    ui::LedTable hmTable;

    // frequency map
    ui::ButtonGroup fmMode;
    ui::NotePicker fmNote { "Marker note" };
    ui::LedField fmFreq { "Marker frequency", "Hz" };
    ui::LedReadout fmOut;
    FrequencyMap fmMap;

    // tuning (shared by every tab)
    ui::LedField a4Field { "A4 reference", "Hz" };
    ui::IllumButton a4Presets { "...", ui::Cap::White, false };
    ui::ButtonGroup spelling;
    double a4 = calc::kA4Hz;
    bool flats = false;

    juce::Rectangle<int> inputArea, outputArea, fitArea;
};

//==============================================================================
class ConverterModule final : public ModuleBase
{
public:
    explicit ConverterModule (ModuleContext&);
    void refresh() override;
    void paint (juce::Graphics&) override;

private:
    void layoutContent (juce::Rectangle<int>) override;
    juce::String helpText() const override;
    enum class Src { None, A, B, C };
    void updateGain (Src);
    void updateEq (Src);
    void updateTime (Src);
    void updateDistance (Src);
    void publishErrors();

    ui::LedField dbField { "Level", "dB" }, gainField { "Linear gain", "x" };
    ui::LedReadout gainOut;
    ui::LedField eqCentre { "Centre", "Hz" }, eqQ { "Q" }, eqOct { "Bandwidth", "oct" };
    ui::LedReadout eqOut;
    ui::LedField tMs { "Time", "ms" }, tHz { "Frequency", "Hz" }, tSamples { "Samples", "smp" }, tRate { "Sample rate", "Hz" };
    ui::IllumButton tHostRate { "HOST SR", ui::Cap::Green, true };
    ui::LedReadout tOut;
    ui::LedField dMetres { "Distance", "m" }, dMs { "Delay", "ms" }, dTemp { "Air temp", "C" };
    ui::LedReadout dOut;

    juce::StringArray gainErr, eqErr, timeErr, distErr;
    juce::Rectangle<int> cells[4];
};

//==============================================================================
class LoopModule final : public ModuleBase
{
public:
    explicit LoopModule (ModuleContext&);
    void refresh() override;
    void paint (juce::Graphics&) override;
    /** 0 = bars > length, 1 = length > BPM. */
    void selectMode (int);

private:
    void layoutContent (juce::Rectangle<int>) override;
    juce::String helpText() const override;
    void refreshLength (Validator&, const calc::Parsed& bpm, double nBars, double nBeats, double unit, double sr);
    void refreshBpm (Validator&, double nBars, double nBeats, double unit, std::optional<double> sr);
    /** The length field in ms, or nothing when it (or the sample rate it needs) is invalid. */
    std::optional<double> lengthMs (Validator&, std::optional<double> sr);
    void lengthUnitChanged (int newUnit);

    ui::ButtonGroup mode, lengthUnit;
    ui::LedField length { "Loop length", "s" };
    ui::LedField bars { "Bars" }, beats { "Beats / bar" }, rate { "Sample rate", "Hz" };
    juce::ComboBox beatUnit;
    ui::IllumButton hostRate { "HOST SR", ui::Cap::Green, true };
    ui::IllumButton useBpm { "USE AS TEMPO", ui::Cap::Yellow, false };
    ui::LedReadout out;
    ui::LedTable table;
    double foundBpm = 0;
    int unitIndex = 0;
    juce::Rectangle<int> inputArea, outArea, tableArea;
};
