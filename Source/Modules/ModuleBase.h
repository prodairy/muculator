#pragma once

#include "UI/Widgets.h"

/** What a module needs from the editor. */
struct ModuleContext
{
    virtual ~ModuleContext() = default;
    /** The BPM this module must use (shared when CHAIN is on). Contains an error when invalid. */
    virtual calc::Parsed bpmFor (const juce::String& moduleId) const = 0;
    virtual juce::String bpmTextFor (const juce::String& moduleId) const = 0;
    virtual void setBpmTextFor (const juce::String& moduleId, const juce::String& text) = 0;
    virtual bool isChained() const = 0;
    virtual double hostSampleRate() const = 0;
    virtual juce::ValueTree moduleState (const juce::String& moduleId) = 0;
};

class ModuleBase : public juce::Component
{
public:
    ModuleBase (const juce::String& id, const juce::String& title, ModuleContext& ctx);

    const juce::String& getId() const { return id; }
    const juce::String& getTitle() const { return title; }

    /** Recalculate everything from current inputs. Must never throw / crash on bad input. */
    virtual void refresh() = 0;

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Opens or closes the "How to use" panel. */
    void showHelp (bool shouldShow);

protected:
    /** Area below the header for the module's content. */
    virtual void layoutContent (juce::Rectangle<int> area) = 0;
    /** Optional readout drawn at the right of the header (e.g. "A4 = 440 Hz"). */
    juce::String headerBadge;
    /** Text for the "How to use" panel (format: see ui::HelpPanel). */
    virtual juce::String helpText() const = 0;
    /** Call when helpText() changes (e.g. another tab) so an open panel follows. */
    void updateHelp();

    calc::Parsed bpm() const { return ctx.bpmFor (id); }
    juce::ValueTree state() { return ctx.moduleState (id); }

    /** Shows validation errors in the module's warning strip (empty = clear). */
    void showErrors (const juce::StringArray& errors);

    ModuleContext& ctx;
    ui::WarningPanel inputWarning;

private:
    juce::String id, title;
    ui::IllumButton helpButton { "?", ui::col::capBlue, true };
    ui::HelpPanel help;
};

/** Collects parse errors while reading fields. */
struct Validator
{
    juce::StringArray errors;

    std::optional<double> read (ui::LedField& f, const calc::Parsed& p)
    {
        f.setInvalid (! p.ok());
        if (! p.ok()) errors.add (ui::str (p.error));
        return p.value;
    }
    std::optional<double> num (ui::LedField& f, const std::string& name, double lo, double hi)
    {
        return read (f, calc::parseInRange (f.text(), name, lo, hi));
    }
    std::optional<double> bpmField (ui::LedField& f, const std::string& name)
    {
        return read (f, calc::parseBpm (f.text(), name));
    }
    void add (const calc::Parsed& p) { if (! p.ok()) errors.add (ui::str (p.error)); }
    bool ok() const { return errors.isEmpty(); }
};
