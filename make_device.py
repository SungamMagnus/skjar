"""Generate the Skjár Max for Live device from code, so layout and wiring are reproducible.

    python3 make_device.py            # writes devices/Skjár.amxd

  Skjár  (audio effect)  a YouTube video in jweb~, its sound mixed into the track, following Live's transport.

  node.script skjar_server.js   serves the player page on localhost (the embed will not play from disk)
  jweb~                         shows the page and puts out its sound
  js skjar.js                   the following: Live's transport in (Live API), start / park / seek out

The Patcher class is Nóta's (nota-maxforlive/make_devices.py), so both devices are built and look the same way.
The container is the unfrozen .amxd layout: 'ampf' header, device type, 'ptch' + length, then the patcher JSON.
"""
import json
import struct
from pathlib import Path

HERE = Path(__file__).parent
OUT = HERE / "devices"

CORAL = [0.929, 0.506, 0.349, 1.0]
TEAL = [0.322, 0.690, 0.643, 1.0]
STEEL = [0.478, 0.651, 0.812, 1.0]
INK = [0.941, 0.925, 0.886, 1.0]
GREY = [0.55, 0.55, 0.52, 1.0]
DARK = [0.11, 0.11, 0.10, 1.0]


class Patcher:
    def __init__(self, device_width, kind):
        self.boxes, self.lines, self.params = [], [], {}
        self.n = 0
        self.width = device_width
        self.kind = kind  # 'aaaa' audio effect, 'mmmm' midi effect
        self.bank = []

    def _id(self):
        self.n += 1
        return f"obj-{self.n}"

    def add(self, maxclass, text=None, ins=1, outs=1, outtypes=None, rect=(20, 20, 60, 20), pres=None, **extra):
        oid = self._id()
        box = {"id": oid, "maxclass": maxclass, "numinlets": ins, "numoutlets": outs,
               "patching_rect": list(rect)}
        if outtypes is not None:
            box["outlettype"] = outtypes
        elif outs:
            box["outlettype"] = [""] * outs
        if text is not None:
            box["text"] = text
        if pres is not None:
            box["presentation"] = 1
            box["presentation_rect"] = list(pres)
        box.update(extra)
        self.boxes.append({"box": box})
        return oid

    def obj(self, text, ins, outs, rect, outtypes=None, **extra):
        return self.add("newobj", text=text, ins=ins, outs=outs, rect=rect, outtypes=outtypes, **extra)

    def line(self, src, src_out, dst, dst_in):
        self.lines.append({"patchline": {"destination": [dst, dst_in], "source": [src, src_out]}})

    # ---- Live UI objects (each is also a Live parameter)
    def _param(self, oid, long, short, ptype, extra):
        valueof = {"parameter_longname": long, "parameter_shortname": short, "parameter_type": ptype}
        valueof.update(extra)
        self.params[oid] = [long, short, 0]
        return {"valueof": valueof}

    def dial(self, long, short, lo, hi, init, unit, rect, pres, color, ptype=0, annotation=None, exponent=None, hidden=False):
        oid = self._id()
        box = {"id": oid, "maxclass": "live.dial", "numinlets": 1, "numoutlets": 2, "outlettype": ["", "float"],
               "parameter_enable": 1, "patching_rect": list(rect), "presentation": 1, "presentation_rect": list(pres),
               "activedialcolor": color, "activeneedlecolor": INK,
               "saved_attribute_attributes": self._param(oid, long, short, ptype, {
                   "parameter_initial": [init], "parameter_initial_enable": 1, "parameter_mmin": lo, "parameter_mmax": hi,
                   "parameter_unitstyle": unit}),
               "varname": short}
        if hidden:   # still a Live parameter (automation, presets), just not drawn in the device
            box["presentation"] = 0
            del box["presentation_rect"]
        if exponent:
            box["saved_attribute_attributes"]["valueof"]["parameter_exponent"] = exponent
        if annotation:
            box["annotation"] = annotation
        self.boxes.append({"box": box})
        self.bank.append(long)
        return oid

    def numbox(self, long, short, lo, hi, init, rect, pres, annotation=None, ptype=1, unit=0):
        oid = self._id()
        box = {"id": oid, "maxclass": "live.numbox", "numinlets": 1, "numoutlets": 2, "outlettype": ["", "float"],
               "parameter_enable": 1, "patching_rect": list(rect), "presentation": 1, "presentation_rect": list(pres),
               "saved_attribute_attributes": self._param(oid, long, short, ptype, {
                   "parameter_initial": [init], "parameter_initial_enable": 1, "parameter_mmin": lo, "parameter_mmax": hi,
                   "parameter_unitstyle": unit}),
               "varname": short}
        if annotation:
            box["annotation"] = annotation
        self.boxes.append({"box": box})
        return oid

    def tab(self, long, short, options, init, rect, pres, color, annotation=None, vertical=False):
        oid = self._id()
        lines = len(options) if vertical else 1   # live.tab lays its options out in this many rows
        box = {"id": oid, "maxclass": "live.tab", "numinlets": 1, "numoutlets": 3, "outlettype": ["", "", "float"],
               "parameter_enable": 1, "patching_rect": list(rect), "presentation": 1, "presentation_rect": list(pres),
               "activebgoncolor": color, "num_lines_patching": lines, "num_lines_presentation": lines,
               "saved_attribute_attributes": self._param(oid, long, short, 2, {
                   "parameter_enum": options, "parameter_initial": [init], "parameter_initial_enable": 1,
                   "parameter_mmax": len(options) - 1}),
               "varname": short}
        if annotation:
            box["annotation"] = annotation
        self.boxes.append({"box": box})
        return oid

    def menu(self, long, short, options, init, rect, pres, fontsize=None):
        oid = self._id()
        box = {"id": oid, "maxclass": "live.menu", "numinlets": 1, "numoutlets": 3, "outlettype": ["", "", "float"],
               "parameter_enable": 1, "patching_rect": list(rect), "presentation": 1, "presentation_rect": list(pres),
               "saved_attribute_attributes": self._param(oid, long, short, 2, {
                   "parameter_enum": options, "parameter_initial": [init], "parameter_initial_enable": 1,
                   "parameter_mmax": len(options) - 1}),
               "varname": short}
        self.boxes.append({"box": box})
        return oid

    def toggle(self, long, short, text, init, rect, pres, color, annotation=None):
        oid = self._id()
        box = {"id": oid, "maxclass": "live.text", "numinlets": 1, "numoutlets": 2, "outlettype": ["", "float"],
               "parameter_enable": 1, "patching_rect": list(rect), "presentation": 1, "presentation_rect": list(pres),
               "mode": 1, "text": text, "texton": text, "activebgoncolor": color,
               "saved_attribute_attributes": self._param(oid, long, short, 2, {
                   "parameter_enum": ["off", "on"], "parameter_initial": [init], "parameter_initial_enable": 1,
                   "parameter_mmax": 1}),
               "varname": short}
        if annotation:
            box["annotation"] = annotation
        self.boxes.append({"box": box})
        return oid

    def button(self, long, short, text, rect, pres):
        oid = self._id()
        box = {"id": oid, "maxclass": "live.text", "numinlets": 1, "numoutlets": 2, "outlettype": ["", "float"],
               "parameter_enable": 1, "patching_rect": list(rect), "presentation": 1, "presentation_rect": list(pres),
               "mode": 0, "text": text, "texton": text,
               "saved_attribute_attributes": self._param(oid, long, short, 2, {
                   "parameter_enum": ["off", "on"], "parameter_mmax": 1}),
               "varname": short}
        self.boxes.append({"box": box})
        return oid

    def comment(self, text, rect, pres, justification=0, fontsize=None, color=None):
        extra = {"textjustification": justification}
        if fontsize:
            extra["fontsize"] = fontsize
        if color:
            extra["textcolor"] = color
        return self.add("live.comment", text=text, ins=1, outs=0, outtypes=[], rect=rect, pres=pres, **extra)

    def panel(self, rect, pres, color):
        return self.add("panel", ins=1, outs=0, outtypes=[], rect=rect, pres=pres, bgcolor=color, border=0, rounded=0,
                        mode=0, background=1, ignoreclick=1)

    # ---- output
    def json(self):
        parameters = dict(self.params)
        banks = [self.bank[i:i + 8] for i in range(0, len(self.bank), 8)]
        parameters["parameterbanks"] = {str(i): {"index": i, "name": "", "parameters": b + ["-"] * (8 - len(b))} for i, b in enumerate(banks)}
        parameters["inherited_shortname"] = 1
        patcher = {
            "fileversion": 1,
            "appversion": {"major": 8, "minor": 5, "revision": 8, "architecture": "x64", "modernui": 1},
            "classnamespace": "box",
            "rect": [60.0, 100.0, 1000.0, 700.0],
            "openrect": [0.0, 0.0, 0.0, 169.0],
            "bglocked": 0,
            "openinpresentation": 1,
            "default_fontsize": 10.0,
            "default_fontface": 0,
            "default_fontname": "Menlo",
            "gridonopen": 1,
            "gridsize": [8.0, 8.0],
            "gridsnaponopen": 1,
            "objectsnaponopen": 1,
            "statusbarvisible": 2,
            "toolbarvisible": 1,
            "lefttoolbarpinned": 0,
            "toptoolbarpinned": 0,
            "righttoolbarpinned": 0,
            "bottomtoolbarpinned": 0,
            "toolbars_unpinned_last_save": 0,
            "tallnewobj": 0,
            "boxanimatetime": 500,
            "enablehscroll": 1,
            "enablevscroll": 1,
            "devicewidth": float(self.width),
            "description": "",
            "digest": "",
            "tags": "",
            "style": "",
            "subpatcher_template": "",
            "assistshowspatchername": 0,
            "boxes": self.boxes,
            "lines": self.lines,
            "parameters": parameters,
            "latency": 0,
            "is_mpe": 0,
            "minimum_live_version": "",
            "minimum_max_version": "",
            "platform_compatibility": 0,
            "project": {"version": 1, "creationdate": 3590052838, "modificationdate": 3590052838, "viewrect": [0.0, 0.0, 300.0, 500.0],
                        "autoorganize": 0, "hideprojectwindow": 1, "showdependencies": 1, "autolocalize": 0,
                        "contents": {"patchers": {}}, "layout": {}, "searchpath": {}, "detailsvisible": 0,
                        "amxdtype": struct.unpack("<I", self.kind.encode())[0], "readonly": 0, "devpathtype": 0,
                        "devpath": ".", "sortmode": 0, "viewmode": 0},
            "saved_attribute_attributes": {"default_plcolor": {"expression": ""}},
        }
        return json.dumps({"patcher": patcher}, indent="\t") + "\n"

    def write(self, path):
        body = self.json().encode("utf-8")
        header = b"ampf" + struct.pack("<I", 4) + self.kind.encode() + b"meta" + struct.pack("<I", 4) + struct.pack("<I", 0)
        data = header + b"ptch" + struct.pack("<I", len(body)) + body
        Path(path).parent.mkdir(parents=True, exist_ok=True)
        Path(path).write_bytes(data)



    # ---- additions for Skjár
    def blob(self, name, long):
        """A pattr whose value is saved with the Live set and in device presets, but is not a control: the link."""
        oid = self._id()
        box = {"id": oid, "maxclass": "newobj", "text": f"pattr {name}", "numinlets": 1, "numoutlets": 3,
               "outlettype": ["", "", ""], "patching_rect": [40, 40, 120, 22], "parameter_enable": 1, "varname": name,
               "saved_attribute_attributes": self._param(oid, long, long, 3, {"parameter_invisible": 1})}
        self.boxes.append({"box": box})
        return oid

    def textfield(self, rect, pres):
        return self.add("textedit", ins=1, outs=4, outtypes=["", "int", "", ""], rect=rect, pres=pres,
                        keymode=1, lines=1, wordwrap=0, outputmode=1, fontsize=10.0, border=1.0,
                        bgcolor=DARK, textcolor=INK, bordercolor=[0.3, 0.3, 0.28, 1.0], tabmode=0,
                        parameter_enable=0)


def build():
    W = 548
    p = Patcher(W, "aaaa")
    GX = 236                      # the controls start here; the video panel is on the left

    # ---- the page: served on localhost, shown and heard through jweb~
    server = p.obj("node.script skjar_server.js @autostart 1", 1, 2, (600, 40, 260, 22))
    web = p.add("jweb~", ins=1, outs=3, outtypes=["signal", "signal", ""], rect=(40, 200, 224, 126),
                pres=(4, 4, 224, 126), latency=0, rendermode=0)
    js = p.obj("js skjar.js", 1, 6, (600, 300, 200, 22), [""] * 6)
    p.line(server, 0, js, 0)
    p.line(web, 2, js, 0)
    p.line(js, 0, web, 0)

    # ---- the link, saved with the set (and in presets) by a hidden pattr
    link = p.blob("skjar_link", "Link")
    restore = p.obj("prepend restore", 1, 1, (600, 80, 110, 22))
    p.line(link, 0, restore, 0)
    p.line(restore, 0, js, 0)
    p.line(js, 1, link, 0)

    lab = lambda t, x, y, w=80: p.comment(t, (0, 0, w, 12), (x, y, w, 12), 0, 9, GREY)
    lab("youtube link", GX, 2, 100)
    field = p.textfield((600, 120, 300, 18), (GX, 15, W - GX - 6, 17))
    furl = p.obj("prepend url", 1, 1, (600, 150, 90, 22))
    p.line(field, 0, furl, 0)
    p.line(furl, 0, js, 0)
    p.line(js, 2, field, 0)

    # ---- left panel: the video, then what it is and what it is doing
    p.panel((0, 0, 10, 10), (4, 4, 224, 161), DARK)
    title = p.comment(" ", (0, 0, 220, 14), (8, 132, 216, 14), 0, 9, INK)
    p.line(js, 5, title, 0)
    p.comment("S K J Á R", (0, 0, 100, 14), (8, 148, 80, 14), 0, 10, GREY)
    status = p.comment("starting", (0, 0, 140, 14), (84, 148, 140, 14), 2, 9, GREY)
    p.line(js, 4, status, 0)

    # ---- SYNC (coral): the cue, its two buttons, and how the video follows Live
    def into(ui, msg, y):
        pre = p.obj(f"prepend {msg}", 1, 1, (900, y, 100, 22))
        p.line(ui, 0, pre, 0)
        p.line(pre, 0, js, 0)

    R1 = 46
    lab("cue", GX, R1, 40)
    cue = p.numbox("Cue", "Cue", 0, 7200, 0, (760, 200, 60, 20), (GX, R1 + 13, 64, 17), ptype=0, unit=9,
                   annotation="Where the video starts when Live does, in seconds. Type it, or scrub the video to the "
                              "spot and press Set.")
    p.boxes[-1]["box"]["saved_attribute_attributes"]["valueof"]["parameter_units"] = "%.2f s"
    into(cue, "cue", 200)
    p.line(js, 3, cue, 0)

    def push(text, x, msg, y, annotation):
        b = p.add("live.text", ins=1, outs=2, outtypes=["", "float"], rect=(760, y, 40, 20), pres=(x, R1 + 13, 36, 17),
                  mode=0, text=text, texton=text, parameter_enable=0, annotation=annotation)
        sel = p.obj("sel 1", 2, 2, (810, y, 40, 22))
        m = p.add("message", text=msg, ins=2, outs=1, rect=(860, y, 60, 22))
        p.line(b, 0, sel, 0)
        p.line(sel, 0, m, 0)
        p.line(m, 0, js, 0)

    push("SET", GX + 68, "setcue", 240, "Takes the video's current position as the cue.")
    push("GO", GX + 106, "gocue", 270, "Jumps the video back to the cue.")

    lab("sync", GX + 156, R1, 40)
    mode = p.tab("Sync", "Sync", ["From cue", "Timeline"], 0, (760, 300, 90, 40), (GX + 156, R1 + 13, 74, 36), CORAL,
                 annotation="From cue: every time Live starts, the video starts at the cue. Timeline: the cue lines up "
                            "with the start of the arrangement, so starting at bar 9 plays the video from where bar 9 falls.",
                 vertical=True)
    into(mode, "mode", 300)

    R2 = 100
    lab("nudge", GX, R2, 50)
    nudge = p.numbox("Nudge", "Nudge", -1000, 1000, 0, (760, 360, 60, 20), (GX, R2 + 13, 64, 17), ptype=0, unit=2,
                     annotation="Moves the video earlier (positive) or later (negative) against Live, to make up for "
                                "the time the stream takes to reach the track.")
    into(nudge, "nudge", 360)

    # ---- the sound (teal): the video's, at its own volume, on top of whatever the track carries
    vol = p.dial("Volume", "Volume", -70, 6, 0, 4, (760, 420, 50, 48), (W - 62, R1 + 6, 56, 52), TEAL,
                 annotation="The level of the video's sound in the track.")
    plugin = p.obj("plugin~", 1, 2, (40, 40, 60, 22), ["signal", "signal"])
    plugout = p.obj("plugout~", 2, 0, (40, 500, 60, 22), [])
    gain = p.obj("dbtoa", 1, 1, (300, 400, 50, 22))
    ramp = p.obj("pack f 30", 2, 1, (300, 430, 60, 22))
    line = p.obj("line~", 2, 2, (300, 460, 50, 22), ["signal", "bang"])
    p.line(vol, 0, gain, 0)
    p.line(gain, 0, ramp, 0)
    p.line(ramp, 0, line, 0)
    for ch, x in ((0, 40), (1, 160)):
        g = p.obj("*~", 2, 1, (x, 400, 40, 22), ["signal"])
        s = p.obj("+~", 2, 1, (x, 460, 40, 22), ["signal"])
        p.line(web, ch, g, 0)
        p.line(line, 0, g, 1)
        p.line(g, 0, s, 0)
        p.line(plugin, ch, s, 1)
        p.line(s, 0, plugout, ch)

    # ---- Live's transport: skjar.js observes it through the Live API, which may only be used once the device is loaded
    ready = p.obj("live.thisdevice", 1, 3, (1100, 40, 100, 22), ["bang", "int", "int"])
    init = p.add("message", text="init", ins=2, outs=1, rect=(1100, 70, 40, 22))
    p.line(ready, 0, init, 0)
    p.line(init, 0, js, 0)
    return p


if __name__ == "__main__":
    dev = build()
    path = OUT / "Skjár.amxd"
    dev.write(path)
    print(path)
