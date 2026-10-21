#pragma once

#include <juce_core/juce_core.h>

/** Everything that knows what a YouTube link looks like, kept apart from the
    view so it can be checked without a window. */
namespace skjar::youtube
{

struct Link
{
    juce::String videoId;   // empty when the text was not a YouTube link
    double startSeconds = -1.0;   // the link's own t= / start=, or -1
};

/** Accepts watch, youtu.be, shorts, embed, live and music links, with or
    without a scheme, and a bare eleven-character id. */
Link parse (const juce::String& text);

/** "90", "1m30s", "1h2m3s" — the forms YouTube writes into t=. */
double parseTimestamp (const juce::String& t);

/** The origin the player page claims. Without one the embed refuses to play
    (error 153); it checks the origin is well formed, not that it is ours. */
extern const char* const pageOrigin;

/** The player page: the IFrame API filling the whole view, and a handful of
    functions the native side calls by name. */
juce::String playerPage (const juce::String& videoId, double startSeconds);

/** m:ss.cc, or h:mm:ss.cc past the hour. */
juce::String formatTime (double seconds);

/** Reads what formatTime writes, plain seconds, and m:ss / h:mm:ss. -1 when
    the text is not a time. */
double parseTime (const juce::String& text);

} // namespace skjar::youtube
