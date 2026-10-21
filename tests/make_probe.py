"""A headless check for jweb~: load the player page from a URL, log what it
says and how loud its audio outlets are, and drive it through park / start.

    python3 tests/make_probe.py <url> <log file>   -> tests/out/probe.maxpat
"""
import json, sys
from pathlib import Path

url, logfile = sys.argv[1], sys.argv[2]
here = Path(__file__).parent
boxes, lines = [], []

def box(text, ins, outs, x, y, w=120, cls="newobj", **extra):
    oid = f"obj-{len(boxes) + 1}"
    b = {"id": oid, "maxclass": cls, "numinlets": ins, "numoutlets": outs, "patching_rect": [x, y, w, 22]}
    if text is not None:
        b["text"] = text
    b.update(extra)
    boxes.append({"box": b})
    return oid

def line(a, ao, b, bi):
    lines.append({"patchline": {"source": [a, ao], "destination": [b, bi]}})

lb = box("loadbang", 1, 1, 20, 20)
dsp = box("; dsp start", 2, 1, 20, 50, cls="message")
dl = box("delay 1500", 2, 1, 160, 20)
rd = box(f"read {url}", 2, 1, 160, 50, w=600, cls="message")
web = box(None, 1, 3, 20, 100, w=320, cls="jweb~", outlettype=["signal", "signal", ""])
boxes[-1]["box"]["patching_rect"] = [20, 100, 320, 180]
js = box(f"js {here / 'probe_log.js'} {logfile}", 1, 1, 380, 300, w=500)
pl = box("peakamp~ 1000", 2, 1, 20, 300)
pr = box("peakamp~ 1000", 2, 1, 160, 300)
al = box("prepend ampL", 1, 1, 20, 330)
ar = box("prepend ampR", 1, 1, 160, 330)
for a, ao, b, bi in [(lb, 0, dsp, 0), (lb, 0, dl, 0), (dl, 0, rd, 0), (rd, 0, web, 0), (web, 2, js, 0), (js, 0, web, 0),
                     (web, 0, pl, 0), (web, 1, pr, 0), (pl, 0, al, 0), (pr, 0, ar, 0), (al, 0, js, 0), (ar, 0, js, 0)]:
    line(a, ao, b, bi)

patch = {"patcher": {"fileversion": 1, "appversion": {"major": 9, "minor": 1, "revision": 5, "architecture": "x64", "modernui": 1},
                     "classnamespace": "box", "rect": [100, 100, 900, 420], "boxes": boxes, "lines": lines}}
out = here / "out"
out.mkdir(exist_ok=True)
(out / "probe.maxpat").write_text(json.dumps(patch, indent=1))
print(out / "probe.maxpat")
