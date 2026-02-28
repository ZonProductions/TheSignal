"""Screen capture tool — click Start/Stop or press Ctrl+R to toggle. Frames saved to timestamped folder."""
import os, threading, time, tkinter as tk
from datetime import datetime
from PIL import ImageGrab
import keyboard

BASE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "captures")
os.makedirs(BASE, exist_ok=True)

capturing = False
frame_count = 0
out_dir = ""

def capture_loop():
    global frame_count, capturing
    while capturing:
        img = ImageGrab.grab()
        img.save(os.path.join(out_dir, f"{frame_count:04d}.png"), "PNG")
        frame_count += 1
        label.config(text=f"Capturing... {frame_count} frames")
        time.sleep(0.2)

def start():
    global capturing, frame_count, out_dir
    if capturing:
        return
    out_dir = os.path.join(BASE, datetime.now().strftime("%Y%m%d_%H%M%S"))
    os.makedirs(out_dir)
    frame_count = 0
    capturing = True
    btn_start.config(state="disabled")
    btn_stop.config(state="normal")
    threading.Thread(target=capture_loop, daemon=True).start()

def stop():
    global capturing
    if not capturing:
        return
    capturing = False
    btn_start.config(state="normal")
    btn_stop.config(state="disabled")
    label.config(text=f"Done — {frame_count} frames in {os.path.basename(out_dir)}")

def toggle(_=None):
    if capturing:
        root.after(0, stop)
    else:
        root.after(0, start)

keyboard.add_hotkey("ctrl+r", toggle)

root = tk.Tk()
root.title("Capture")
root.attributes("-topmost", True)
root.geometry("250x100")

btn_start = tk.Button(root, text="Start", command=start, width=10)
btn_start.pack(pady=5)
btn_stop = tk.Button(root, text="Stop", command=stop, width=10, state="disabled")
btn_stop.pack()
label = tk.Label(root, text="Ready — Ctrl+R to toggle")
label.pack(pady=5)

root.mainloop()
