// Renders the editor in several states to PNG files for visual review.
#include "PluginEditor.h"

static void save (juce::Component& c, const juce::File& dir, const juce::String& name)
{
    auto img = c.createComponentSnapshot (c.getLocalBounds(), true, 1.0f);
    juce::FileOutputStream out (dir.getChildFile (name + ".png"));
    out.setPosition (0);
    out.truncate();
    juce::PNGImageFormat().writeImageToStream (img, out);
    std::printf ("wrote %s.png\n", name.toRawUTF8());
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File dir (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    dir.createDirectory();

    MuculatorProcessor proc;
    std::unique_ptr<MuculatorEditor> ed (dynamic_cast<MuculatorEditor*> (proc.createEditor()));

    const char* names[] = { "1_reverb", "2_delay", "3_comp", "4_pitch", "5_convert", "6_loop" };
    for (int i = 0; i < MuculatorEditor::numModules; ++i)
    {
        ed->selectModule (i);
        save (*ed, dir, names[i]);
    }

    ed->selectModule (3);
    auto* pitch = dynamic_cast<PitchModule*> (ed->getModule (3));
    const char* tabs[] = { "4a_transpose", "4b_stretch_ok", "4c_freq", "4d_harmonics" };
    for (int t = 0; t < 4; ++t)
    {
        pitch->selectTab (t);
        save (*ed, dir, tabs[t]);
    }

    // Stretch: A4 sample at 120 BPM played at 126 BPM -> microtone warning
    pitch->selectTab (1);
    ed->setBpmTextFor ("pitch", "126");
    save (*ed, dir, "4e_stretch_microtone");

    // Invalid inputs must not crash
    for (auto bad : { "0", "", "abc", "-5", "1e999", "99999" })
    {
        ed->setBpmTextFor ("pitch", bad);
        for (int i = 0; i < MuculatorEditor::numModules; ++i)
        {
            ed->selectModule (i);
            if (i == 3)
                for (int t = 0; t < 4; ++t) pitch->selectTab (t);
        }
    }
    ed->setBpmTextFor ("pitch", "0");
    ed->selectModule (0);
    save (*ed, dir, "7_invalid_bpm");

    // state round-trip
    juce::MemoryBlock mb;
    proc.getStateInformation (mb);
    proc.setStateInformation (mb.getData(), (int) mb.getSize());
    std::printf ("state bytes: %d\n", (int) mb.getSize());

    ed.reset();
    return 0;
}
