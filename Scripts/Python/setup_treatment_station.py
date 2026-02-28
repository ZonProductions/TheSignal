"""
Setup TreatmentStationDemo map for The Signal prototype.
Adds a PlayerStart and configures World Settings.
Run via BlueprintMCP /api/python endpoint.
"""
import unreal

# Add a PlayerStart actor at a reasonable spawn point
# Position it slightly above ground to avoid clipping
editor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
player_start = editor_subsystem.spawn_actor_from_class(
    unreal.PlayerStart,
    unreal.Vector(0, 0, 200),
    unreal.Rotator(0, 0, 0)
)
if player_start:
    player_start.set_actor_label('PlayerStart_Grace')
    print(f'PlayerStart added at (0, 0, 200)')
else:
    print('ERROR: Failed to spawn PlayerStart')
