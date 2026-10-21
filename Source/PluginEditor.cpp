#include "PluginEditor.h"

#include "Presets.h"
#include "YouTube.h"

namespace skjar
{

using namespace sungam;

namespace
{
constexpr int pad = 16, headerH = 44, sourceH = 52, stripH = 64;

/** How far the video may wander before it is pulled back, and how long to
    leave it alone after each pull — a seek takes a moment to land, and judging
    it before then only causes another. */
constexpr double driftLimit = 0.35, settleTime = 1.5;

double now() { return juce::Time::getMillisecondCounterHiRes() * 0.001; }

void colourButton (juce::TextButton& b, juce::Colour c)
{
    b.setColour (juce::TextButton::buttonColourId, c);
    b.setMouseCursor (juce::MouseCursor::PointingHandCursor);
}
} // namespace

SkjarEditor::SkjarEditor (SkjarProcessor& p) : AudioProcessorEditor (p), proc_ (p)
{
    setLookAndFeel (&lnf_);

    url_.setFont (mono (12.0f));
    url_.setTextToShowWhenEmpty ("Paste a YouTube link", ink (0.38f));
    url_.setIndents (8, 0);
    url_.setJustification (juce::Justification::centredLeft);
    url_.setSelectAllWhenFocused (true);
    url_.onReturnKey = [this] { loadLink(); };
    addAndMakeVisible (url_);

    colourButton (load_, hue::teal);
    load_.onClick = [this] { loadLink(); };
    addAndMakeVisible (load_);

    addAndMakeVisible (video_);

    cue_.setFont (mono (12.0f));
    cue_.setIndents (6, 0);
    cue_.setJustification (juce::Justification::centred);
    cue_.setInputRestrictions (12, "0123456789.:");
    cue_.setSelectAllWhenFocused (true);
    cue_.onReturnKey = [this] { commitCue(); unfocusAllComponents(); };
    cue_.onFocusLost = [this] { commitCue(); };
    cue_.setTooltip ("Where the video starts when the transport does");
    addAndMakeVisible (cue_);

    for (auto* b : { &setCue_, &goCue_, &fromCue_, &timeline_ })
    {
        colourButton (*b, hue::coral);
        addAndMakeVisible (*b);
    }
    setCue_.onClick = [this] {
        if (video_.state().ready)
            setCue (video_.state().time);
    };
    goCue_.onClick = [this] { video_.park (proc_.settings.cue); };
    fromCue_.onClick = [this] { setFollowTimeline (false); };
    timeline_.onClick = [this] { setFollowTimeline (true); };

    colourButton (mute_, hue::teal);
    mute_.setClickingTogglesState (true);
    mute_.onClick = [this] {
        proc_.settings.muted = mute_.getToggleState();
        video_.setMuted (proc_.settings.muted);
    };
    addAndMakeVisible (mute_);

    preset_.setTextWhenNothingSelected ("No preset");
    preset_.setTextWhenNoChoicesAvailable ("No presets saved");
    preset_.onChange = [this] { choosePreset(); };
    addAndMakeVisible (preset_);

    for (auto* b : { &save_, &delete_ })
    {
        colourButton (*b, hue::steel);
        addAndMakeVisible (*b);
    }
    save_.onClick = [this] { savePreset(); };
    delete_.onClick = [this] { deletePreset(); };

    setResizable (true, true);
    setResizeLimits (760, 480, 2400, 1600);
    setSize (860, 600);

    syncFromProcessor();
    startTimerHz (30);
}

SkjarEditor::~SkjarEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

/* ── Layout ──────────────────────────────────────────────────────────────── */

void SkjarEditor::resized()
{
    auto area = getLocalBounds().reduced (pad, 0);
    area.removeFromTop (headerH);

    auto source = area.removeFromTop (sourceH);
    sourceFrame_ = source.toFloat().reduced (0.0f, 4.0f);
    {
        auto row = source.reduced (12, 0).withSizeKeepingCentre (source.getWidth() - 24, 26).translated (0, 2);
        load_.setBounds (row.removeFromRight (72));
        row.removeFromRight (8);
        url_.setBounds (row);
    }

    auto strip = area.removeFromBottom (stripH + pad).withTrimmedBottom (pad);
    area.removeFromBottom (12);
    area.removeFromTop (8);
    video_.setBounds (area);

    auto sync = strip.removeFromLeft (juce::jmax (500, (int) (strip.getWidth() * 0.62f)));
    strip.removeFromLeft (12);
    syncFrame_ = sync.toFloat().reduced (0.0f, 4.0f);
    presetFrame_ = strip.toFloat().reduced (0.0f, 4.0f);

    {
        auto row = sync.reduced (12, 0).withSizeKeepingCentre (sync.getWidth() - 24, 26).translated (0, 2);
        row.removeFromLeft (34);   // "CUE", painted
        cue_.setBounds (row.removeFromLeft (100));
        row.removeFromLeft (4);
        setCue_.setBounds (row.removeFromLeft (46));
        row.removeFromLeft (4);
        goCue_.setBounds (row.removeFromLeft (40));
        mute_.setBounds (row.removeFromRight (58));
        row.removeFromRight (14);
        timeline_.setBounds (row.removeFromRight (84));
        fromCue_.setBounds (row.removeFromRight (84));
    }
    {
        auto row = strip.reduced (12, 0).withSizeKeepingCentre (strip.getWidth() - 24, 26).translated (0, 2);
        delete_.setBounds (row.removeFromRight (44));
        row.removeFromRight (4);
        save_.setBounds (row.removeFromRight (52));
        row.removeFromRight (8);
        preset_.setBounds (row);
    }
}

void SkjarEditor::paint (juce::Graphics& g)
{
    g.fillAll (hue::paper);

    auto header = getLocalBounds().reduced (pad, 0).removeFromTop (headerH).toFloat();
    // Spelled out in caps: JUCE's toUpperCase leaves the á alone.
    wordmark (g, juce::String (juce::CharPointer_UTF8 ("SKJ\xc3\x81R")), header.removeFromLeft (160.0f).translated (0.0f, 2.0f), 16.0f, ink (0.55f));

    // Live state, right-aligned: the transport, then the picture.
    const auto t = proc_.transport();
    const auto& v = video_.state();

    auto readout = header.removeFromRight (400.0f).translated (0.0f, 2.0f);
    const auto live = t.playing ? hue::amber : ink (0.38f);
    const auto transportText = (t.playing ? "PLAY " : "STOP ") + youtube::formatTime (t.seconds);
    g.setFont (mono (11.0f, true));
    g.setColour (live);
    g.drawText (transportText, readout.removeFromRight (130.0f), juce::Justification::centredRight, false);
    lamp (g, { readout.getRight() + 122.0f - juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), transportText),
               readout.getCentreY() }, t.playing, live);
    readout.removeFromRight (24.0f);

    g.setFont (mono (11.0f));
    g.setColour (ink (0.55f));
    if (v.ready)
        g.drawText ("VIDEO " + youtube::formatTime (v.time) + " / " + youtube::formatTime (v.duration),
                    readout, juce::Justification::centredRight, false);

    if (status_.isNotEmpty())
    {
        g.setColour (ink (0.70f));
        g.drawText (status_.toUpperCase(), header.reduced (12.0f, 0.0f).translated (0.0f, 2.0f),
                    juce::Justification::centredLeft, true);
    }

    frame (g, sourceFrame_, hue::teal, "Source");
    frame (g, syncFrame_, hue::coral, "Sync");
    frame (g, presetFrame_, hue::steel, "Preset");

    g.setFont (mono (10.5f, true));
    g.setColour (hue::coral);
    g.drawText ("CUE", juce::Rectangle<float> (syncFrame_.getX() + 12.0f, (float) cue_.getY(), 30.0f,
                                               (float) cue_.getHeight()),
                juce::Justification::centredLeft, false);
}

/* ── Following the transport ─────────────────────────────────────────────── */

double SkjarEditor::expectedVideoTime (const Transport& t) const
{
    return juce::jmax (0.0, proc_.settings.cue + (t.seconds - anchor_));
}

void SkjarEditor::park (const Transport& t)
{
    // Stopped, the picture shows where the next start will land: the cue, or
    // in timeline mode wherever the host's play head now sits.
    const double at = proc_.settings.followTimeline ? juce::jmax (0.0, proc_.settings.cue + t.seconds)
                                                    : proc_.settings.cue;
    video_.park (at);
    parkedAt_ = at;
}

void SkjarEditor::follow (const Transport& t)
{
    const auto& v = video_.state();
    const bool becameReady = v.ready && ! wasReady_;
    wasReady_ = v.ready;

    if (t.playing != hostWasPlaying_)
    {
        hostWasPlaying_ = t.playing;
        started_ = false;
        if (t.playing)
            anchor_ = proc_.settings.followTimeline ? 0.0 : t.seconds;
        else
            park (t);
    }
    else if (! t.playing)
    {
        // A stopped host whose play head is moved: in timeline mode, show the
        // frame it now points at.
        if (becameReady || (proc_.settings.followTimeline
                            && std::abs (t.seconds - lastHostSeconds_) > 0.01))
            park (t);
    }
    else
    {
        // A jump the transport did not play through: a loop, or the play head
        // moved while running. From the cue, a jump is a fresh start.
        const double step = t.seconds - lastHostSeconds_;
        if (step < -0.05 || step > 0.5)
        {
            if (! proc_.settings.followTimeline)
                anchor_ = t.seconds;
            started_ = false;
        }
    }
    lastHostSeconds_ = t.seconds;

    if (! t.playing || ! v.ready)
        return;

    const double expected = expectedVideoTime (t);
    if (! started_)
    {
        video_.start (expected);
        started_ = true;
        lastCorrection_ = now();
        return;
    }

    if (now() - lastCorrection_ < settleTime || video_.stateAge() > 0.2)
        return;

    if (v.state == 1)
    {
        if (std::abs (v.time - expected) > driftLimit)
        {
            video_.seek (expected);
            lastCorrection_ = now();
        }
    }
    else if (v.state != 3 && v.state != 0)
    {
        // Paused or cued while the host plays — likely clicked in the player,
        // or a start that never took.
        video_.start (expected);
        lastCorrection_ = now();
    }
}

void SkjarEditor::timerCallback()
{
    if (proc_.stateGeneration.load() != seenGeneration_)
        syncFromProcessor();

    video_.poll();
    follow (proc_.transport());

    const auto err = video_.state().error;
    const auto text = err != 0 ? errorText (err) : juce::String();
    if (text != status_ && (err != 0 || status_.startsWith ("Player")))
        status_ = text;

    repaint (getLocalBounds().removeFromTop (headerH));
}

/* ── Settings ────────────────────────────────────────────────────────────── */

void SkjarEditor::syncFromProcessor()
{
    seenGeneration_ = proc_.stateGeneration.load();
    const auto& s = proc_.settings;

    url_.setText (s.url, false);
    cue_.setText (youtube::formatTime (s.cue), false);
    fromCue_.setToggleState (! s.followTimeline, juce::dontSendNotification);
    timeline_.setToggleState (s.followTimeline, juce::dontSendNotification);
    mute_.setToggleState (s.muted, juce::dontSendNotification);
    video_.setMuted (s.muted);

    refreshPresets();

    const auto link = youtube::parse (s.url);
    status_.clear();
    wasReady_ = false;
    started_ = false;
    if (link.videoId.isNotEmpty())
        video_.load (link.videoId, s.cue);
    else
        video_.clear();
}

void SkjarEditor::loadLink()
{
    const auto text = url_.getText().trim();
    const auto link = youtube::parse (text);
    if (link.videoId.isEmpty())
    {
        status_ = text.isEmpty() ? juce::String() : "Not a YouTube link";
        repaint();
        return;
    }

    proc_.settings.url = text;
    // A link with its own start time (?t=) brings it in as the cue.
    if (link.startSeconds >= 0.0)
        proc_.settings.cue = link.startSeconds;
    cue_.setText (youtube::formatTime (proc_.settings.cue), false);

    status_.clear();
    wasReady_ = false;
    started_ = false;
    video_.load (link.videoId, proc_.settings.cue);
    unfocusAllComponents();
}

void SkjarEditor::commitCue()
{
    const double t = youtube::parseTime (cue_.getText());
    if (t < 0.0)
        cue_.setText (youtube::formatTime (proc_.settings.cue), false);
    else
        setCue (t);
}

void SkjarEditor::setCue (double seconds)
{
    proc_.settings.cue = juce::jmax (0.0, seconds);
    cue_.setText (youtube::formatTime (proc_.settings.cue), false);
    if (! proc_.transport().playing)
        park (proc_.transport());
}

void SkjarEditor::setFollowTimeline (bool follow)
{
    proc_.settings.followTimeline = follow;
    fromCue_.setToggleState (! follow, juce::dontSendNotification);
    timeline_.setToggleState (follow, juce::dontSendNotification);

    const auto t = proc_.transport();
    if (t.playing)
    {
        anchor_ = follow ? 0.0 : t.seconds;
        started_ = false;
    }
    else
        park (t);
}

/* ── Presets ─────────────────────────────────────────────────────────────── */

void SkjarEditor::refreshPresets()
{
    preset_.clear (juce::dontSendNotification);
    const auto names = presets::list();
    preset_.addItemList (names, 1);

    const int index = names.indexOf (proc_.presetName);
    if (index >= 0)
        preset_.setSelectedItemIndex (index, juce::dontSendNotification);
    delete_.setEnabled (index >= 0);
}

void SkjarEditor::choosePreset()
{
    const auto name = preset_.getText();
    if (name.isEmpty() || name == proc_.presetName)
        return;

    if (auto s = presets::load (name))
    {
        proc_.settings = *s;
        proc_.presetName = name;
        syncFromProcessor();
    }
    else
    {
        status_ = "Could not read preset " + name;
        refreshPresets();
    }
}

void SkjarEditor::savePreset()
{
    commitCue();

    auto* w = new juce::AlertWindow ("Save preset", "Saves the link, the cue and the sync mode.",
                                     juce::MessageBoxIconType::NoIcon, this);
    w->setLookAndFeel (&lnf_);
    w->addTextEditor ("name", proc_.presetName, {});
    if (auto* field = w->getTextEditor ("name"))
        field->setFont (mono (12.0f));
    w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<SkjarEditor> self (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([self, w] (int result) {
        const auto name = w->getTextEditorContents ("name").trim();
        if (self == nullptr || result != 1 || name.isEmpty())
            return;

        if (presets::save (name, self->proc_.settings))
        {
            self->proc_.presetName = juce::File::createLegalFileName (name);
            self->status_.clear();
        }
        else
            self->status_ = "Could not save to " + presets::folder().getFullPathName();
        self->refreshPresets();
    }), true);
}

void SkjarEditor::deletePreset()
{
    const auto name = proc_.presetName;
    if (name.isEmpty())
        return;

    juce::Component::SafePointer<SkjarEditor> self (this);
    auto options = juce::MessageBoxOptions::makeOptionsOkCancel (
        juce::MessageBoxIconType::NoIcon, "Delete preset", "Delete \"" + name + "\"?", "Delete", "Cancel", this);
    juce::AlertWindow::showAsync (options, [self, name] (int result) {
        if (self == nullptr || result != 1)
            return;
        presets::remove (name);
        self->proc_.presetName.clear();
        self->refreshPresets();
    });
}

juce::String SkjarEditor::errorText (int code) const
{
    switch (code)
    {
        case 2:   return "Player: bad video id";
        case 5:   return "Player: cannot play this video here";
        case 100: return "Player: video not found or private";
        case 101:
        case 150: return "Player: the owner does not allow embedding";
        case 153: return "Player: refused, missing origin";
        default:  return "Player: error " + juce::String (code);
    }
}

} // namespace skjar
