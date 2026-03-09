"""Enable performance stat overlays during PIE."""
import unreal

# These execute in the PIE viewport
unreal.SystemLibrary.execute_console_command(None, "stat fps")
unreal.SystemLibrary.execute_console_command(None, "stat unit")
unreal.SystemLibrary.execute_console_command(None, "stat gpu")
print("Enabled stat fps, stat unit, stat gpu overlays")
print("Look at the overlay in PIE:")
print("  Frame = total frame time")
print("  Game  = game thread (logic, ticking, physics)")
print("  Draw  = draw thread (preparing render commands)")
print("  GPU   = GPU render time (shaders, triangles, lighting)")
print("  RHI   = render hardware interface")
print("")
print("The LARGEST number is your bottleneck:")
print("  GPU high   -> rendering bound (too many triangles, lights, Lumen, shadows)")
print("  Game high  -> CPU bound (too many actors ticking, physics, AI)")
print("  Draw high  -> too many draw calls (too many individual actors)")
