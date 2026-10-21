#include "YouTube.h"

namespace skjar::youtube
{

// Anything but youtube.com itself: claiming YouTube's own origin gets error
// 152 from the embed, while any other well-formed https origin plays.
const char* const pageOrigin = "https://skjar.sungam.app";

namespace
{
bool isIdChar (juce::juce_wchar c)
{
    return juce::CharacterFunctions::isLetterOrDigit (c) || c == '-' || c == '_';
}

bool isVideoId (const juce::String& s)
{
    if (s.length() != 11)
        return false;
    for (auto c : s)
        if (! isIdChar (c))
            return false;
    return true;
}

/** The run of id characters at the start of s. */
juce::String leadingId (const juce::String& s)
{
    int n = 0;
    while (n < s.length() && isIdChar (s[n]))
        ++n;
    return s.substring (0, n);
}

juce::String queryValue (const juce::String& query, const juce::String& key)
{
    for (auto& pair : juce::StringArray::fromTokens (query, "&", ""))
        if (pair.upToFirstOccurrenceOf ("=", false, false) == key)
            return juce::URL::removeEscapeChars (pair.fromFirstOccurrenceOf ("=", false, false));
    return {};
}
} // namespace

double parseTimestamp (const juce::String& raw)
{
    auto t = raw.trim().toLowerCase();
    if (t.isEmpty())
        return -1.0;
    if (t.containsOnly ("0123456789."))
        return t.getDoubleValue();

    double total = 0.0;
    juce::String digits;
    for (auto c : t)
    {
        if (juce::CharacterFunctions::isDigit (c) || c == '.')
        {
            digits += c;
            continue;
        }
        const double v = digits.getDoubleValue();
        digits.clear();
        if (c == 'h') total += v * 3600.0;
        else if (c == 'm') total += v * 60.0;
        else if (c == 's') total += v;
        else return -1.0;
    }
    return total + digits.getDoubleValue();
}

Link parse (const juce::String& text)
{
    auto s = text.trim();
    if (isVideoId (s))
        return { s, -1.0 };

    // Strip the scheme and the subdomain; what is left decides the form.
    s = s.fromFirstOccurrenceOf ("://", false, false).isNotEmpty()
            ? s.fromFirstOccurrenceOf ("://", false, false) : s;
    for (auto prefix : { "www.", "m.", "music." })
        if (s.startsWithIgnoreCase (prefix))
            s = s.substring ((int) strlen (prefix));

    const auto query = s.fromFirstOccurrenceOf ("?", false, false)
                           .upToFirstOccurrenceOf ("#", false, false);
    const auto path = s.upToFirstOccurrenceOf ("?", false, false)
                          .upToFirstOccurrenceOf ("#", false, false);

    auto start = parseTimestamp (queryValue (query, "t"));
    if (start < 0.0)
        start = parseTimestamp (queryValue (query, "start"));

    juce::String id;
    if (path.startsWithIgnoreCase ("youtu.be/"))
        id = leadingId (path.fromFirstOccurrenceOf ("/", false, false));
    else if (path.startsWithIgnoreCase ("youtube.com/") || path.startsWithIgnoreCase ("youtube-nocookie.com/"))
    {
        const auto rest = path.fromFirstOccurrenceOf ("/", false, false);
        if (rest.startsWithIgnoreCase ("watch"))
            id = queryValue (query, "v");
        else
            for (auto form : { "shorts/", "embed/", "live/", "v/" })
                if (rest.startsWithIgnoreCase (form))
                    id = leadingId (rest.substring ((int) strlen (form)));
    }

    if (! isVideoId (id))
        return {};
    return { id, start };
}

juce::String playerPage (const juce::String& videoId, double startSeconds)
{
    // Stage colour behind the player, so the letterbox reads as Beygla's
    // stage and not as a black hole in a grey panel.
    juce::String page = R"HTML(<!doctype html>
<html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  html, body { margin: 0; height: 100%; background: #201f1e; overflow: hidden; }
  #player { position: absolute; inset: 0; width: 100%; height: 100%; }
</style></head>
<body>
<div id="player"></div>
<script>
  var player = null, ready = false, lastError = 0;

  // Seeking a player that has not started yet starts it. A park is a seek
  // that must not, so the pause is held until the player gets that far.
  var holdPaused = false;

  function onYouTubeIframeAPIReady() {
    player = new YT.Player('player', {
      width: '100%', height: '100%', videoId: '%ID%',
      playerVars: { playsinline: 1, rel: 0, start: %START%, origin: '%ORIGIN%' },
      events: {
        onReady: function () { ready = true; },
        onStateChange: function (e) {
          if (holdPaused && e.data === YT.PlayerState.PLAYING) {
            holdPaused = false;
            player.pauseVideo();
          }
        },
        onError: function (e) { lastError = e.data; }
      }
    });
  }

  function skjarState() {
    if (! ready) return JSON.stringify({ ready: false, error: lastError });
    return JSON.stringify({
      ready: true, error: lastError,
      time: player.getCurrentTime(),
      state: player.getPlayerState(),
      duration: player.getDuration()
    });
  }
  function skjarStart(t) { if (ready) { holdPaused = false; player.seekTo(t, true); player.playVideo(); } }
  function skjarSeek(t)  { if (ready) player.seekTo(t, true); }
  function skjarPlay()   { if (ready) { holdPaused = false; player.playVideo(); } }
  function skjarPark(t)  {
    if (! ready) return;
    var s = player.getPlayerState();
    holdPaused = s !== YT.PlayerState.PAUSED;
    player.seekTo(t, true);
    player.pauseVideo();
  }
  function skjarMute(on) { if (ready) { if (on) player.mute(); else player.unMute(); } }
</script>
<script src="https://www.youtube.com/iframe_api"></script>
</body></html>)HTML";

    return page.replace ("%ID%", videoId)
               .replace ("%START%", juce::String ((int) std::floor (juce::jmax (0.0, startSeconds))))
               .replace ("%ORIGIN%", pageOrigin);
}

juce::String formatTime (double seconds)
{
    seconds = juce::jmax (0.0, seconds);
    const auto cs = (juce::int64) std::llround (seconds * 100.0);
    const auto h = cs / 360000, m = (cs / 6000) % 60, s = (cs / 100) % 60, c = cs % 100;
    const auto two = [] (juce::int64 v) { return juce::String (v).paddedLeft ('0', 2); };
    if (h > 0)
        return juce::String (h) + ":" + two (m) + ":" + two (s) + "." + two (c);
    return juce::String (m) + ":" + two (s) + "." + two (c);
}

double parseTime (const juce::String& text)
{
    const auto t = text.trim();
    if (t.isEmpty() || ! t.containsOnly ("0123456789.:"))
        return -1.0;

    double total = 0.0;
    for (auto& part : juce::StringArray::fromTokens (t, ":", ""))
        total = total * 60.0 + part.getDoubleValue();
    return total;
}

} // namespace skjar::youtube
