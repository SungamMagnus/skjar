"""The device's own scripts — server, page, skjar.js — wired as in the device,
with fake_live.js standing in for plugsync~ and everything logged to a file.

    python3 tests/make_sync_probe.py <log file>   -> tests/out/sync_probe.maxpat
"""
import json, sys
from pathlib import Path

log = sys.argv[1]
here = Path(__file__).parent.resolve()
dev = here.parent / "devices"
boxes, lines = [], []

def box(text, ins, outs, x, y, w=160, cls="newobj", **extra):
    oid = f"obj-{len(boxes) + 1}"
    b = {"id": oid, "maxclass": cls, "numinlets": ins, "numoutlets": outs, "patching_rect": [x, y, w, 22]}
    if text is not None:
        b["text"] = text
    b.update(extra)
    boxes.append({"box": b})
    return oid

def line(a, ao, b, bi):
    lines.append({"patchline": {"source": [a, ao], "destination": [b, bi]}})

dsp = box("; dsp start", 2, 1, 20, 20, cls="message")
lb = box("loadbang", 1, 1, 20, 50)
server = box(f"node.script {dev / 'skjar_server.js'} @autostart 1", 1, 2, 200, 20, w=500)
web = box(None, 1, 3, 20, 100, cls="jweb~", outlettype=["signal", "signal", ""], latency=0)
boxes[-1]["box"]["patching_rect"] = [20, 100, 320, 180]
sk = box(f"js {dev / 'skjar.js'}", 1, 6, 380, 300, w=400, outlettype=[""] * 6)
cue = box("prepend cue", 1, 1, 600, 340)
metro = box("metro 30", 2, 1, 380, 100)
fake = box(f"js {here / 'fake_live.js'}", 1, 1, 380, 130, w=400)
logger = box(f"js {here / 'sync_log.js'} {log}", 1, 0, 380, 420, w=500)
cmd = box("prepend cmd", 1, 1, 380, 380)
pk = box("peakamp~ 1000", 2, 1, 20, 300)
amp = box("prepend amp", 1, 1, 20, 330)
for a, ao, b, bi in [(lb, 0, dsp, 0), (lb, 0, metro, 0), (metro, 0, fake, 0), (fake, 0, sk, 0), (server, 0, sk, 0),
                     (web, 2, sk, 0), (sk, 0, web, 0), (sk, 3, cue, 0), (cue, 0, sk, 0), (sk, 0, cmd, 0),
                     (cmd, 0, logger, 0), (web, 2, logger, 0), (web, 0, pk, 0), (pk, 0, amp, 0), (amp, 0, logger, 0)]:
    line(a, ao, b, bi)

patch = {"patcher": {"fileversion": 1, "appversion": {"major": 9, "minor": 1, "revision": 5, "architecture": "x64", "modernui": 1},
                     "classnamespace": "box", "rect": [100, 100, 900, 480], "boxes": boxes, "lines": lines}}
(here / "out").mkdir(exist_ok=True)
(here / "out" / "sync_probe.maxpat").write_text(json.dumps(patch, indent=1))
print(here / "out" / "sync_probe.maxpat")
