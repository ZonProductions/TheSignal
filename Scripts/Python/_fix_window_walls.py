"""Check SM_WindowWall material slots and swap glass material."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# First check one window wall to see slot layout
for a in eas.get_all_level_actors():
    if a.get_actor_label() == 'F4_SM_WindowWall10':
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        print(f"=== F4_SM_WindowWall10 materials ===")
        for i in range(smc.get_num_materials()):
            mat = smc.get_material(i)
            print(f"  Slot {i}: {mat.get_name() if mat else 'None'}")
        break

# Count all WindowWall actors
count = 0
for a in eas.get_all_level_actors():
    label = a.get_actor_label()
    if 'WindowWall' in label:
        count += 1
print(f"\nTotal WindowWall actors: {count}")
