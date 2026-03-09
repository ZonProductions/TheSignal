"""Capture real-time performance stats during PIE and dump to file."""
import unreal
import os

# Start CSV profiling - captures all stat data to a CSV file
# Output goes to Saved/Profiling/CSV/
unreal.SystemLibrary.execute_console_command(None, "csvprofile start")
print("CSV profiling STARTED")
print("Walk around in PIE for 30-60 seconds to capture data")
print("Then run _perf_stop.py to stop and I'll analyze the results")
print(f"Output will be in: {os.path.join(unreal.Paths.project_saved_dir(), 'Profiling', 'CSV')}")
