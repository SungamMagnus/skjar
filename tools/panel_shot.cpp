/* Renders the editor to a PNG without a host. The picture itself is a native
   web view and does not draw into the snapshot; everything around it does. */

#include "PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;

    skjar::SkjarProcessor proc;
    proc.settings.url = "https://www.youtube.com/watch?v=M7lc1UVf-VE";
    proc.settings.cue = 42.5;
    proc.presetName = "Demo";
    ++proc.stateGeneration;

    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
    juce::MessageManager::getInstance()->runDispatchLoopUntil (300);

    const auto out = juce::File::getCurrentWorkingDirectory()
                         .getChildFile (argc > 1 ? argv[1] : "panel.png");
    juce::FileOutputStream stream (out);
    stream.setPosition (0);
    stream.truncate();
    juce::PNGImageFormat().writeImageToStream (editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f), stream);
    std::printf ("%s\n", out.getFullPathName().toRawUTF8());
    return 0;
}
