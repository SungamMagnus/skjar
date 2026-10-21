// Logs everything jweb~ says, and drives the page through a fixed script once
// it is ready: park at 30 s, start at 60 s, park at 10 s.
autowatch = 1;
inlets = 1;
outlets = 1;
var path = jsarguments[1];
var t0 = Date.now();
var phase = 0;

function log(s) {
    var f = new File(path, "readwrite");
    f.position = f.eof;
    f.writeline(((Date.now() - t0) / 1000).toFixed(1) + " " + s);
    f.close();
}

function anything() {
    var a = arrayfromargs(messagename, arguments);
    if (messagename === "ampL" || messagename === "ampR") { log(a.join(" ")); return; }
    if (messagename === "state" && a[1] == 1 && phase === 0) { phase = 1; step(); }
    if (messagename !== "state" || Math.random() < 0.2) log(a.join(" "));
}

function step() {
    var plan = [["park", 30, 4000], ["start", 60, 8000], ["park", 10, 4000]];
    var i = 0;
    var t = new Task(function () {
        if (i >= plan.length) { log("done"); arguments.callee.task.cancel(); return; }
        log("> " + plan[i][0] + " " + plan[i][1]);
        outlet(0, plan[i][0], plan[i][1]);
        t.interval = plan[i][2];
        i++;
    });
    t.interval = 500;
    t.repeat(plan.length + 1);
}
