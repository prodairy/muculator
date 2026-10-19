#include "PluginProcessor.h"
#include "PluginEditor.h"

MuculatorProcessor::MuculatorProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void MuculatorProcessor::prepareToPlay (double sampleRate, int)
{
    if (sampleRate > 0)
        currentSampleRate = sampleRate;
}

bool MuculatorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

template <typename Sample>
void MuculatorProcessor::readHost (juce::AudioBuffer<Sample>& buffer)
{
    // Audio is untouched (pass-through); clear only outputs that have no matching input.
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    lastBlockMs = juce::Time::getMillisecondCounter();
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm(); bpm.hasValue() && *bpm > 0)
                hostBpm = *bpm;
}

void MuculatorProcessor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&)  { readHost (b); }
void MuculatorProcessor::processBlock (juce::AudioBuffer<double>& b, juce::MidiBuffer&) { readHost (b); }

juce::AudioProcessorEditor* MuculatorProcessor::createEditor() { return new MuculatorEditor (*this); }

void MuculatorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void MuculatorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto loaded = juce::ValueTree::fromXml (*xml);
        if (! loaded.hasType (state.getType()))
            return;

        // Merge into the existing tree so an open editor's child references stay valid.
        state.copyPropertiesFrom (loaded, nullptr);
        for (auto child : loaded)
            state.getOrCreateChildWithName (child.getType(), nullptr).copyPropertiesFrom (child, nullptr);

        // Tells an open editor to reload its fields.
        state.setProperty ("loadCount", (int) state.getProperty ("loadCount", 0) + 1, nullptr);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MuculatorProcessor(); }
