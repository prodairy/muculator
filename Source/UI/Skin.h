#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ui
{
/** Lit colour roles of an IllumButton. The skin decides the actual colour, so buttons follow a skin change. */
enum class Cap { Red, Yellow, Green, Blue, White, Orange };

/** Everything that decides how Muculator looks. Skins are JSON files with the .muskin extension
    (format: docs/SKINS.md). Anything a file leaves out or gets wrong keeps its value from the base skin. */
struct Skin
{
    enum class ButtonStyle { Glow, Flat, Gloss, Glass };
    enum class Texture { Brushed, Gradient, Flat };

    juce::String id, name, author, description;

    // Panels and text
    juce::Colour panel      { 0xff3a3d42 };   // window background
    juce::Colour panelDark  { 0xff2a2c30 };   // top bar, help panel, menus
    juce::Colour panelEdge  { 0xff1a1b1e };   // outlines, button bezels
    juce::Colour recess     { 0xff202225 };   // inside of the framed sections
    juce::Colour accent     { 0xffe0342a };   // stripe under the top bar
    juce::Colour text       { 0xffe6e4dc };   // labels on panels
    juce::Colour textDim    { 0xff9a9890 };   // secondary labels on panels
    juce::Colour highlight  { 0xfff2c200 };   // help headings and the HOST badge

    // LED screens
    juce::Colour screen     { 0xff0a0b0c };
    juce::Colour screenEdge { 0xff000000 };
    juce::Colour screenText { 0xff9a9890 };   // labels and column headers on screens
    juce::Colour ledRed     { 0xffff3b2e };   // errors, out of tune
    juce::Colour ledAmber   { 0xffffb020 };   // values
    juce::Colour ledGreen   { 0xff3dff86 };   // results, selected rows
    juce::Colour ledBlue    { 0xff48a8ff };   // info, help

    // Buttons
    juce::Colour capRed     { 0xffe0342a };
    juce::Colour capYellow  { 0xfff2c200 };
    juce::Colour capGreen   { 0xff27c24c };
    juce::Colour capBlue    { 0xff2f86ff };
    juce::Colour capWhite   { 0xffeeeeea };
    juce::Colour capOrange  { 0xffff7a1a };
    juce::Colour buttonFace { 0xff2c2e32 };   // unlit face (flat, gloss and glass styles)
    juce::Colour buttonText { 0xffe6e4dc };   // unlit text
    juce::Colour litText    { 0xff141414 };   // text on a lit button

    // Shapes
    ButtonStyle buttonStyle = ButtonStyle::Glow;
    Texture texture = Texture::Brushed;
    float buttonCorner = 3.5f, sectionCorner = 5.0f, screenCorner = 3.0f;
    float accentStripe = 3.0f;    // height of the top bar stripe, 0 = none
    bool screws = true, screenGlare = true;

    // Fonts (empty = system default)
    juce::String labelFont, ledFont;

    juce::Colour cap (Cap) const;

    /** The built-in default; identical to Skins/console.muskin. */
    static Skin console();

    /** Reads a .muskin file. Unknown or invalid values keep the base skin's value and are listed in problems.
        Returns false only when the text is not a JSON object at all. */
    static bool parse (const juce::String& json, const Skin& base, Skin& result, juce::StringArray& problems);

    /** Writes every field, so the result is a complete template to edit. */
    juce::String toJson() const;
};

/** The skin every Muculator window is drawn with right now. */
const Skin& skin();

/** Finds the built-in and user skins, remembers the choice on this computer and tells open windows to redraw. */
class SkinManager final : public juce::ChangeBroadcaster
{
public:
    SkinManager();

    struct Entry
    {
        juce::String id;          // built-in: "console"; user file: "user:<file name>"
        juce::String name;
        juce::File file;          // empty for built-in skins
        juce::StringArray problems;
        bool readable = true;     // false when the file is not valid JSON
    };

    const std::vector<Entry>& builtIns() const { return builtInEntries; }
    const std::vector<Entry>& userSkins() const { return userEntries; }
    juce::String currentId() const { return current; }

    /** Applies a skin to every open window. Unknown ids fall back to Console. */
    void select (const juce::String& id, bool remember = true);
    /** Scans the skins folder again and re-applies the current skin (picks up edits to its file). */
    void reload();
    /** Saves the current skin as a complete, editable file in the skins folder and returns it. */
    juce::File exportCurrent();

    /** Where user skins live: .../Muculator/Skins in the user's application data folder. */
    static juce::File skinsFolder();
    /** Reads a built-in skin by id (console, pattern, session, modular, studio-pro). */
    static bool loadBuiltIn (const juce::String& id, Skin& result, juce::StringArray& problems);
    static const juce::StringArray& builtInIds();

private:
    void scan();
    std::vector<Entry> builtInEntries, userEntries;
    juce::String current { "console" };
};
} // namespace ui
