// Stands in for Live's API observers, reporting the song position only once a
// second — sparser than Live — to show the video plays straight through
// between reports. A link with ?t=40 at once; stopped until 6 s; playing at
// 120 BPM; at 12 s the song position jumps back 4 beats (a loop); stopped at
// 17 s; playing again from 20 s to 26 s.
inlets = 1;
outlets = 1;
var t0 = Date.now();
var beats = 0, last = 0, lastReport = -1, wasPlaying = false, jumped = false;

function bang() {
    var t = (Date.now() - t0) / 1000;
    if (last === 0) {
        outlet(0, "tempo", 120);
        outlet(0, "url", "https://youtu.be/M7lc1UVf-VE?t=40");
    }
    var playing = (t > 6 && t < 17) || (t > 20 && t < 26);
    if (playing) beats += (t - last) * 2;
    last = t;
    if (playing !== wasPlaying) { outlet(0, "transport", playing ? 1 : 0); wasPlaying = playing; }
    if (! jumped && t > 12) { jumped = true; beats -= 4; outlet(0, "beats", beats); lastReport = t; return; }
    if (t - lastReport >= 1) { outlet(0, "beats", beats); lastReport = t; }
}
