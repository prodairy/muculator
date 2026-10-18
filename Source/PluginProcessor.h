#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

/** Pass-through processor. Holds UI state (ValueTree) and reports host tempo / sample rate. */
class MuculatorProcessor final : public juce::AudioProcessor
{
public:
    MuculatorProcessor();
    ~MuculatorProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    bool supportsDoublePrecisionProcessing() const override { return true; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Muculator"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** 0 when the host has not reported a tempo. */
    double getHostBpm() const noexcept { return hostBpm.load(); }
    double getCurrentSampleRate() const noexcept { return currentSampleRate.load(); }

    juce::ValueTree state { "MUCULATOR" };

private:
    template <typename Sample> void readHost (juce::AudioBuffer<Sample>&);

    std::atomic<double> hostBpm { 0.0 };
    std::atomic<double> currentSampleRate { 48000.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MuculatorProcessor)
};
