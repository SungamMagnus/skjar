#!/usr/bin/env python3
"""Build the shareable Skjár device: python3 make_share.py

Produces dist/Skjar-for-Live/ and dist/Skjar-for-Live.zip with one self-contained (frozen) .amxd and a short README.
Frozen = skjar.js and the server are embedded in the device, and the player page is written into the server, so the
people you share it with install nothing else and keep no other file next to it.
"""
import json
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

HERE = Path(__file__).parent
DEV = HERE / "devices"
BUILD = HERE / "build" / "share"
DIST = HERE / "dist" / "Skjar-for-Live"
sys.path.insert(0, str(HERE))
import freeze  # noqa: E402

README = """Skjár for Ableton Live
======================

A YouTube video inside a Live device, with its sound in the track, following Live's transport.

  1. Drag Skjár.amxd onto an audio track (or into your User Library: Presets > Audio Effects > Max Audio Effect).
  2. Paste a YouTube link into the field and press Return.
  3. Set the cue: type it in seconds, or scrub the video to the spot and press SET.
  4. Press play in Live. The video starts from the cue, and its sound comes out of the track.

Sync
  From cue   every start begins at the cue; the video then plays straight on until Live stops.
  Timeline   the cue lines up with the start of the arrangement, so starting at bar 9 plays the video from where
             bar 9 falls.
  Nudge      moves the video earlier (+) or later (-) against Live, in milliseconds.

The link, cue and the other settings are saved with the Live set, and in device presets (the save button in the
device's title bar).

Needs
  Ableton Live 12.2 or later with Max for Live, on a Mac or PC, and an internet connection.

If macOS says the device cannot be opened
  The device is not notarized. Open System Settings > Privacy & Security and choose "Allow Anyway", or run
      xattr -dr com.apple.quarantine "Skjár.amxd"
  in Terminal and open it again.

Notes
  * Videos whose owners do not allow embedding will not play; the status line under the video says so.
  * Ads can play, and their sound comes through too.
  * Recording or bouncing someone else's YouTube audio into a project can raise copyright questions.
  * Hover a control in Live (Info View) for a one-line explanation.
"""


def run(*cmd):
    subprocess.run([str(c) for c in cmd], check=True)


def main():
    run(sys.executable, HERE / "make_device.py")

    # The server with the page written into it, so the frozen device carries everything.
    if BUILD.exists():
        shutil.rmtree(BUILD)
    BUILD.mkdir(parents=True)
    page = (DEV / "skjar_player.html").read_text(encoding="utf-8")
    server = (DEV / "skjar_server.js").read_text(encoding="utf-8")
    assert "const INLINE_PAGE = null;" in server
    (BUILD / "skjar_server.js").write_text(
        server.replace("const INLINE_PAGE = null;", f"const INLINE_PAGE = {json.dumps(page)};"), encoding="utf-8")
    shutil.copy(DEV / "skjar.js", BUILD / "skjar.js")

    if DIST.parent.exists():
        shutil.rmtree(DIST.parent)
    DIST.mkdir(parents=True)
    freeze.freeze(DEV / "Skjár.amxd", DIST / "Skjár.amxd", [], [BUILD / "skjar.js", BUILD / "skjar_server.js"])
    (DIST / "README.txt").write_text(README, encoding="utf-8")

    zpath = DIST.parent / "Skjar-for-Live.zip"
    with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED) as z:
        for f in sorted(DIST.iterdir()):
            z.write(f, f"{DIST.name}/{f.name}")
    for f in sorted(DIST.iterdir()):
        print(f"  {f.name}: {f.stat().st_size} bytes")
    print(f"{zpath}: {zpath.stat().st_size} bytes")


if __name__ == "__main__":
    main()
