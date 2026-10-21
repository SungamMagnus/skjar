#pragma once

#include <memory>

#include <juce_gui_extra/juce_gui_extra.h>

namespace skjar
{

/** What the player last said about itself. YouTube's own state numbers:
    -1 unstarted, 0 ended, 1 playing, 2 paused, 3 buffering, 5 cued. */
struct PlayerState
{
    bool ready = false;
    int error = 0;          // the IFrame API's onError code, 0 for none
    int state = -1;
    double time = 0.0;
    double duration = 0.0;

    bool playing() const { return state == 1 || state == 3; }
};

/** A WKWebView running the YouTube IFrame API, driven from the message
    thread. Every call is fire-and-forget: the page answers asynchronously, and
    poll() is how the answers come back. */
class VideoView : public juce::NSViewComponent
{
public:
    VideoView();
    ~VideoView() override;

    void load (const juce::String& videoId, double startSeconds);
    void clear();

    /** Seek and play — the transport has started. */
    void start (double seconds);
    /** Seek and stay paused, showing that frame — the transport is stopped. */
    void park (double seconds);
    /** Seek without changing whether it plays — drift correction. */
    void seek (double seconds);
    void play();
    void setMuted (bool muted);

    /** Asks the page for its state; the answer lands in state() a moment later. */
    void poll();
    const PlayerState& state() const { return state_; }

    /** Seconds since the last answer came back — how stale state() is. */
    double stateAge() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    PlayerState state_;
    double stateStamp_ = 0.0;
    bool muted_ = false;

    void run (const juce::String& js);

    JUCE_DECLARE_WEAK_REFERENCEABLE (VideoView)
    JUCE_DECLARE_NON_COPYABLE (VideoView)
};

} // namespace skjar
