#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace skjar
{

namespace id
{
const juce::Identifier state { "Skjar" }, url { "url" }, cue { "cue" },
    follow { "followTimeline" }, muted { "muted" }, preset { "preset" };
}

juce::ValueTree Settings::toTree() const
{
    juce::ValueTree t (id::state);
    t.setProperty (id::url, url, nullptr);
    t.setProperty (id::cue, cue, nullptr);
    t.setProperty (id::follow, followTimeline, nullptr);
    t.setProperty (id::muted, muted, nullptr);
    return t;
}

Settings Settings::fromTree (const juce::ValueTree& t)
{
    Settings s;
    s.url = t.getProperty (id::url).toString();
    s.cue = juce::jmax (0.0, (double) t.getProperty (id::cue, 0.0));
    s.followTimeline = t.getProperty (id::follow, false);
    s.muted = t.getProperty (id::muted, false);
    return s;
}

SkjarProcessor::SkjarProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

bool SkjarProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == layouts.getMainInputChannelSet()
        && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void SkjarProcessor::processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&)
{
    auto* head = getPlayHead();
    if (head == nullptr)
        return;

    const auto pos = head->getPosition();
    if (! pos.hasValue())
        return;

    playing_.store (pos->getIsPlaying(), std::memory_order_relaxed);

    // Hosts disagree on which clock they fill in: seconds where offered, then
    // samples, then beats at the current tempo.
    if (auto s = pos->getTimeInSeconds())
        seconds_.store (*s, std::memory_order_relaxed);
    else if (auto n = pos->getTimeInSamples())
        seconds_.store ((double) *n / sampleRate_, std::memory_order_relaxed);
    else if (auto ppq = pos->getPpqPosition())
        if (auto bpm = pos->getBpm(); bpm && *bpm > 0.0)
            seconds_.store (*ppq * 60.0 / *bpm, std::memory_order_relaxed);
}

Transport SkjarProcessor::transport() const
{
    return { playing_.load (std::memory_order_relaxed), seconds_.load (std::memory_order_relaxed) };
}

void SkjarProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto t = settings.toTree();
    t.setProperty (id::preset, presetName, nullptr);
    if (auto xml = t.createXml())
        copyXmlToBinary (*xml, dest);
}

void SkjarProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        const auto t = juce::ValueTree::fromXml (*xml);
        if (! t.hasType (id::state))
            return;
        settings = Settings::fromTree (t);
        presetName = t.getProperty (id::preset).toString();
        ++stateGeneration;
    }
}

juce::AudioProcessorEditor* SkjarProcessor::createEditor()
{
    return new SkjarEditor (*this);
}

} // namespace skjar

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new skjar::SkjarProcessor();
}
