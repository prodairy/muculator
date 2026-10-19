#include "Skin.h"
#include "SkinData.h"
#include <optional>

namespace ui
{
namespace
{
Skin& currentSkin()
{
    static Skin s = Skin::console();
    return s;
}

struct ColourKey { const char* key; juce::Colour Skin::* member; };
const ColourKey colourKeys[] = {
    { "panel", &Skin::panel },           { "panelDark", &Skin::panelDark },   { "panelEdge", &Skin::panelEdge },
    { "recess", &Skin::recess },         { "accent", &Skin::accent },         { "text", &Skin::text },
    { "textDim", &Skin::textDim },       { "highlight", &Skin::highlight },   { "screen", &Skin::screen },
    { "screenEdge", &Skin::screenEdge }, { "screenText", &Skin::screenText }, { "ledRed", &Skin::ledRed },
    { "ledAmber", &Skin::ledAmber },     { "ledGreen", &Skin::ledGreen },     { "ledBlue", &Skin::ledBlue },
    { "capRed", &Skin::capRed },         { "capYellow", &Skin::capYellow },   { "capGreen", &Skin::capGreen },
    { "capBlue", &Skin::capBlue },       { "capWhite", &Skin::capWhite },     { "capOrange", &Skin::capOrange },
    { "buttonFace", &Skin::buttonFace }, { "buttonText", &Skin::buttonText }, { "litText", &Skin::litText },
};

struct NumberKey { const char* key; float Skin::* member; float min, max; };
const NumberKey numberKeys[] = {
    { "buttonCorner", &Skin::buttonCorner, 0.0f, 12.0f },
    { "sectionCorner", &Skin::sectionCorner, 0.0f, 16.0f },
    { "screenCorner", &Skin::screenCorner, 0.0f, 10.0f },
    { "accentStripe", &Skin::accentStripe, 0.0f, 8.0f },
};

const juce::StringArray buttonStyleNames { "glow", "flat", "gloss", "glass" };
const juce::StringArray textureNames { "brushed", "gradient", "flat" };

/** "#RRGGBB" or "#AARRGGBB"; the # is optional. */
std::optional<juce::Colour> parseColour (const juce::String& text)
{
    const auto hex = text.trim().trimCharactersAtStart ("#");
    if ((hex.length() != 6 && hex.length() != 8) || ! hex.containsOnly ("0123456789abcdefABCDEF"))
        return std::nullopt;
    const auto value = (juce::uint32) hex.getHexValue64();
    return juce::Colour (hex.length() == 6 ? (0xff000000u | value) : value);
}

juce::String colourText (juce::Colour c)
{
    return "#" + (c.getAlpha() == 0xff ? c.toDisplayString (false) : c.toDisplayString (true));
}

juce::PropertiesFile::Options settingsOptions()
{
    juce::PropertiesFile::Options o;
    o.applicationName = "Muculator";
    o.filenameSuffix = ".settings";
   #if JUCE_LINUX || JUCE_BSD
    o.folderName = ".config/Muculator";
   #else
    o.folderName = "Muculator";
   #endif
    o.osxLibrarySubFolder = "Application Support";
    return o;
}

const juce::String userPrefix { "user:" };
} // namespace

const Skin& skin() { return currentSkin(); }

juce::Colour Skin::cap (Cap c) const
{
    switch (c)
    {
        case Cap::Red:    return capRed;
        case Cap::Yellow: return capYellow;
        case Cap::Green:  return capGreen;
        case Cap::Blue:   return capBlue;
        case Cap::Orange: return capOrange;
        case Cap::White:  break;
    }
    return capWhite;
}

Skin Skin::console()
{
    Skin s;
    s.id = "console";
    s.name = "Console";
    s.author = "Muculator";
    s.description = "Analog mixing console: grey metal, LED screens, illuminated buttons.";
    return s;
}

bool Skin::parse (const juce::String& json, const Skin& base, Skin& result, juce::StringArray& problems)
{
    juce::var root;
    if (const auto r = juce::JSON::parse (json, root); r.failed() || ! root.isObject())
    {
        problems.add ("Not a JSON object" + (r.failed() ? ": " + r.getErrorMessage() : juce::String()));
        return false;
    }

    result = base;
    static int depth = 0; // "base" may name a built-in skin; never follow it more than once
    if (const auto baseId = root["base"].toString(); baseId.isNotEmpty())
    {
        Skin b;
        juce::StringArray ignored;
        if (depth == 0 && SkinManager::builtInIds().contains (baseId))
        {
            ++depth;
            SkinManager::loadBuiltIn (baseId, b, ignored);
            --depth;
            result = b;
        }
        else
            problems.add ("base: \"" + baseId + "\" is not a built-in skin");
    }

    if (root.hasProperty ("name"))        result.name = root["name"].toString().trim();
    if (root.hasProperty ("author"))      result.author = root["author"].toString().trim();
    if (root.hasProperty ("description")) result.description = root["description"].toString().trim();

    const juce::StringArray topKeys { "format", "base", "name", "author", "description", "colors", "style", "fonts" };
    if (auto* obj = root.getDynamicObject())
        for (const auto& p : obj->getProperties())
            if (! topKeys.contains (p.name.toString()))
                problems.add ("unknown key \"" + p.name.toString() + "\"");

    if (auto* colours = root["colors"].getDynamicObject())
    {
        for (const auto& p : colours->getProperties())
        {
            const auto key = p.name.toString();
            const auto* k = std::find_if (std::begin (colourKeys), std::end (colourKeys), [&] (auto& c) { return key == c.key; });
            if (k == std::end (colourKeys))
                problems.add ("colors: unknown key \"" + key + "\"");
            else if (const auto c = parseColour (p.value.toString()))
                result.*(k->member) = *c;
            else
                problems.add ("colors." + key + ": \"" + p.value.toString() + "\" is not a colour like #RRGGBB");
        }
    }

    if (auto* style = root["style"].getDynamicObject())
    {
        for (const auto& p : style->getProperties())
        {
            const auto key = p.name.toString();
            const auto value = p.value;
            if (key == "buttons" || key == "texture")
            {
                const auto& names = key == "buttons" ? buttonStyleNames : textureNames;
                const int i = names.indexOf (value.toString().trim().toLowerCase());
                if (i < 0)
                    problems.add ("style." + key + ": use one of " + names.joinIntoString (", "));
                else if (key == "buttons")
                    result.buttonStyle = (ButtonStyle) i;
                else
                    result.texture = (Texture) i;
            }
            else if (key == "screws" || key == "screenGlare")
            {
                if (! value.isBool())
                    problems.add ("style." + key + ": use true or false");
                else
                    (key == "screws" ? result.screws : result.screenGlare) = (bool) value;
            }
            else if (const auto* k = std::find_if (std::begin (numberKeys), std::end (numberKeys), [&] (auto& n) { return key == n.key; });
                     k != std::end (numberKeys))
            {
                const double v = (double) value;
                if (! (value.isInt() || value.isInt64() || value.isDouble()) || v < k->min || v > k->max)
                    problems.add ("style." + key + ": use a number from " + juce::String (k->min, 0) + " to " + juce::String (k->max, 0));
                else
                    result.*(k->member) = (float) v;
            }
            else
                problems.add ("style: unknown key \"" + key + "\"");
        }
    }

    if (auto* fonts = root["fonts"].getDynamicObject())
    {
        for (const auto& p : fonts->getProperties())
        {
            const auto key = p.name.toString();
            if (key == "label")    result.labelFont = p.value.toString().trim();
            else if (key == "led") result.ledFont = p.value.toString().trim();
            else problems.add ("fonts: unknown key \"" + key + "\"");
        }
    }
    return true;
}

juce::String Skin::toJson() const
{
    auto* colours = new juce::DynamicObject();
    for (const auto& k : colourKeys)
        colours->setProperty (k.key, colourText (this->*(k.member)));

    auto* style = new juce::DynamicObject();
    style->setProperty ("buttons", buttonStyleNames[(int) buttonStyle]);
    style->setProperty ("texture", textureNames[(int) texture]);
    for (const auto& k : numberKeys)
        style->setProperty (k.key, this->*(k.member));
    style->setProperty ("screws", screws);
    style->setProperty ("screenGlare", screenGlare);

    auto* fonts = new juce::DynamicObject();
    fonts->setProperty ("label", labelFont);
    fonts->setProperty ("led", ledFont);

    auto* root = new juce::DynamicObject();
    root->setProperty ("format", 1);
    root->setProperty ("name", name);
    root->setProperty ("author", author);
    root->setProperty ("description", description);
    root->setProperty ("colors", juce::var (colours));
    root->setProperty ("style", juce::var (style));
    root->setProperty ("fonts", juce::var (fonts));
    return juce::JSON::toString (juce::var (root)) + "\n";
}

//==============================================================================
SkinManager::SkinManager()
{
    scan();
   #if ! MUCULATOR_SNAPSHOT
    juce::PropertiesFile settings (settingsOptions());
    select (settings.getValue ("skin", "console"), false);
   #endif
}

const juce::StringArray& SkinManager::colourSkinIds()
{
    static const juce::StringArray ids { "indigo", "teal-grove", "violet-night", "harbor", "naples" };
    return ids;
}

const juce::StringArray& SkinManager::builtInIds()
{
    static const juce::StringArray ids = []
    {
        juce::StringArray all { "console", "pattern", "session", "modular", "studio-pro" };
        all.addArray (colourSkinIds());
        return all;
    }();
    return ids;
}

bool SkinManager::loadBuiltIn (const juce::String& id, Skin& result, juce::StringArray& problems)
{
    for (int i = 0; i < SkinData::namedResourceListSize; ++i)
    {
        if (juce::String (SkinData::originalFilenames[i]) != id + ".muskin")
            continue;
        int size = 0;
        const char* data = SkinData::getNamedResource (SkinData::namedResourceList[i], size);
        if (! Skin::parse (juce::String::fromUTF8 (data, size), Skin::console(), result, problems))
            return false;
        result.id = id;
        return true;
    }
    problems.add ("no built-in skin called \"" + id + "\"");
    return false;
}

juce::File SkinManager::skinsFolder()
{
    return settingsOptions().getDefaultFile().getParentDirectory().getChildFile ("Skins");
}

void SkinManager::scan()
{
    builtInEntries.clear();
    for (const auto& id : builtInIds())
    {
        Entry e;
        Skin s;
        e.id = id;
        e.readable = loadBuiltIn (id, s, e.problems);
        e.name = s.name;
        builtInEntries.push_back (e);
    }

    userEntries.clear();
    auto files = skinsFolder().findChildFiles (juce::File::findFiles, false, "*.muskin");
    files.sort();
    for (const auto& f : files)
    {
        Entry e;
        Skin s;
        e.id = userPrefix + f.getFileName();
        e.file = f;
        e.readable = Skin::parse (f.loadFileAsString(), Skin::console(), s, e.problems);
        e.name = s.name.isNotEmpty() && s.name != "Console" ? s.name : f.getFileNameWithoutExtension();
        userEntries.push_back (e);
    }
}

void SkinManager::select (const juce::String& id, bool remember)
{
    Skin s;
    juce::StringArray problems;
    bool ok = false;
    if (id.startsWith (userPrefix))
    {
        const auto f = skinsFolder().getChildFile (id.fromFirstOccurrenceOf (userPrefix, false, false));
        ok = f.existsAsFile() && Skin::parse (f.loadFileAsString(), Skin::console(), s, problems);
        s.id = id;
    }
    else
        ok = loadBuiltIn (id, s, problems);

    if (! ok)
        s = Skin::console();

    current = s.id;
    currentSkin() = s;
   #if ! MUCULATOR_SNAPSHOT
    if (remember)
    {
        juce::PropertiesFile settings (settingsOptions());
        settings.setValue ("skin", current);
    }
   #else
    juce::ignoreUnused (remember);
   #endif
    sendSynchronousChangeMessage();
}

void SkinManager::reload()
{
    scan();
    select (current, false);
}

juce::File SkinManager::exportCurrent()
{
    auto folder = skinsFolder();
    folder.createDirectory();
    auto base = skin().name.isNotEmpty() ? skin().name : juce::String ("My skin");
    auto file = folder.getNonexistentChildFile (juce::File::createLegalFileName (base + " copy"), ".muskin", true);
    auto copy = skin();
    copy.name = file.getFileNameWithoutExtension();
    copy.author = {};
    file.replaceWithText (copy.toJson());
    scan();
    return file;
}
} // namespace ui
