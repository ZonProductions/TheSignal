#!/usr/bin/env python
"""
MP4 Compressor — targets ~2MB output size.
Usage:
    python compress_mp4.py input.mp4
    python compress_mp4.py input.mp4 -o output.mp4
    python compress_mp4.py input.mp4 --target 3  (target 3MB instead)
    python compress_mp4.py *.mp4               (batch mode)
"""

import subprocess
import sys
import os
import argparse
import json
import shutil

# Find ffmpeg/ffprobe - check PATH first, then winget install location
def _find_tool(name):
    found = shutil.which(name)
    if found:
        return found
    # Winget installs here
    winget_dir = os.path.expanduser("~/AppData/Local/Microsoft/WinGet/Packages")
    if os.path.isdir(winget_dir):
        for root, dirs, files in os.walk(winget_dir):
            if f"{name}.exe" in files:
                return os.path.join(root, f"{name}.exe")
    print(f"ERROR: {name} not found. Install ffmpeg: winget install ffmpeg")
    sys.exit(1)

FFMPEG = _find_tool("ffmpeg")
FFPROBE = _find_tool("ffprobe")


def get_duration(path):
    """Get video duration in seconds via ffprobe."""
    result = subprocess.run(
        [FFPROBE, "-v", "quiet", "-print_format", "json", "-show_format", path],
        capture_output=True, text=True
    )
    info = json.loads(result.stdout)
    return float(info["format"]["duration"])


def compress(input_path, output_path, target_mb=2.0):
    """Two-pass compress an MP4 to a target file size."""
    if not os.path.isfile(input_path):
        print(f"  SKIP: {input_path} not found")
        return False

    duration = get_duration(input_path)
    input_size_mb = os.path.getsize(input_path) / (1024 * 1024)

    if input_size_mb <= target_mb:
        print(f"  SKIP: {input_path} already {input_size_mb:.1f}MB (under {target_mb}MB)")
        return False

    # Target bitrate: (target_bytes * 8) / duration, minus ~64kbps for audio
    target_bits = target_mb * 1024 * 1024 * 8
    audio_bitrate = 64  # kbps
    video_bitrate = int((target_bits / duration - audio_bitrate * 1000) / 1000)  # kbps

    if video_bitrate < 50:
        print(f"  WARN: {input_path} is {duration:.0f}s — video bitrate would be {video_bitrate}kbps, may look rough")
        video_bitrate = 50

    print(f"  {input_path}: {input_size_mb:.1f}MB, {duration:.1f}s -> {video_bitrate}kbps video + {audio_bitrate}kbps audio")

    # Pass 1
    subprocess.run([
        FFMPEG, "-y", "-i", input_path,
        "-c:v", "libx264", "-b:v", f"{video_bitrate}k",
        "-pass", "1", "-an", "-f", "null",
        "NUL" if os.name == "nt" else "/dev/null"
    ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    # Pass 2
    subprocess.run([
        FFMPEG, "-y", "-i", input_path,
        "-c:v", "libx264", "-b:v", f"{video_bitrate}k",
        "-pass", "2",
        "-c:a", "aac", "-b:a", f"{audio_bitrate}k",
        "-movflags", "+faststart",
        output_path
    ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    final_size = os.path.getsize(output_path) / (1024 * 1024)
    print(f"  DONE: {output_path} -> {final_size:.2f}MB")
    return True


def main():
    parser = argparse.ArgumentParser(description="Compress MP4s to a target size")
    parser.add_argument("files", nargs="+", help="Input MP4 file(s)")
    parser.add_argument("-o", "--output", help="Output path (single file only)")
    parser.add_argument("--target", type=float, default=2.0, help="Target size in MB (default: 2)")
    args = parser.parse_args()

    for f in args.files:
        if args.output and len(args.files) == 1:
            out = args.output
        else:
            name, ext = os.path.splitext(f)
            out = f"{name}_compressed{ext}"

        compress(f, out, args.target)

    # Clean up ffmpeg 2-pass log files
    for log in ["ffmpeg2pass-0.log", "ffmpeg2pass-0.log.mbtree"]:
        if os.path.exists(log):
            os.remove(log)


if __name__ == "__main__":
    main()
