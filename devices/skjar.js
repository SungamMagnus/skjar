/*
 * Skjár: keeps the YouTube player in jweb~ in step with Live's transport.
 *
 * In:   port <n>                    the local server is up (skjar_server.js)
 *       url <text> / restore <text>  a link typed in, or the one saved with the set
 *       cue <s>, mode <0|1>, nudge <ms>, setcue, gocue
 *       init                        from live.thisdevice: the Live API may be used now
 *       transport <0|1>, beats <b>, tempo <bpm>   what the Live API observers report (tests send them directly)
 *       state <ready> <state> <time> <duration> <error>, video <title>     from the page
 *
 * Out:  0 jweb~   1 pattr (the saved link)   2 link field   3 cue numbox
 *       4 status line   5 title line
 *
 * The page only reports and obeys; every decision is made here, at 30 Hz.
 */
autowatch = 1;
inlets = 1;
outlets = 6;

var CORAL = [0.929, 0.506, 0.349, 1.0];
var TEAL = [0.322, 0.690, 0.643, 1.0];
var GREY = [0.55, 0.55, 0.52, 1.0];

/** How far the video may wander before it is pulled back, and how long to
    leave it alone after each pull — a seek takes a moment to land, and judging
    it before then only causes another. */
var DRIFT_LIMIT = 0.35, SETTLE = 1.5;

var port = 0, url = "", videoId = "";
var cue = 0, follow = 0, nudge = 0;
/* Live's position as last reported, and when. Between reports it is carried
   forward on the clock, so the video plays straight on however rarely Live
   says where it is; only a report that disagrees with that is a jump. */
var host = { playing: 0, beats: 0, tempo: 120, stamp: 0 };
var observers = [];
var page = { ready: 0, state: -1, time: 0, duration: 0, error: 0, stamp: 0 };
var title = "";

var hostWasPlaying = 0, started = 0, wasReady = 0;
var startedAt = 0, lastHostSeconds = 0, lastCorrection = 0;
var shownStatus = null, shownTitle = null, shownColour = null;

function now() { return Date.now() / 1000; }

/* ── Links ─────────────────────────────────────────────────────────────── */

function parseTimestamp(t) {
    t = String(t || "").toLowerCase();
    if (! t) return -1;
    if (/^[0-9.]+$/.test(t)) return parseFloat(t);
    var m = /^(?:(\d+)h)?(?:(\d+)m)?(?:(\d+(?:\.\d+)?)s?)?$/.exec(t);
    if (! m) return -1;
    return (parseInt(m[1] || 0) * 3600) + (parseInt(m[2] || 0) * 60) + parseFloat(m[3] || 0);
}

function query(q, key) {
    var parts = q.split("&");
    for (var i = 0; i < parts.length; ++i) {
        var kv = parts[i].split("=");
        if (kv[0] === key) return decodeURIComponent(kv[1] || "");
    }
    return "";
}

/** Watch, youtu.be, shorts, embed, live and music links, or a bare id. */
function parseLink(text) {
    var s = String(text).replace(/^\s+|\s+$/g, "");
    var isId = function (x) { return /^[A-Za-z0-9_-]{11}$/.test(x); };
    if (isId(s)) return { id: s, start: -1 };

    s = s.replace(/^[a-z]+:\/\//i, "").replace(/^(www|m|music)\./i, "");
    var path = s.split(/[?#]/)[0];
    var q = (s.split("?")[1] || "").split("#")[0];
    var start = parseTimestamp(query(q, "t"));
    if (start < 0) start = parseTimestamp(query(q, "start"));

    var id = "", m;
    if ((m = /^youtu\.be\/([A-Za-z0-9_-]+)/i.exec(path))) id = m[1];
    else if (/^youtube(-nocookie)?\.com\/watch/i.test(path)) id = query(q, "v");
    else if ((m = /^youtube(?:-nocookie)?\.com\/(?:shorts|embed|live|v)\/([A-Za-z0-9_-]+)/i.exec(path))) id = m[1];
    return isId(id) ? { id: id, start: start } : null;
}

function formatTime(s) {
    s = Math.max(0, s);
    var cs = Math.round(s * 100);
    var h = Math.floor(cs / 360000), m = Math.floor(cs / 6000) % 60, sec = Math.floor(cs / 100) % 60, c = cs % 100;
    var two = function (v) { return (v < 10 ? "0" : "") + v; };
    return (h > 0 ? h + ":" + two(m) : m) + ":" + two(sec) + "." + two(c);
}

/* ── Loading ───────────────────────────────────────────────────────────── */

function readPage() {
    if (! port) return;
    var base = "http://127.0.0.1:" + port + "/";
    page.ready = 0; page.error = 0; wasReady = 0; started = 0; title = "";
    outlet(0, "read", videoId ? base + "?v=" + videoId + "&t=" + Math.floor(cue) : base);
}

function useLink(text, fromUser) {
    var link = parseLink(text);
    if (! link) {
        if (fromUser) say(String(text).length ? "not a youtube link" : "", CORAL);
        return;
    }
    url = String(text);
    videoId = link.id;
    if (fromUser) {
        outlet(1, "set", url);
        // A link carrying its own start time (?t=) brings it in as the cue.
        if (link.start >= 0) outlet(3, link.start);
    }
    readPage();
}

function anything() {
    var args = arrayfromargs(arguments);
    switch (messagename) {
    case "url":
        // textedit may lead with its own "text" selector, depending on its output mode.
        if (args[0] === "text") args.shift();
        useLink(args.join(" "), true);
        break;
    case "restore": outlet(2, "set", args.join(" ")); useLink(args.join(" "), false); break;
    case "state":
        page.ready = args[0]; page.state = args[1]; page.time = args[2];
        page.duration = args[3]; page.error = args[4]; page.stamp = now();
        break;
    case "video":   title = args.join(" "); break;
    case "port":    port = args[0]; readPage(); break;

    case "cue":     cue = Math.max(0, args[0]); if (! host.playing && page.ready) park(); break;
    case "mode":
        follow = args[0] ? 1 : 0;
        if (host.playing) { startedAt = now(); started = 0; }
        else if (page.ready) park();
        break;
    case "nudge":   nudge = args[0]; break;

    case "transport": onTransport(args[0]); break;
    case "beats":   onBeats(args[0]); break;
    case "tempo":   onTempo(args[0]); break;
    }
}

/* ── Cue buttons ───────────────────────────────────────────────────────── */

function setcue() { if (page.ready) outlet(3, page.time); }
function gocue() { park(); }

/* ── Following the transport ───────────────────────────────────────────── */

/* ── Live's transport ──────────────────────────────────────────────────── */

function init() {
    observe("is_playing", onTransport);
    observe("tempo", onTempo);
    observe("current_song_time", onBeats);
}

function observe(property, handler) {
    var api = new LiveAPI(function (args) {
        if (args[0] === property) handler(args[1]);
    }, "live_set");
    api.property = property;
    observers.push(api);
}

function hostSeconds() {
    var at = host.beats * 60 / host.tempo;
    return host.playing ? at + (now() - host.stamp) : at;
}

/** Where Live now is, in beats, carried forward to this moment. */
function beatsNow() { return hostSeconds() * host.tempo / 60; }

function onTransport(v) {
    v = v ? 1 : 0;
    if (v === host.playing) return;
    host.beats = beatsNow();       // carry the position up to the change
    host.stamp = now();
    host.playing = v;
}

function onTempo(bpm) {
    if (! (bpm > 0)) return;
    host.beats = beatsNow();
    host.stamp = now();
    host.tempo = bpm;
}

function onBeats(b) {
    var reported = b * 60 / host.tempo;
    var predicted = hostSeconds();
    host.beats = b;
    host.stamp = now();

    // A jump the transport did not play through: a loop, or the play head
    // moved while running. Only the timeline follows it; from the cue, the
    // video plays straight on until Live stops.
    if (follow && host.playing && hostWasPlaying && Math.abs(reported - predicted) > 0.5)
        started = 0;
}

/** Stopped, the picture shows where the next start will land: the cue, or in
    timeline mode wherever Live's play head now sits. */
function park() {
    var at = follow ? cue + hostSeconds() + nudge / 1000 : cue;
    outlet(0, "park", Math.max(0, at));
}

/** Where the video should be now. Timeline: the cue plus Live's position.
    From cue: the cue plus however long Live has been playing, on the clock —
    so loops and jumps in Live do not touch it. */
function expected(secs) {
    var t = follow ? secs : now() - startedAt;
    return Math.max(0, cue + t + nudge / 1000);
}

function tick() {
    var secs = hostSeconds();
    var becameReady = page.ready && ! wasReady;
    wasReady = page.ready;

    if (host.playing !== hostWasPlaying) {
        hostWasPlaying = host.playing;
        started = 0;
        if (host.playing) startedAt = now();
        else if (page.ready) park();
    } else if (! host.playing) {
        if (becameReady || (follow && page.ready && Math.abs(secs - lastHostSeconds) > 0.01)) park();
    }
    lastHostSeconds = secs;

    if (host.playing && page.ready) {
        var want = expected(secs);
        if (! started) {
            outlet(0, "start", want);
            started = 1;
            lastCorrection = now();
        } else if (now() - lastCorrection >= SETTLE && now() - page.stamp < 0.3) {
            if (page.state === 1) {
                if (Math.abs(page.time - want) > DRIFT_LIMIT) {
                    outlet(0, "seek", want);
                    lastCorrection = now();
                }
            } else if (page.state !== 3 && page.state !== 0) {
                // Paused or cued while Live plays: clicked in the player, or a
                // start that never took.
                outlet(0, "start", want);
                lastCorrection = now();
            }
        }
    }

    show();
}

/* ── Display ───────────────────────────────────────────────────────────── */

function errorText(code) {
    switch (code) {
    case 2:   return "bad video id";
    case 5:   return "cannot play this video here";
    case 100: return "video not found or private";
    case 101:
    case 150: return "the owner does not allow embedding";
    case 152:
    case 153: return "youtube refused the player";
    default:  return "player error " + code;
    }
}

function say(text, colour) {
    if (text !== shownStatus) { outlet(4, "set", text); shownStatus = text; }
    if (colour !== shownColour) { outlet(4, "textcolor", colour); shownColour = colour; }
}

function show() {
    if (page.error) say(errorText(page.error), CORAL);
    else if (! videoId) say(port ? "paste a link, press return" : "starting", GREY);
    else if (! page.ready) say("loading", GREY);
    else say((page.state === 1 || page.state === 3 ? "PLAY " : "STOP ") + formatTime(page.time)
             + " / " + formatTime(page.duration).replace(/\.\d\d$/, ""), page.state === 1 ? TEAL : GREY);

    if (title !== shownTitle) { outlet(5, "set", title.length ? title : " "); shownTitle = title; }
}

var timer = new Task(tick);
timer.interval = 33;
timer.repeat();

function loadbang() { show(); }
function notifydeleted() {
    timer.cancel();
    for (var i = 0; i < observers.length; ++i) observers[i].property = "";
}
