#include "PluginEditor.h"

using namespace ui;

namespace
{
const juce::StringArray moduleIds   { "reverb", "delay", "comp", "pitch", "convert", "loop" };
const juce::StringArray moduleNames { "REVERB", "DELAY", "COMP", "PITCH", "CONVERT", "LOOP" };
const juce::Identifier globalBpmId { "globalBpm" }, chainId { "chain" }, hostSyncId { "hostSync" },
                       moduleId { "module" }, bpmId { "bpm" };

juce::String bpmToText (double bpm)
{
    auto s = juce::String (bpm, 2);
    return s.trimCharactersAtEnd ("0").trimCharactersAtEnd (".");
}
} // namespace

MuculatorEditor::MuculatorEditor (MuculatorProcessor& p)
    : AudioProcessorEditor (p), proc (p), state (p.state),
      moduleButtons (moduleNames, col::capWhite, 1, false)
{
    setLookAndFeel (&lnf);

    if (! state.hasProperty (globalBpmId)) state.setProperty (globalBpmId, "120", nullptr);
    if (! state.hasProperty (chainId))     state.setProperty (chainId, true, nullptr);
    if (! state.hasProperty (hostSyncId))  state.setProperty (hostSyncId, false, nullptr);

    bpmField.setFontHeight (30.0f);
    bpmField.setCaption ("Tempo  BPM");
    bpmField.onChange = [this] { setBpmTextFor (currentId(), bpmField.getText()); };
    minus.onClick = [this] { nudgeBpm (-1.0); };
    plus.onClick  = [this] { nudgeBpm (1.0); };
    minus.setTooltip ("-1 BPM");
    plus.setTooltip ("+1 BPM");
    tapButton.onClick = [this] { tap(); };
    tapButton.setTooltip ("Tap at least twice in tempo");
    chainButton.setToggleState ((bool) state[chainId], juce::dontSendNotification);
    chainButton.onClick = [this] { setChained (chainButton.getToggleState()); };
    hostSync.setToggleState ((bool) state[hostSyncId], juce::dontSendNotification);
    hostSync.setTooltip ("Follow the host (DAW) tempo");
    hostSync.onClick = [this]
    {
        state.setProperty (hostSyncId, hostSync.getToggleState(), nullptr);
        applyHostTempo (true);
        updateTopBar();
        repaint();
    };

    for (auto* c : std::initializer_list<juce::Component*> { &bpmField, &minus, &plus, &tapButton, &chainButton, &hostSync, &moduleButtons })
        addAndMakeVisible (c);

    moduleButtons.onSelect = [this] (int i) { selectModule (i); };

    buildModules();
    selectModule ((int) state.getProperty (moduleId, 3));
    state.addListener (this);

    setSize (1000, 680);
    startTimerHz (15);
}

MuculatorEditor::~MuculatorEditor()
{
    state.removeListener (this);
    setLookAndFeel (nullptr);
}

void MuculatorEditor::buildModules()
{
    for (auto& m : modules)
        removeChildComponent (m.get());
    modules.clear();
    modules.push_back (std::make_unique<ReverbModule> (*this));
    modules.push_back (std::make_unique<DelayModule> (*this));
    modules.push_back (std::make_unique<CompressorModule> (*this));
    modules.push_back (std::make_unique<PitchModule> (*this));
    modules.push_back (std::make_unique<ConverterModule> (*this));
    modules.push_back (std::make_unique<LoopModule> (*this));
    for (auto& m : modules)
        addChildComponent (*m);
}

void MuculatorEditor::valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& prop)
{
    if (tree == state && prop == juce::Identifier ("loadCount"))
    {
        // Host restored a saved state while the editor is open: rebuild from it.
        chainButton.setToggleState ((bool) state[chainId], juce::dontSendNotification);
        hostSync.setToggleState ((bool) state[hostSyncId], juce::dontSendNotification);
        buildModules();
        selectModule ((int) state.getProperty (moduleId, 3));
    }
}

const juce::String& MuculatorEditor::currentId() const { return moduleIds.getReference (current); }

void MuculatorEditor::selectModule (int index)
{
    current = juce::jlimit (0, numModules - 1, index);
    state.setProperty (moduleId, current, nullptr);
    moduleButtons.setSelected (current, false);
    for (int i = 0; i < (int) modules.size(); ++i)
        modules[(size_t) i]->setVisible (i == current);
    resized();
    applyHostTempo (true); // CHAIN off: the newly shown module must follow the host too
    modules[(size_t) current]->refresh();
    updateTopBar();
    repaint();
}

//==============================================================================
juce::ValueTree MuculatorEditor::moduleState (const juce::String& id)
{
    return state.getOrCreateChildWithName (juce::Identifier (id), nullptr);
}

bool MuculatorEditor::isChained() const { return (bool) state[chainId]; }

double MuculatorEditor::hostSampleRate() const
{
    const double sr = proc.getCurrentSampleRate();
    return sr > 0 ? sr : 48000.0;
}

juce::String MuculatorEditor::bpmTextFor (const juce::String& id) const
{
    if (isChained())
        return state[globalBpmId].toString();
    const auto child = state.getChildWithName (juce::Identifier (id));
    return child.isValid() && child.hasProperty (bpmId) ? child[bpmId].toString() : state[globalBpmId].toString();
}

calc::Parsed MuculatorEditor::bpmFor (const juce::String& id) const
{
    return calc::parseBpm (bpmTextFor (id).toStdString());
}

void MuculatorEditor::setBpmTextFor (const juce::String& id, const juce::String& text)
{
    if (isChained())
        state.setProperty (globalBpmId, text, nullptr);
    else
        moduleState (id).setProperty (bpmId, text, nullptr);
    bpmChanged();
}

void MuculatorEditor::bpmChanged()
{
    updateTopBar();
    for (auto& m : modules)
        m->refresh();
}

void MuculatorEditor::setChained (bool on)
{
    if (on)
    {
        // Adopt the BPM currently on screen as the shared one.
        const auto shown = bpmTextFor (currentId());
        state.setProperty (chainId, true, nullptr);
        state.setProperty (globalBpmId, shown, nullptr);
    }
    else
    {
        // Each module starts from the shared value, then can diverge.
        const auto shared = state[globalBpmId].toString();
        state.setProperty (chainId, false, nullptr);
        for (auto& id : moduleIds)
            moduleState (id).setProperty (bpmId, shared, nullptr);
    }
    bpmChanged();
}

void MuculatorEditor::nudgeBpm (double delta)
{
    if ((bool) state[hostSyncId])
        return;
    const auto p = bpmFor (currentId());
    const double base = p.ok() ? std::round (*p.value) : 120.0;
    setBpmTextFor (currentId(), bpmToText (juce::jlimit (calc::kMinBpm, calc::kMaxBpm, base + delta)));
}

void MuculatorEditor::tap()
{
    tapButton.flash();
    const double now = juce::Time::getMillisecondCounterHiRes();
    if (! tapTimes.empty() && now - tapTimes.back() > 2000.0)
        tapTimes.clear();
    tapTimes.push_back (now);
    if (tapTimes.size() > 8)
        tapTimes.erase (tapTimes.begin());
    if (tapTimes.size() < 2)
        return;

    const double avg = (tapTimes.back() - tapTimes.front()) / (double) (tapTimes.size() - 1);
    if (avg <= 0)
        return;
    if ((bool) state[hostSyncId])
    {
        hostSync.setToggleState (false, juce::dontSendNotification);
        state.setProperty (hostSyncId, false, nullptr);
    }
    setBpmTextFor (currentId(), bpmToText (juce::jlimit (calc::kMinBpm, calc::kMaxBpm, 60000.0 / avg)));
}

void MuculatorEditor::applyHostTempo (bool force)
{
    const double host = proc.getHostBpm();
    if (! (bool) state[hostSyncId] || host <= 0)
        return;
    if (! force && std::abs (host - lastHostBpm) <= 1e-4)
        return;
    lastHostBpm = host;
    if (bpmTextFor (currentId()) != bpmToText (host))
        setBpmTextFor (currentId(), bpmToText (host));
}

bool MuculatorEditor::hostIdle() const
{
    return proc.getHostBpm() > 0 && proc.msSinceLastBlock() > 1000;
}

void MuculatorEditor::timerCallback()
{
    applyHostTempo (false);
    repaint (hostArea);
    if (const bool idle = hostIdle(); idle != wasIdle)
    {
        wasIdle = idle;
        repaint (footer);
    }
}

void MuculatorEditor::updateTopBar()
{
    const bool synced = (bool) state[hostSyncId];
    if (! bpmField.isEditing())
        bpmField.setText (bpmTextFor (currentId()));
    bpmField.setInvalid (! bpmFor (currentId()).ok());
    bpmField.setLocked (synced, "HOST");
    bpmField.setCaption (isChained() ? "Tempo BPM  (all modules)" : "Tempo BPM  (" + moduleNames[current] + " only)");
    minus.setEnabled (! synced);
    plus.setEnabled (! synced);
    repaint (topBar);
}

//==============================================================================
void MuculatorEditor::paint (juce::Graphics& g)
{
    // console panel
    juce::ColourGradient bg (col::panel.brighter (0.06f), 0, 0, col::panel.darker (0.25f), 0, (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();
    juce::Random rng (7);
    for (int y = 0; y < getHeight(); y += 2) // brushed metal lines
    {
        g.setColour (juce::Colours::white.withAlpha (0.006f + rng.nextFloat() * 0.012f));
        g.drawHorizontalLine (y, 0, (float) getWidth());
    }

    // top bar
    g.setColour (col::panelDark);
    g.fillRect (topBar);
    g.setColour (col::panelEdge);
    g.drawHorizontalLine (topBar.getBottom(), 0, (float) getWidth());
    g.setColour (col::capRed);
    g.fillRect (topBar.getX(), topBar.getBottom() - 3, getWidth(), 3);

    for (auto pt : { juce::Point<float> (10, 10), juce::Point<float> ((float) getWidth() - 10, 10),
                     juce::Point<float> (10, (float) getHeight() - 10), juce::Point<float> ((float) getWidth() - 10, (float) getHeight() - 10) })
        drawScrew (g, pt, 4.5f);

    // logo
    auto logo = logoArea;
    g.setColour (col::silkscreen);
    g.setFont (labelFont (27.0f));
    g.drawText ("MUCULATOR", logo.removeFromTop (logo.getHeight() * 60 / 100), juce::Justification::bottomLeft);
    g.setColour (col::silkDim);
    g.setFont (labelFont (10.0f));
    g.drawText ("PRODUCTION CALCULATOR  /  PASS-THROUGH", logo, juce::Justification::topLeft);

    // host tempo readout
    auto h = hostArea;
    g.setFont (labelFont (10.5f));
    g.setColour (col::silkscreen.withAlpha (0.85f));
    g.drawText ("HOST TEMPO", h.removeFromTop (15), juce::Justification::centredLeft);
    h.removeFromTop (1);
    drawScreen (g, h.toFloat());
    const double host = proc.getHostBpm();
    const bool synced = (bool) state[hostSyncId];
    const bool idle = hostIdle();
    auto ledArea = h.removeFromLeft (20).toFloat();
    drawLed (g, ledArea.withSizeKeepingCentre (8, 8).translated (4, 0), col::ledGreen, synced && host > 0 && ! idle);
    if (idle)
    {
        // Last tempo stays visible but dimmed; the host is not running the plugin right now.
        g.setFont (ledFont (11.0f));
        g.setColour (col::ledAmber);
        g.drawText ("IDLE", h.removeFromRight (40).withTrimmedRight (6), juce::Justification::centredRight);
    }
    g.setFont (ledFont (17.0f));
    g.setColour (host <= 0 ? col::silkDim : idle ? col::ledGreen.withAlpha (0.35f) : col::ledGreen);
    g.drawText (host > 0 ? juce::String (host, 2) : juce::String ("-- no host --"), h.reduced (6, 0), juce::Justification::centred);

    // module strip label
    g.setColour (col::silkDim);
    g.setFont (labelFont (10.0f));
    g.drawText ("MODULE", moduleStrip.withWidth (60), juce::Justification::centredLeft);

    // footer
    if (synced && idle)
    {
        g.setColour (col::ledAmber);
        g.setFont (labelFont (10.5f));
        g.drawText ("HOST IDLE: some hosts pause plugins on silent tracks. Select or record-arm this track, "
                    "or press Play on a track that has a region.",
                    footer, juce::Justification::centredLeft);
        return;
    }
    g.setColour (col::silkDim);
    g.setFont (labelFont (10.0f, false));
    g.drawText ("A4 = 440 Hz  |  C4 = MIDI 60  |  audio passes through untouched", footer, juce::Justification::centredLeft);
    g.drawText (isChained() ? "CHAIN ON: one BPM for every module" : "CHAIN OFF: each module keeps its own BPM",
                footer, juce::Justification::centredRight);
}

void MuculatorEditor::resized()
{
    auto r = getLocalBounds();
    topBar = r.removeFromTop (78);
    auto t = topBar.reduced (20, 10);
    t.removeFromBottom (2);
    logoArea = t.removeFromLeft (290);

    hostArea = t.removeFromRight (150);
    t.removeFromRight (12);
    hostSync.setBounds (t.removeFromRight (86).withTrimmedTop (14));
    t.removeFromRight (12);
    chainButton.setBounds (t.removeFromRight (72).withTrimmedTop (14));
    t.removeFromRight (12);
    tapButton.setBounds (t.removeFromRight (72).withTrimmedTop (14));
    t.removeFromRight (16);
    plus.setBounds (t.removeFromRight (34).withTrimmedTop (14));
    t.removeFromRight (6);
    minus.setBounds (t.removeFromRight (34).withTrimmedTop (14));
    t.removeFromRight (8);
    bpmField.setBounds (t.removeFromRight (juce::jmin (220, t.getWidth())));

    r.reduce (16, 0);
    r.removeFromTop (10);
    moduleStrip = r.removeFromTop (40);
    moduleButtons.setBounds (moduleStrip.withTrimmedLeft (60));
    r.removeFromTop (12);
    footer = r.removeFromBottom (24);
    r.removeFromBottom (6);
    contentArea = r;
    for (auto& m : modules)
        m->setBounds (contentArea);
}
