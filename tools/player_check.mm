/* Two checks without a host: that the link parser reads every form it claims
   to, and — given a link — that the player page actually loads, starts, seeks
   and parks in a real WKWebView.

     player_check                  parser only
     player_check <youtube link>   parser, then the player */

#include <juce_gui_extra/juce_gui_extra.h>

#include "VideoView.h"
#include "YouTube.h"

using namespace skjar;

namespace
{
int failures = 0;

void expect (bool ok, const juce::String& what)
{
    std::printf ("%s  %s\n", ok ? "ok  " : "FAIL", what.toRawUTF8());
    if (! ok)
        ++failures;
}

void checkParser()
{
    const struct { const char* in; const char* id; double start; } cases[] = {
        { "https://www.youtube.com/watch?v=dQw4w9WgXcQ", "dQw4w9WgXcQ", -1 },
        { "youtube.com/watch?feature=share&v=dQw4w9WgXcQ&t=90", "dQw4w9WgXcQ", 90 },
        { "https://youtu.be/dQw4w9WgXcQ?t=1m30s", "dQw4w9WgXcQ", 90 },
        { "https://m.youtube.com/watch?v=dQw4w9WgXcQ#x", "dQw4w9WgXcQ", -1 },
        { "https://music.youtube.com/watch?v=dQw4w9WgXcQ&list=RD", "dQw4w9WgXcQ", -1 },
        { "https://www.youtube.com/shorts/dQw4w9WgXcQ", "dQw4w9WgXcQ", -1 },
        { "https://www.youtube.com/embed/dQw4w9WgXcQ?start=12", "dQw4w9WgXcQ", 12 },
        { "https://www.youtube.com/live/dQw4w9WgXcQ?si=abc", "dQw4w9WgXcQ", -1 },
        { "  dQw4w9WgXcQ  ", "dQw4w9WgXcQ", -1 },
        { "https://vimeo.com/123456", "", -1 },
        { "https://www.youtube.com/watch?v=short", "", -1 },
        { "hello", "", -1 },
    };
    for (auto& c : cases)
    {
        const auto link = youtube::parse (c.in);
        expect (link.videoId == c.id && std::abs (link.startSeconds - c.start) < 1e-9,
                juce::String ("parse ") + c.in + " -> '" + link.videoId + "' @" + juce::String (link.startSeconds));
    }

    expect (youtube::formatTime (75.5) == "1:15.50", "format 75.5 -> " + youtube::formatTime (75.5));
    expect (youtube::formatTime (3725.0) == "1:02:05.00", "format 3725 -> " + youtube::formatTime (3725.0));
    expect (std::abs (youtube::parseTime ("1:15.5") - 75.5) < 1e-9, "parseTime 1:15.5");
    expect (std::abs (youtube::parseTime ("1:02:05") - 3725.0) < 1e-9, "parseTime 1:02:05");
    expect (std::abs (youtube::parseTime ("42.25") - 42.25) < 1e-9, "parseTime 42.25");
    expect (youtube::parseTime ("abc") < 0.0, "parseTime abc rejected");
}

/** Pumps the message loop until the condition holds or the time runs out. */
bool waitFor (VideoView& v, double seconds, std::function<bool (const PlayerState&)> done)
{
    const auto until = juce::Time::getMillisecondCounterHiRes() + seconds * 1000.0;
    while (juce::Time::getMillisecondCounterHiRes() < until)
    {
        v.poll();
        juce::MessageManager::getInstance()->runDispatchLoopUntil (100);
        if (done (v.state()))
            return true;
    }
    return false;
}

void describe (const PlayerState& s)
{
    std::printf ("      ready=%d state=%d time=%.2f duration=%.2f error=%d\n",
                 (int) s.ready, s.state, s.time, s.duration, s.error);
}

void checkPlayer (const juce::String& url)
{
    const auto link = youtube::parse (url);
    expect (link.videoId.isNotEmpty(), "player link parses");
    if (link.videoId.isEmpty())
        return;

    // The web view has to be in a real window to run its media.
    juce::DocumentWindow window ("player_check", juce::Colours::black, 0);
    VideoView view;
    view.setSize (640, 360);
    window.setContentNonOwned (&view, true);
    window.setVisible (true);

    view.setMuted (true);
    view.load (link.videoId, 0.0);

    expect (waitFor (view, 25.0, [] (auto& s) { return s.ready || s.error != 0; }), "player ready");
    describe (view.state());
    expect (view.state().error == 0, "no player error");
    if (! view.state().ready)
        return;

    view.park (30.0);
    waitFor (view, 4.0, [] (auto&) { return false; });
    describe (view.state());
    expect (! view.state().playing() && std::abs (view.state().time - 30.0) < 1.0, "park holds at 30s, paused");

    view.start (60.0);
    expect (waitFor (view, 15.0, [] (auto& s) { return s.state == 1 && s.time > 61.0; }), "start plays from 60s");
    describe (view.state());

    view.park (10.0);
    waitFor (view, 3.0, [] (auto&) { return false; });
    describe (view.state());
    expect (! view.state().playing() && std::abs (view.state().time - 10.0) < 1.0, "park from playing stops at 10s");
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;

    checkParser();
    if (argc > 1)
        checkPlayer (argv[1]);

    std::printf ("\n%s\n", failures == 0 ? "all ok" : (juce::String (failures) + " failed").toRawUTF8());
    return failures == 0 ? 0 : 1;
}
