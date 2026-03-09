"""Force disable Lumen GI and reflections via console commands during PIE."""
import unreal

# 0 = None, 1 = Lumen, 2 = SSGI
unreal.SystemLibrary.execute_console_command(None, "r.DynamicGlobalIlluminationMethod 0")
# 0 = None, 1 = Lumen, 2 = SSR
unreal.SystemLibrary.execute_console_command(None, "r.ReflectionMethod 2")
# Disable Lumen screen traces
unreal.SystemLibrary.execute_console_command(None, "r.Lumen.ScreenProbeGather 0")
unreal.SystemLibrary.execute_console_command(None, "r.Lumen.Reflections 0")

print("Forced: Lumen GI OFF, Lumen Reflections OFF, SSR ON")
