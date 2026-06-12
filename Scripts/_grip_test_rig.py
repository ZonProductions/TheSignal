"""Spawn a posed grip-test rig in the EDITOR world (no PIE needed).
Operator mesh + melee idle pose + pipe attached with current grip values.
Actors labeled TEST_GripRig* — deleted by _grip_test_rig_cleanup.py. NEVER save the map."""
import unreal

RIG_LOC = unreal.Vector(-1492.0, -910.0, 3000.0)  # high above the map

sk = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono')
idle = unreal.load_asset('/Game/TheSignal/Animations/Melee/A_MeleePipe_Idle')
pipe_sm = unreal.load_asset('/Game/InventorySystemPro/ExampleContent/Common/Art/Pipe/SM_Pipe')
assert sk and idle and pipe_sm

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Clean previous rig if re-running
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith('TEST_GripRig'):
        eas.destroy_actor(a)

actor = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, RIG_LOC, unreal.Rotator(roll=0, pitch=0, yaw=0))
actor.set_actor_label('TEST_GripRig_Body')
comp = actor.skeletal_mesh_component
comp.set_skeletal_mesh_asset(sk)
comp.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
comp.set_animation(idle)
comp.play(False)
comp.set_position(0.5, False)

pipe_actor = eas.spawn_actor_from_class(unreal.StaticMeshActor, RIG_LOC, unreal.Rotator(roll=0, pitch=0, yaw=0))
pipe_actor.set_actor_label('TEST_GripRig_Pipe')
pc = pipe_actor.static_mesh_component
pc.set_mobility(unreal.ComponentMobility.MOVABLE)
pc.set_static_mesh(pipe_sm)
pipe_actor.attach_to_component(comp, 'hand_r',
    unreal.AttachmentRule.SNAP_TO_TARGET, unreal.AttachmentRule.SNAP_TO_TARGET,
    unreal.AttachmentRule.SNAP_TO_TARGET, False)

# Current grip values (from the CDO fit)
GRIP_LOC = unreal.Vector(-18.69, -19.34, 22.94)
GRIP_ROT = unreal.Rotator(roll=-49.54, pitch=0.00, yaw=-44.03)
pipe_actor.set_actor_relative_location(GRIP_LOC, False, False)
pipe_actor.set_actor_relative_rotation(GRIP_ROT, False, False)

# Camera: front-right 3/4 view at chest height, 220cm out
hr = comp.get_socket_transform('hand_r', unreal.RelativeTransformSpace.RTS_WORLD)
target = hr.translation
cam_loc = unreal.Vector(target.x + 160.0, target.y + 160.0, target.z + 20.0)
look = unreal.MathLibrary.find_look_at_rotation(cam_loc, target)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
ues.set_level_viewport_camera_info(cam_loc, look)
unreal.SystemLibrary.execute_console_command(None, 'viewmode unlit')
unreal.log(f'RIG READY at {RIG_LOC}. hand_r at {target}. Camera set, unlit mode on.')
