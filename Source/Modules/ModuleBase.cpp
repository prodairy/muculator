#include "ModuleBase.h"

ModuleBase::ModuleBase (const juce::String& i, const juce::String& t, ModuleContext& c)
    : ctx (c), id (i), title (t)
{
    addChildComponent (inputWarning);
    addAndMakeVisible (helpButton);
    addChildComponent (help);
    helpButton.setTooltip ("How to use this module");
    help.setFooter ("HOST SYNC reads the tempo only while the host runs the plugin. Some hosts pause plugins on tracks "
                    "that are not playing. In Logic, put Muculator on the Stereo Out; elsewhere, on the master channel.");
    helpButton.onClick = [this] { showHelp (helpButton.getToggleState()); };
    help.onClose = [this] { showHelp (false); };
}

void ModuleBase::showHelp (bool shouldShow)
{
    helpButton.setToggleState (shouldShow, juce::dontSendNotification);
    if (shouldShow)
    {
        help.setContent (title, helpText());
        help.setVisible (true);
        help.toFront (true);
    }
    else
    {
        help.setVisible (false);
    }
}

void ModuleBase::updateHelp()
{
    if (help.isVisible())
        help.setContent (title, helpText());
}

void ModuleBase::showErrors (const juce::StringArray& errors)
{
    if (errors.isEmpty())
        inputWarning.clear();
    else
        inputWarning.setMessage ({}, "INVALID INPUT: " + errors.joinIntoString ("  |  "), ui::WarningPanel::Level::Error);
}

void ModuleBase::paint (juce::Graphics& g)
{
    auto header = getLocalBounds().removeFromTop (30);
    g.setColour (ui::skin().text);
    g.setFont (ui::labelFont (17.0f));
    g.drawText (title.toUpperCase(), header.withTrimmedLeft (4), juce::Justification::centredLeft);

    if (headerBadge.isNotEmpty())
    {
        auto badge = header.removeFromRight (190).reduced (0, 3);  // resized() leaves 200 px for it
        ui::drawScreen (g, badge.toFloat());
        g.setColour (ui::skin().ledGreen);
        g.setFont (ui::ledFont (14.0f));
        g.drawText (headerBadge, badge, juce::Justification::centred);
    }
}

void ModuleBase::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop (30);
    header.removeFromLeft (200);
    if (headerBadge.isNotEmpty())
        header.removeFromRight (200);
    helpButton.setBounds (header.removeFromRight (34).reduced (2, 3));
    inputWarning.setBounds (header.reduced (6, 0));
    r.removeFromTop (6);
    help.setBounds (r);
    layoutContent (r);
}
