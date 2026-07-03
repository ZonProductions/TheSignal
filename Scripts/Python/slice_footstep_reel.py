"""
Slice a footstep REEL into individual step one-shots (SFX_SHAMBLER_FOOTSTEP_01..N.wav).

A reel can't cadence-match dynamic locomotion (walk<->chase<->run bursts), so the C++
distance-based step driver (UZP_ShamblerBehaviorComponent) triggers ONE-SHOTS per stride.
This script produces those one-shots from the dev's chosen reel.

Method: ffmpeg -> 16-bit/44.1k temp copies (mono for analysis, stereo for slicing);
RMS-window onset detection (threshold over noise floor, min gap between steps);
each slice gets a short fade-in/out to kill clicks. SYSTEM python (not UE python).

Usage:  python Scripts/Python/slice_footstep_reel.py
Re-run safe: overwrites its own output slices only.
"""
import os
import struct
import subprocess
import sys
import wave

REEL = r"C:\Users\Ommei\workspace\TheSignal\Sfx\SFX_SHAMBLER_FOOTSTEPS.wav"
OUT_DIR = r"C:\Users\Ommei\workspace\TheSignal\Sfx\ShamblerFootsteps"
SCRATCH = os.environ.get("TEMP", r"C:\Windows\Temp")
BASE = "SFX_SHAMBLER_FOOTSTEP"

WIN_MS = 10          # RMS window
MIN_GAP_MS = 260     # two steps can't be closer than this
THRESH_MULT = 4.0    # onset = RMS > noise_floor * this
PRE_MS = 25          # slice starts this far BEFORE the onset (keep the transient attack)
MAX_LEN_MS = 650     # slice cap
FADE_MS = 15         # fade-in/out on every slice


def ffmpeg(args):
    r = subprocess.run(["ffmpeg", "-y", "-hide_banner", "-loglevel", "error"] + args,
                       capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError("ffmpeg failed: %s" % r.stderr)


def read_wav(path):
    with wave.open(path, "rb") as w:
        assert w.getsampwidth() == 2, "expected 16-bit"
        return (w.getnchannels(), w.getframerate(),
                list(struct.unpack("<%dh" % (w.getnframes() * w.getnchannels()),
                                   w.readframes(w.getnframes()))))


def main():
    if not os.path.isfile(REEL):
        sys.exit("REEL MISSING: %s" % REEL)
    os.makedirs(OUT_DIR, exist_ok=True)

    mono = os.path.join(SCRATCH, "sham_steps_mono.wav")
    stereo = os.path.join(SCRATCH, "sham_steps_16.wav")
    ffmpeg(["-i", REEL, "-ac", "1", "-ar", "44100", "-sample_fmt", "s16", mono])
    ffmpeg(["-i", REEL, "-ar", "44100", "-sample_fmt", "s16", stereo])

    _, rate, msamples = read_wav(mono)
    ch, srate, ssamples = read_wav(stereo)
    assert rate == srate

    win = max(1, rate * WIN_MS // 1000)
    rms = []
    for i in range(0, len(msamples) - win, win):
        acc = 0
        for s in msamples[i:i + win]:
            acc += s * s
        rms.append((acc / win) ** 0.5)
    floor = sorted(rms)[len(rms) // 5] + 1.0  # 20th percentile ~ noise floor
    thresh = floor * THRESH_MULT

    onsets, last = [], -10 ** 9
    min_gap_w = MIN_GAP_MS // WIN_MS
    for i, v in enumerate(rms):
        if v >= thresh and (i - last) >= min_gap_w:
            onsets.append(i * win)  # frame index in mono == frame index in stereo
            last = i
    if not onsets:
        sys.exit("NO ONSETS FOUND (floor=%.1f thresh=%.1f maxRMS=%.1f)" % (floor, thresh, max(rms)))

    pre = rate * PRE_MS // 1000
    max_len = rate * MAX_LEN_MS // 1000
    fade = rate * FADE_MS // 1000
    total_frames = len(ssamples) // ch

    written = []
    for n, on in enumerate(onsets):
        a = max(0, on - pre)
        b = min(total_frames, a + max_len)
        if n + 1 < len(onsets):
            b = min(b, onsets[n + 1] - rate * 30 // 1000)  # stop 30ms before next step
        if b - a < rate * 80 // 1000:
            continue  # degenerate sliver
        seg = ssamples[a * ch:b * ch]
        frames = len(seg) // ch
        for f in range(min(fade, frames)):          # fade-in
            g = f / fade
            for c in range(ch):
                seg[f * ch + c] = int(seg[f * ch + c] * g)
        for f in range(min(fade, frames)):          # fade-out
            g = f / fade
            idx = frames - 1 - f
            for c in range(ch):
                seg[idx * ch + c] = int(seg[idx * ch + c] * g)
        out = os.path.join(OUT_DIR, "%s_%02d.wav" % (BASE, len(written) + 1))
        with wave.open(out, "wb") as w:
            w.setnchannels(ch)
            w.setsampwidth(2)
            w.setframerate(rate)
            w.writeframes(struct.pack("<%dh" % len(seg), *seg))
        written.append(out)

    print("SLICED %d steps -> %s" % (len(written), OUT_DIR))
    for p in written:
        print("  %s (%.0f KB)" % (os.path.basename(p), os.path.getsize(p) / 1024))


if __name__ == "__main__":
    main()
