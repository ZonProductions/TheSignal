"""Minimal PIE driver for The Signal — adapted from Reliquary's pie_test.py.
Usage: python Scripts/Python/pie_drive.py start|stop
Focuses the UE window and sends Alt+P (start PIE) or Esc (stop PIE).
"""
import sys
import time
import win32gui
import win32con
import keyboard


def find_ue_window():
    result = []

    def cb(hwnd, _):
        if win32gui.IsWindowVisible(hwnd):
            t = win32gui.GetWindowText(hwnd)
            if 'Unreal Editor' in t and 'TheSignal' in t:
                result.append(hwnd)
    win32gui.EnumWindows(cb, None)
    return result[0] if result else None


def focus(hwnd):
    win32gui.ShowWindow(hwnd, win32con.SW_RESTORE)
    win32gui.SetForegroundWindow(hwnd)
    time.sleep(0.8)


def main():
    action = sys.argv[1] if len(sys.argv) > 1 else 'start'
    hwnd = find_ue_window()
    if not hwnd:
        print('UE WINDOW NOT FOUND')
        sys.exit(1)
    focus(hwnd)
    if action == 'start':
        keyboard.press_and_release('alt+p')
        print('PIE START SENT')
    elif action == 'stop':
        keyboard.press_and_release('esc')
        print('PIE STOP SENT')


if __name__ == '__main__':
    main()
