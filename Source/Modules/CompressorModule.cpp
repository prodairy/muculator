#include "Modules.h"

using namespace ui;

static juce::StringArray sourceNames()
{
    juce::StringArray s;
    for (auto& src : calc::compSources()) s.add (src.name);
    return s;
}

CompressorModule::CompressorModule (ModuleContext& c)
    : ModuleBase ("comp", "Compressor Attack / Release", c),
      sources (sourceNames(), col::capOrange, 201, false)
{
    sources.setColumns (4);
    addAndMakeVisible (sources);
    addAndMakeVisible (result);
    addAndMakeVisible (releaseTable);
    addChildComponent (advice);
    result.setLineHeight (24.0f);
    releaseTable.setColumns ({ "RELEASE", "NORMAL ms", "DOTTED ms", "TRIPLET ms" }, { 1.0f, 1.2f, 1.2f, 1.2f });

    sources.setSelected ((int) state().getProperty ("source", 0), false);
    sources.onSelect = [this] (int i) { state().setProperty ("source", i, nullptr); refresh(); };
    refresh();
}

void CompressorModule::refresh()
{
    const auto& list = calc::compSources();
    const auto& src = list[(size_t) juce::jlimit (0, (int) list.size() - 1, sources.getSelected())];
    const auto b = bpm();

    if (! b.ok())
    {
        showErrors ({ str (b.error) });
        releaseTable.setRows ({});
        result.setLines ({
            { "SOURCE", src.name, col::ledGreen },
            { "ATTACK (source)", str (calc::fmt (src.attackRecMs, 0)) + " ms", col::ledAmber, 1.3f },
            { "RELEASE (BPM)", "--", col::ledRed, 1.3f },
        });
        advice.clear();
        return;
    }
    showErrors ({});
    const double v = *b.value;
    const double relMs = calc::noteMs (v, src.releaseDenominator, src.releaseKind);

    result.setLines ({
        { "SOURCE", src.name, col::ledGreen },
        { "ATTACK (source)", str (calc::fmt (src.attackRecMs, 0)) + " ms", col::ledAmber, 1.3f },
        { "  range", str (calc::fmt (src.attackMinMs, 0)) + " - " + str (calc::fmt (src.attackMaxMs, 0)) + " ms", col::ledAmber.withAlpha (0.7f) },
        { "RELEASE (BPM)", str (calc::noteValueLabel (src.releaseDenominator, src.releaseKind)) + " = "
                               + str (calc::fmtMs (relMs)) + " ms", col::ledAmber, 1.3f },
        { "", src.hint, col::silkDim },
    });

    std::vector<juce::StringArray> rows;
    int rec = -1;
    const auto& dens = calc::releaseDenominators();
    for (int i = 0; i < (int) dens.size(); ++i)
    {
        const int d = dens[(size_t) i];
        if (d == src.releaseDenominator) rec = i;
        rows.push_back ({ "1/" + juce::String (d),
                          str (calc::fmtMs (calc::noteMs (v, d))),
                          str (calc::fmtMs (calc::noteMs (v, d, calc::NoteKind::Dotted))),
                          str (calc::fmtMs (calc::noteMs (v, d, calc::NoteKind::Triplet))) });
    }
    releaseTable.setRows (rows);
    releaseTable.setRecommendedRow (rec);
    for (int i = 0; i < (int) dens.size(); ++i)
        if (calc::noteMs (v, dens[(size_t) i]) < src.attackMaxMs * 2.0)
            releaseTable.setRowColour (i, col::ledRed.withAlpha (0.6f));

    if (relMs < src.attackMaxMs * 2.0)
        advice.setMessage ("RELEASE VERY SHORT", "At " + str (calc::fmt (v, 1)) + " BPM the suggested release ("
                               + str (calc::fmtMs (relMs)) + " ms) is close to the attack. Use the next longer note value "
                               "or the compressor's auto-release to avoid distortion.", WarningPanel::Level::Error);
    else if (relMs > 1500.0)
        advice.setMessage ("RELEASE VERY LONG", "At this slow tempo the release may never recover; try a shorter note value.",
                           WarningPanel::Level::Info);
    else
        advice.setMessage ({}, "Release is tied to BPM so gain reduction recovers in time with the groove. "
                               "Attack depends only on the source. Red rows are too short for this source.",
                           WarningPanel::Level::Info);
}

void CompressorModule::paint (juce::Graphics& g)
{
    ModuleBase::paint (g);
    drawSection (g, sourceArea, "Source");
    drawSection (g, resultArea, "Suggested settings");
    drawSection (g, tableArea, "Release by note value");
}

void CompressorModule::layoutContent (juce::Rectangle<int> r)
{
    sourceArea = r.removeFromTop (150);
    r.removeFromTop (10);
    tableArea = r.removeFromRight (r.getWidth() * 45 / 100);
    r.removeFromRight (10);
    resultArea = r;

    sources.setBounds (sourceArea.reduced (8).withTrimmedTop (22));
    auto res = resultArea.reduced (8).withTrimmedTop (22);
    advice.setBounds (res.removeFromBottom (64));
    res.removeFromBottom (8);
    result.setBounds (res);
    releaseTable.setBounds (tableArea.reduced (8).withTrimmedTop (22));
}
