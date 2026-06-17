"""
trigger_build.py

Fires off Scripts/build_signal.bat as a fully detached process so it survives
the editor being killed. Used by Claude in Nwiro, where direct shell execution
is disabled — Claude calls this via the in-editor MCP Python endpoint:

    exec(open(r'C:\\Users\\Ommei\\workspace\\TheSignal\\Scripts\\Python\\trigger_build.py').read())

Flow:
  1. os.startfile spawns cmd /c on the bat via Windows ShellExecute (fully
     independent of the editor's process tree).
  2. Python returns immediately so the MCP response flushes.
  3. Bat sleeps ~3s, then kills the editor, runs Build.bat, relaunches editor.

Verify after rebuild (next Claude turn, once editor is back):
  - C:\\Users\\Ommei\\workspace\\TheSignal\\Scripts\\last_build.status -> "OK" or "FAIL N"
  - C:\\Users\\Ommei\\workspace\\TheSignal\\Scripts\\last_build.log       -> full Build.bat output
  - DLL mtime via os.path.getmtime on Binaries/Win64/UnrealEditor-TheSignal.dll
  - unreal.load_class(None, '/Script/TheSignal.<NewClassName>')
"""

import os

BAT = r'C:\Users\Ommei\workspace\TheSignal\Scripts\build_signal.bat'

if not os.path.isfile(BAT):
    raise FileNotFoundError(f'build bat not found: {BAT}')

# ShellExecute via os.startfile — the spawned cmd.exe has no parent handle to
# the editor, so killing the editor mid-build doesn't take the build with it.
os.startfile(BAT)

print('[trigger_build] spawned build_signal.bat (detached).')
print('[trigger_build] editor will be killed in ~3s; rebuild then relaunch.')
print('[trigger_build] log:    C:\\Users\\Ommei\\workspace\\TheSignal\\Scripts\\last_build.log')
print('[trigger_build] status: C:\\Users\\Ommei\\workspace\\TheSignal\\Scripts\\last_build.status')
