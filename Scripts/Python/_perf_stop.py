"""Stop CSV profiling and report output location."""
import unreal
import os

unreal.SystemLibrary.execute_console_command(None, "csvprofile stop")

csv_dir = os.path.join(unreal.Paths.project_saved_dir(), 'Profiling', 'CSV')
print(f"CSV profiling STOPPED")
print(f"CSV output directory: {csv_dir}")

# List any CSV files
if os.path.exists(csv_dir):
    files = sorted(os.listdir(csv_dir), key=lambda f: os.path.getmtime(os.path.join(csv_dir, f)), reverse=True)
    print(f"Found {len(files)} CSV files:")
    for f in files[:5]:
        full = os.path.join(csv_dir, f)
        size = os.path.getsize(full) / 1024
        print(f"  {f} ({size:.1f} KB)")
else:
    print("CSV directory not found yet - profiling may not have written data")
    # Try alternate stat dump
    unreal.SystemLibrary.execute_console_command(None, "stat stopfile")
