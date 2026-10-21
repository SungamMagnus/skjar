#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "Sungam.h"
#include "VideoView.h"

namespace skjar
{

/**
 * The link across the top, the picture under it, and one strip along the
 * bottom for timing and presets. It is also where the video is kept in step
 * with the transport: the processor only reports what the play head is doing,
 * and this follows along at the timer's rate — the player lives in the editor,
 * so with the editor closed there is nothing to follow.
 */
class SkjarEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SkjarEditor (SkjarProcessor&);
    ~SkjarEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SkjarProcessor& proc_;
    sungam::LookAndFeel lnf_;

    juce::TextEditor url_;
    juce::TextButton load_ { "Load" };
    VideoView video_;

    juce::TextEditor cue_;
    juce::TextButton setCue_ { "Set" }, goCue_ { "Go" };
    juce::TextButton fromCue_ { "From cue" }, timeline_ { "Timeline" };
    juce::TextButton mute_ { "Mute" };

    juce::ComboBox preset_;
    juce::TextButton save_ { "Save" }, delete_ { "Del" };

    juce::Rectangle<float> sourceFrame_, syncFrame_, presetFrame_;
    juce::String status_;

    // Following the transport.
    bool hostWasPlaying_ = false;
    bool started_ = false;         // play issued for this run of the transport
    bool wasReady_ = false;
    double anchor_ = 0.0;          // host seconds that line up with the cue
    double lastHostSeconds_ = 0.0;
    double lastCorrection_ = 0.0;
    double parkedAt_ = -1.0;
    int seenGeneration_ = -1;

    void timerCallback() override;
    void follow (const Transport&);
    double expectedVideoTime (const Transport&) const;
    void park (const Transport&);

    void syncFromProcessor();
    void loadLink();
    void commitCue();
    void setCue (double seconds);
    void setFollowTimeline (bool);

    void refreshPresets();
    void choosePreset();
    void savePreset();
    void deletePreset();

    juce::String errorText (int code) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SkjarEditor)
};

} // namespace skjar
