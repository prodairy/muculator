#pragma once

#include "PluginProcessor.h"
#include "Modules/Modules.h"

class MuculatorEditor final : public juce::AudioProcessorEditor,
                              public ModuleContext,
                              private juce::Timer,
                              private juce::ValueTree::Listener
{
public:
    explicit MuculatorEditor (MuculatorProcessor&);
    ~MuculatorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void selectModule (int index);
    ModuleBase* getModule (int index) { return modules[(size_t) index].get(); }
    static constexpr int numModules = 6;

    // ModuleContext
    calc::Parsed bpmFor (const juce::String& moduleId) const override;
    juce::String bpmTextFor (const juce::String& moduleId) const override;
    void setBpmTextFor (const juce::String& moduleId, const juce::String& text) override;
    bool isChained() const override;
    double hostSampleRate() const override;
    juce::ValueTree moduleState (const juce::String& moduleId) override;

private:
    void timerCallback() override;
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
    void buildModules();
    void bpmChanged();
    void nudgeBpm (double delta);
    void tap();
    void setChained (bool);
    void updateTopBar();
    /** Copies the host tempo into the visible module's BPM when HOST SYNC is on. */
    void applyHostTempo (bool force);
    /** The host reported a tempo before but has stopped calling the plugin. */
    bool hostIdle() const;
    const juce::String& currentId() const;

    MuculatorProcessor& proc;
    juce::ValueTree state;
    ui::ConsoleLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    ui::LedField bpmField { "BPM" };
    ui::IllumButton minus { "-", ui::col::capWhite, false }, plus { "+", ui::col::capWhite, false };
    ui::IllumButton tapButton { "TAP", ui::col::capRed, false };
    ui::ChainButton chainButton;
    ui::IllumButton hostSync { "HOST SYNC", ui::col::capGreen, true };
    ui::ButtonGroup moduleButtons;

    std::vector<std::unique_ptr<ModuleBase>> modules;
    int current = 0;
    std::vector<double> tapTimes;
    double lastHostBpm = -1;
    bool wasIdle = false;
    juce::Rectangle<int> topBar, logoArea, hostArea, moduleStrip, contentArea, footer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MuculatorEditor)
};
