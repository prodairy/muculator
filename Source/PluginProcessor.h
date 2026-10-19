#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

/** Pass-through processor. Holds UI state (ValueTree) and reports host tempo / sample rate. */
class MuculatorProcessor final : public juce::AudioProcessor
{
public:
    /** standalone: running as its own app, with no host. Defaults to how JUCE loaded the plugin. */
    explicit MuculatorProcessor (bool standalone = juce::PluginHostType::getPluginLoadedAs()
                                                       == juce::AudioProcessor::wrapperType_Standalone);
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

    /** The standalone app: no host tempo, and the sample rate is only the audio device's. */
    bool isStandalone() const noexcept { return standalone; }

    /** 0 when the host has not reported a tempo. */
    double getHostBpm() const noexcept { return hostBpm.load(); }
    double getCurrentSampleRate() const noexcept { return currentSampleRate.load(); }
    /** Milliseconds since the host last called processBlock (huge if it never did). */
    juce::uint32 msSinceLastBlock() const noexcept { return juce::Time::getMillisecondCounter() - lastBlockMs.load(); }

    juce::ValueTree state { "MUCULATOR" };

private:
    static BusesProperties buses (bool isStandaloneApp);
    template <typename Sample> void readHost (juce::AudioBuffer<Sample>&);

    const bool standalone;
    std::atomic<double> hostBpm { 0.0 };
    std::atomic<double> currentSampleRate { 48000.0 };
    std::atomic<juce::uint32> lastBlockMs { juce::Time::getMillisecondCounter() - 0x7fffffffu };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MuculatorProcessor)
};
