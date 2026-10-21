#include "Presets.h"

namespace skjar::presets
{

namespace
{
const char* const extension = ".skjar";

juce::File fileFor (const juce::String& name)
{
    return folder().getChildFile (juce::File::createLegalFileName (name.trim()) + extension);
}
} // namespace

juce::File folder()
{
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory)
        .getChildFile ("Library/Audio/Presets/Sungam/Skjar");
}

juce::StringArray list()
{
    juce::StringArray names;
    for (auto& f : folder().findChildFiles (juce::File::findFiles, false, juce::String ("*") + extension))
        names.add (f.getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}

bool save (const juce::String& name, const Settings& s)
{
    if (name.trim().isEmpty() || ! folder().createDirectory())
        return false;
    if (auto xml = s.toTree().createXml())
        return xml->writeTo (fileFor (name));
    return false;
}

std::optional<Settings> load (const juce::String& name)
{
    if (auto xml = juce::XmlDocument::parse (fileFor (name)))
        return Settings::fromTree (juce::ValueTree::fromXml (*xml));
    return std::nullopt;
}

bool remove (const juce::String& name)
{
    return fileFor (name).deleteFile();
}

} // namespace skjar::presets
