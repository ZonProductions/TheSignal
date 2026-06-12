"""Phase B (simulate world): find the test pawn, make visible, equip pipe, set camera."""
import unreal
es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = es.get_game_world()
assert world, 'simulate not running'
pawns = unreal.GameplayStatics.get_all_actors_of_class(world,
    unreal.load_asset('/Game/Core/Player/BP_GraceCharacter').generated_class())
assert pawns, 'no Grace pawn in simulate world'
pawn = pawns[0]
unreal.log(f'FOUND {pawn.get_name()} in {world.get_name()}')

for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent):
    c.set_only_owner_see(False)

kin = next(c for c in pawn.get_components_by_class(unreal.ActorComponent) if 'Kinemation' in c.get_name())
pipe_bp = unreal.load_asset('/Game/InventorySystemPro/ExampleContent/Common/Equipment/Weapons/BP_Pipe')
ok = kin.equip_weapon_class(pipe_bp.generated_class())
unreal.log(f'EQUIP: {ok}')

loc = pawn.get_actor_location()
cam = unreal.Vector(loc.x + 80.0, loc.y + 120.0, loc.z + 40.0)
target = unreal.Vector(loc.x, loc.y, loc.z + 25.0)
look = unreal.MathLibrary.find_look_at_rotation(cam, target)
es.set_level_viewport_camera_info(cam, look)
unreal.SystemLibrary.execute_console_command(None, 'Slate.bAllowThrottling 0')
unreal.log('READY FOR CAPTURE')
