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

    // Click-to-copy: what a click copies, and the COPIED flash on a Delay table cell
    int copyFailures = 0;
    for (auto [shown, want] : { std::pair { "462.00 Hz", "462.00" }, { "+0.84 st", "0.84" }, { "-15.5 ct", "-15.5" },
                                { "29.297 (1/64)", "29.297" }, { "1845.7", "1845.7" }, { "1/8D", "1/8D" },
                                { "A#4", "A#4" }, { "12.1 x 9.6 x 7.6 m", "12.1 x 9.6 x 7.6 m" }, { "x1.0500  (105.00 %)", "x1.0500  (105.00 %)" } })
        if (ui::copyableValue (shown) != want)
        {
            ++copyFailures;
            std::printf ("FAIL copyableValue (%s) = %s\n", shown, ui::copyableValue (shown).toRawUTF8());
        }
    ed->selectModule (1);
    for (auto* child : ed->getModule (1)->getChildren())
        if (auto* table = dynamic_cast<ui::LedTable*> (child))
        {
            const auto pos = juce::Point<float> ((float) table->getWidth() * 0.2f, 22.0f + 4.0f + 3.5f * 30.0f);
            const auto now = juce::Time::getCurrentTime();
            table->mouseDown (juce::MouseEvent (juce::Desktop::getInstance().getMainMouseSource(), pos, {},
                                                juce::MouseInputSource::defaultPressure, 0, 0, 0, 0, table, table, now, pos, now, 1, false));
            save (*ed, dir, "2c_copied");
        }
    ed->selectModule (3);

    // "How to use" panels
    pitch->showHelp (true);
    save (*ed, dir, "4f_stretch_help");
    pitch->selectTab (3);
    save (*ed, dir, "4g_harmonics_help");
    pitch->showHelp (false);
    for (int i = 0; i < MuculatorEditor::numModules; ++i)
    {
        if (i == 3) continue;
        ed->selectModule (i);
        ed->getModule (i)->showHelp (true);
        save (*ed, dir, juce::String (i + 1) + "h_help");
        ed->getModule (i)->showHelp (false);
    }
    ed->setBpmTextFor ("pitch", "120");
    ed->selectModule (3);
    pitch->selectTab (1);

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

    // Harmonics with a square wave, 64 harmonics, flats and A4 = 432 Hz (rebuilds the modules from state)
    auto pitchState = proc.state.getOrCreateChildWithName ("pitch", nullptr);
    for (auto [prop, value] : { std::pair<const char*, juce::var> { "hmWave", 2 }, { "hmSet", 1 }, { "hmCount", 2 }, { "flats", true },
                                { "a4", "432" }, { "tab", 3 } })
        pitchState.setProperty (prop, value, nullptr);
    proc.state.setProperty ("module", 3, nullptr);
    proc.state.setProperty ("loadCount", (int) proc.state.getProperty ("loadCount", 0) + 1, nullptr);
    save (*ed, dir, "4h_harmonics_square_flats_432");

    // HOST SYNC on while the host is not running the plugin (no audio blocks here) -> footer hint
    proc.state.setProperty ("hostSync", true, nullptr);
    save (*ed, dir, "8_host_paused");
    proc.state.setProperty ("hostSync", false, nullptr);

    // state round-trip
    juce::MemoryBlock mb;
    proc.getStateInformation (mb);
    proc.setStateInformation (mb.getData(), (int) mb.getSize());
    std::printf ("state bytes: %d\n", (int) mb.getSize());

    ed.reset();
    return copyFailures == 0 ? 0 : 1;
}
