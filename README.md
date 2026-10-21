# Skjár for Max for Live

A YouTube video in an Ableton Live device, with **its sound in the track**, following Live's transport. Paste a link,
set a cue, press play in Live: the video starts from the cue, and what it plays comes out of the device like any other
audio, so it can be heard, metered, routed and recorded in Live.

Needs Live 12.2 or later with Max for Live (Max 9's `jweb~`).

| | |
|---|---|
| **youtube link** | paste one and press Return. Watch, youtu.be, shorts, embed, live and music links, or a bare video id. A link with `?t=` sets the cue too |
| **cue** | where the video starts when Live does, in seconds. **SET** takes the video's current position (scrub there in the player first), **GO** jumps back to it |
| **sync** | **From cue**: every start begins at the cue, and the video then plays straight on until Live stops, whatever Live's loop or play head does meanwhile. **Timeline**: the cue lines up with the start of the arrangement, so starting at bar 9 plays the video from where bar 9 falls, and moving the play head while stopped moves the picture |
| **nudge** | moves the video earlier (+) or later (−) against Live, to make up for the time the stream takes to reach the track |
| **Volume** | the level of the video's sound; the track's own input passes through underneath it |

While playing, the video is pulled back whenever it drifts more than 0.35 s from where it should be.

The link, cue, sync mode, nudge and volume are saved with the Live set, and in Live's own device presets (the save
button in the device's title bar).

## How it works

| File | Does |
|---|---|
| `devices/Skjár.amxd` | the device (generated, do not edit by hand) |
| `devices/skjar_server.js` | `node.script`: serves the player page on `127.0.0.1`, on any free port |
| `devices/skjar_player.html` | the YouTube IFrame API, driven through `window.max` |
| `devices/skjar.js` | `js`: reads the transport from `plugsync~` and tells the page to start, park or seek |
| `make_device.py` | builds the `.amxd` (its `Patcher` class is Nóta's) |

`jweb~` is Chromium, and puts out whatever sound the page makes, which is what makes this possible at all: the VST
version's WebKit view will not give the sound up. The page is served over http rather than read from disk because the
YouTube embed refuses to play (error 153) in a page with no origin.

Keep the four files in `devices/` together: the device finds its scripts next to itself.

## Tests

```sh
node tests/test_links.js
```

checks the link parser. The rest need Max 9 (the one inside Live works) and open a window for about half a minute:

```sh
python3 tests/make_sync_probe.py "$PWD/tests/out/sync.log"
open -a "/Applications/Ableton Live 12 Suite.app/Contents/App-Resources/Max/Max.app" tests/out/sync_probe.maxpat
```

runs the device's server, page and sync script with a simulated transport and logs the commands, the player's state
and the level of its sound. `tests/make_probe.py <url> <log>` loads any URL in `jweb~` and logs the same.

## Limits

- Videos whose owners disable embedding will not play; the status line says so.
- Ads can play, and their sound comes through too.
- Sync is as close as YouTube's seeking allows: within a frame or two, not sample-accurate.
- Recording or bouncing someone else's YouTube audio into a project can raise copyright questions.
