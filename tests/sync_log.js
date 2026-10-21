// Logs the sync script's commands, the page's state and the audio level.
inlets = 1;
outlets = 0;
var path = jsarguments[1];
var t0 = Date.now();
var n = 0;
function anything() {
    var a = arrayfromargs(messagename, arguments);
    if (messagename === "state" && (n++ % 5)) return;
    var f = new File(path, "readwrite");
    f.position = f.eof;
    f.writeline(((Date.now() - t0) / 1000).toFixed(1) + " " + a.join(" "));
    f.close();
}
