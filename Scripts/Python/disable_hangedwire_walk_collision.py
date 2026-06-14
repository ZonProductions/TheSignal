"""
Make every SM_HangedWire in the open level non-blocking to walking pawns.

Sets each SM_HangedWire StaticMeshComponent's response to the Pawn channel to
Ignore (player/NPC capsules pass through) while leaving all other collision
intact (visibility traces, bullets, etc.). Per-actor change, scoped to the level
— does NOT touch the shared mesh asset, so SM_HangedWire elsewhere is unaffected.

Uses EditorLevelLibrary.get_all_level_actors() so sublevel actors are included.
Run via the MCP Python endpoint (editor must NOT be in PIE — saves are blocked there).
"""
import unreal

PAWN = unreal.CollisionChannel.ECC_PAWN
IGNORE = unreal.CollisionResponseType.ECR_IGNORE

actors = unreal.EditorLevelLibrary.get_all_level_actors()

def is_hanged_wire(a):
    if not isinstance(a, unreal.StaticMeshActor):
        return False
    if "HangedWire" in a.get_actor_label():
        return True
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        m = c.static_mesh
        if m and "HangedWire" in m.get_name():
            return True
    return False

targets = [a for a in actors if is_hanged_wire(a)]
unreal.log("[HangedWire] Found %d SM_HangedWire actor(s)" % len(targets))

changed = 0
for a in targets:
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        try:
            before = c.get_collision_response_to_channel(PAWN)
        except Exception:
            before = "?"
        c.set_collision_response_to_channel(PAWN, IGNORE)
        try:
            after = c.get_collision_response_to_channel(PAWN)
        except Exception:
            after = "?"
        unreal.log("  %s :: %s  Pawn %s -> %s"
                   % (a.get_actor_label(), c.get_name(),
                      str(before).split(".")[-1], str(after).split(".")[-1]))
        changed += 1

unreal.log("[HangedWire] Set Pawn=Ignore on %d component(s)" % changed)

# Save the level(s) holding these actors.
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.save_all_dirty_levels()
unreal.log("[HangedWire] Saved dirty levels. DONE.")
