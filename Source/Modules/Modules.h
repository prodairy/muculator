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
    ui::LedTable table;
    ui::LedReadout summary;
    juce::Rectangle<int> tableArea, summaryArea;
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
    ui::ButtonGroup sources;
    ui::LedReadout result;
    ui::LedTable releaseTable;
    ui::WarningPanel advice;
    juce::Rectangle<int> sourceArea, resultArea, tableArea;
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
    void refreshTranspose (Validator&);
    void refreshStretch (Validator&);
    void refreshFreq (Validator&);
    void refreshHarmonics (Validator&);
    void updateVisibility();

    ui::ButtonGroup tabs;
    int tab = 0;

    // transpose
    ui::NotePicker tpNote { "Source note" };
    ui::LedField tpSemis { "Semitones", "st" }, tpCents { "Cents", "ct" };
    ui::LedReadout tpOut;

    // stretch
    ui::ButtonGroup stMode;
    ui::NotePicker stNote { "Old note" };
    ui::LedField stFreq { "Old frequency", "Hz" };
    ui::LedField stOldBpm { "Old BPM" }, stNewBpm { "New BPM" };
    ui::LedReadout stOut;
    ui::WarningPanel microtone;

    // freq -> note
    ui::LedField fnFreq { "Frequency", "Hz" };
    ui::LedReadout fnOut;

    // harmonics
    ui::ButtonGroup hmMode;
    ui::NotePicker hmNote { "Fundamental note" };
    ui::LedField hmFreq { "Fundamental", "Hz" };
    ui::LedTable hmTable;

    juce::Rectangle<int> inputArea, outputArea;
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
    ui::IllumButton tHostRate { "HOST SR", ui::col::capGreen, false };
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

private:
    void layoutContent (juce::Rectangle<int>) override;
    ui::LedField bars { "Bars" }, beats { "Beats / bar" }, rate { "Sample rate", "Hz" };
    juce::ComboBox beatUnit;
    ui::IllumButton hostRate { "HOST SR", ui::col::capGreen, false };
    ui::LedReadout out;
    ui::LedTable table;
    juce::Rectangle<int> inputArea, outArea, tableArea;
};
