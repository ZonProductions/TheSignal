import unreal

# Get the placed BP_NPC instance
actors = unreal.EditorLevelLibrary.get_all_level_actors()
npc = None
for a in actors:
    if 'BP_NPC' in a.get_class().get_name():
        npc = a
        break

if not npc:
    print('No BP_NPC found in level')
else:
    print(f'Actor: {npc.get_name()} | Class: {npc.get_class().get_name()}')
    print(f'Root component: {npc.root_component}')
    if npc.root_component:
        print(f'  Root class: {npc.root_component.get_class().get_name()}')

    # List ALL components
    print('\n--- ALL Components ---')
    all_comps = npc.get_components_by_class(unreal.ActorComponent)
    for c in all_comps:
        class_name = c.get_class().get_name()
        comp_name = c.get_name()
        print(f'  {comp_name} ({class_name})')

    # Check class hierarchy
    print('\n--- Class Hierarchy ---')
    cls = npc.get_class()
    while cls:
        print(f'  {cls.get_name()}')
        cls = cls.get_editor_property('super_struct') if hasattr(cls, 'get_editor_property') else None
        try:
            cls = cls if cls else None
        except:
            break
