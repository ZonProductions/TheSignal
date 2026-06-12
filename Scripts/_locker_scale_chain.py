import unreal

es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = es.get_game_world()
assert w, 'NOT IN PIE'
p = unreal.GameplayStatics.get_player_character(w, 0)
ploc = p.get_actor_location()

best = None
best_d = 1e9
for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Actor):
    if 'LootLocker' in a.get_class().get_name():
        d = (a.get_actor_location() - ploc).length()
        if d < best_d:
            best, best_d = a, d

for c in best.get_components_by_class(unreal.SphereComponent):
    if c.get_name() == 'InteractionArea':
        node = c
        while node:
            rel = node.get_relative_transform().scale3d
            unreal.log(f'chain: {node.get_name()} relScale=({rel.x:.2f}, {rel.y:.2f}, {rel.z:.2f})')
            node = node.get_attach_parent()
