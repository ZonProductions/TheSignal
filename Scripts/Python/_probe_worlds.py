"""Probe what world handles Python has access to during PIE."""
import unreal

ews = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
print(f"editor world: {ews.get_editor_world()}")

# Try get_game_world if it exists
try:
    gw = ews.get_game_world()
    print(f"get_game_world() -> {gw}")
    if gw:
        # Try to enumerate pawns there
        pawns = unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.Pawn)
        print(f"  pawns in game world: {len(pawns)}")
        for p in pawns[:5]:
            print(f"    {p.get_actor_label()} [{type(p).__name__}] @ {p.get_actor_location()}")
except Exception as e:
    print(f"get_game_world failed: {e}")

# Also try iter_all_subobjects-like approach
print("\nKnown subsystem methods on UnrealEditorSubsystem:")
for n in dir(ews):
    if 'world' in n.lower() or 'pie' in n.lower() or 'game' in n.lower():
        print(f"  {n}")
