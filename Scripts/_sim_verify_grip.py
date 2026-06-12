"""Self-verification rig: simulate mode, spawn Grace, equip pipe.
Run AFTER les.editor_play_simulate(). View-model visibility forced on
(OnlyOwnerSee doesn't render without a possessing player in simulate)."""
import unreal

es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = es.get_game_world()
assert world, 'simulate not running'

bp = unreal.load_asset('/Game/Core/Player/BP_GraceCharacter')
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
pawn = eas.spawn_actor_from_class(bp.generated_class(),
    unreal.Vector(-1450.0, -950.0, 1090.0), unreal.Rotator(roll=0, pitch=0, yaw=0))
assert pawn, 'spawn failed'
unreal.log(f'SPAWNED: {pawn.get_name()} in world {pawn.get_world().get_name()}')

# Make view model + arms visible without an owning player
for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent):
    c.set_only_owner_see(False)

kin = next(c for c in pawn.get_components_by_class(unreal.ActorComponent) if 'Kinemation' in c.get_name())
pipe_bp = unreal.load_asset('/Game/InventorySystemPro/ExampleContent/Common/Equipment/Weapons/BP_Pipe')
ok = kin.equip_weapon_class(pipe_bp.generated_class())
unreal.log(f'EQUIP: {ok}')

# Camera: right side of the pawn, looking at chest/hands height
loc = pawn.get_actor_location()
cam = unreal.Vector(loc.x + 90.0, loc.y + 130.0, loc.z + 35.0)
target = unreal.Vector(loc.x, loc.y, loc.z + 30.0)
look = unreal.MathLibrary.find_look_at_rotation(cam, target)
es.set_level_viewport_camera_info(cam, look)
unreal.SystemLibrary.execute_console_command(None, 'Slate.bAllowThrottling 0')
unreal.log('CAMERA SET')
