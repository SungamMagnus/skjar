#pragma once

#include <atomic>

#include <juce_audio_processors/juce_audio_processors.h>

namespace skjar
{

/** What the audio thread saw of the host transport on its last block. */
struct Transport
{
    bool playing = false;
    double seconds = 0.0;
};

/** The settings a preset holds, and the plug-in's whole saved state. Message
    thread only — the audio thread never reads any of it. */
struct Settings
{
    juce::String url;
    double cue = 0.0;            // where the video is when the transport starts
    bool followTimeline = false; // off: every start begins at the cue
    bool muted = false;

    juce::ValueTree toTree() const;
    static Settings fromTree (const juce::ValueTree&);
};

/**
 * Passes audio through untouched. Its only job on the audio thread is to read
 * the play head; the editor does everything else.
 */
class SkjarProcessor : public juce::AudioProcessor
{
public:
    SkjarProcessor();

    void prepareToPlay (double sampleRate, int) override { sampleRate_ = sampleRate; }
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return juce::String::fromUTF8 (JucePlugin_Name); }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    Transport transport() const;

    Settings settings;
    juce::String presetName;   // the preset the settings came from, if any

    /** Bumped whenever the host restores state, so an open editor knows to
        reload rather than keep showing what it had. */
    std::atomic<int> stateGeneration { 0 };

private:
    double sampleRate_ = 44100.0;
    std::atomic<bool> playing_ { false };
    std::atomic<double> seconds_ { 0.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SkjarProcessor)
};

} // namespace skjar
