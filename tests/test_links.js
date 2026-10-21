// node tests/test_links.js — the link parser and time format in skjar.js,
// run outside Max with its Max-only globals stubbed out.
const fs = require("fs");
const path = require("path");
const src = fs.readFileSync(path.join(__dirname, "..", "devices", "skjar.js"), "utf8");
const Task = function () { this.repeat = () => {}; this.cancel = () => {}; };
const api = new Function("arrayfromargs", "outlet", "Task", "messagename", src + "\nreturn { parseLink, formatTime };")
    (() => [], () => {}, Task, "");

let failures = 0;
function expect(ok, what) { console.log((ok ? "ok    " : "FAIL  ") + what); if (! ok) failures++; }

const cases = [
    ["https://www.youtube.com/watch?v=dQw4w9WgXcQ", "dQw4w9WgXcQ", -1],
    ["youtube.com/watch?feature=share&v=dQw4w9WgXcQ&t=90", "dQw4w9WgXcQ", 90],
    ["https://youtu.be/dQw4w9WgXcQ?t=1m30s", "dQw4w9WgXcQ", 90],
    ["https://m.youtube.com/watch?v=dQw4w9WgXcQ#x", "dQw4w9WgXcQ", -1],
    ["https://music.youtube.com/watch?v=dQw4w9WgXcQ&list=RD", "dQw4w9WgXcQ", -1],
    ["https://www.youtube.com/shorts/dQw4w9WgXcQ", "dQw4w9WgXcQ", -1],
    ["https://www.youtube.com/embed/dQw4w9WgXcQ?start=12", "dQw4w9WgXcQ", 12],
    ["https://www.youtube.com/live/dQw4w9WgXcQ?si=abc", "dQw4w9WgXcQ", -1],
    ["  dQw4w9WgXcQ  ", "dQw4w9WgXcQ", -1],
    ["https://vimeo.com/123456", null],
    ["https://www.youtube.com/watch?v=short", null],
    ["hello", null],
];
for (const [text, id, start] of cases) {
    const got = api.parseLink(text);
    const ok = id === null ? got === null : got && got.id === id && got.start === start;
    expect(ok, `parse ${text.trim()} -> ${JSON.stringify(got)}`);
}
expect(api.formatTime(75.5) === "1:15.50", "format 75.5 -> " + api.formatTime(75.5));
expect(api.formatTime(3725) === "1:02:05.00", "format 3725 -> " + api.formatTime(3725));
console.log(failures ? `\n${failures} failed` : "\nall ok");
process.exit(failures ? 1 : 0);
