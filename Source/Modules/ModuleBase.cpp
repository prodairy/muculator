#include "ModuleBase.h"

ModuleBase::ModuleBase (const juce::String& i, const juce::String& t, ModuleContext& c)
    : ctx (c), id (i), title (t)
{
    addChildComponent (inputWarning);
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
    g.setColour (ui::col::silkscreen);
    g.setFont (ui::labelFont (17.0f));
    g.drawText (title.toUpperCase(), header.withTrimmedLeft (4), juce::Justification::centredLeft);

    if (headerBadge.isNotEmpty())
    {
        auto badge = header.removeFromRight (190).reduced (0, 3);
        ui::drawScreen (g, badge.toFloat());
        g.setColour (ui::col::ledGreen);
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
    inputWarning.setBounds (header.reduced (6, 0));
    r.removeFromTop (6);
    layoutContent (r);
}
