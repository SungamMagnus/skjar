#pragma once

#include <optional>

#include "PluginProcessor.h"

/** Presets are one small XML file each, in the folder macOS hosts already
    look in for presets — so they can be copied, renamed or shared in Finder. */
namespace skjar::presets
{

juce::File folder();

/** Names, sorted, without the extension. */
juce::StringArray list();

bool save (const juce::String& name, const Settings&);
std::optional<Settings> load (const juce::String& name);
bool remove (const juce::String& name);

} // namespace skjar::presets
