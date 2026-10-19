#pragma once

#include "PluginProcessor.h"
#include "Modules/Modules.h"

class MuculatorEditor final : public juce::AudioProcessorEditor,
                              public ModuleContext,
                              private juce::Timer,
                              private juce::ValueTree::Listener,
                              private juce::ChangeListener
{
public:
    explicit MuculatorEditor (MuculatorProcessor&);
    ~MuculatorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

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
    /** The skin changed (here or in another Muculator window): redraw everything with it. */
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void showSkinMenu();
    void buildModules();
    void bpmChanged();
    /** Moves the BPM by whole steps of stepSize, landing on the step grid (120.5 + 1 -> 121). */
    void nudgeBpm (int steps, double stepSize);
    void tap();
    void setChained (bool);
    void updateTopBar();
    /** Copies the host tempo into the visible module's BPM when HOST SYNC is on. */
    void applyHostTempo (bool force);
    /** The host has not called processBlock for a second (paused track, or never started). */
    bool hostPaused() const;
    const juce::String& currentId() const;

    MuculatorProcessor& proc;
    juce::ValueTree state;
    juce::SharedResourcePointer<ui::SkinManager> skins; // before lnf: loads the saved skin
    ui::ConsoleLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    ui::LedField bpmField { "BPM" };
    ui::IllumButton minus { "-", ui::Cap::White, false }, plus { "+", ui::Cap::White, false };
    ui::IllumButton tapButton { "TAP", ui::Cap::Red, false };
    ui::ChainButton chainButton;
    ui::IllumButton hostSync { "HOST SYNC", ui::Cap::Green, true };
    ui::IllumButton skinButton { "SKIN", ui::Cap::White, false };
    ui::ButtonGroup moduleButtons;

    std::vector<std::unique_ptr<ModuleBase>> modules;
    int current = 0;
    std::vector<double> tapTimes;
    double lastHostBpm = -1;
    bool wasPaused = false;
    /** Logic pauses plugins on tracks that are not playing; the hints name the fix for it. */
    const bool inLogic = juce::PluginHostType().isLogic();
    double lastSampleRate = 0;
    juce::Rectangle<int> topBar, logoArea, hostArea, moduleStrip, contentArea, footer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MuculatorEditor)
};
