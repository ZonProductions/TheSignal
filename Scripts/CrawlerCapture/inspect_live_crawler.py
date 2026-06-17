# Run IN PIE. Inspects the crawler nearest the player: full component tree of
# root + attached actors, so we know what the BODY component is before baking.
import unreal
out=[]
ues=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gw=ues.get_game_world()
if gw is None:
    out.append("ERROR: not in PIE. Press Play first.")
else:
    pawn=unreal.GameplayStatics.get_player_pawn(gw,0)
    ploc=pawn.get_actor_location() if pawn else unreal.Vector(0,0,0)
    roots=[a for a in unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.Actor) if a.get_class().get_name()=='BP_PatrolCreature_C']
    # nearest crawler to player
    roots.sort(key=lambda a: (a.get_actor_location()-ploc).length())
    out.append("crawlers=%d; player=(%.0f,%.0f,%.0f)"%(len(roots),ploc.x,ploc.y,ploc.z))
    for r in roots[:3]:
        d=(r.get_actor_location()-ploc).length()
        group=[r]+list(r.get_attached_actors(True,True))
        out.append("\n== %s  dist=%.0f  actors=%d =="%(r.get_actor_label(), d, len(group)))
        for x in group:
            prims=x.get_components_by_class(unreal.PrimitiveComponent)
            for c in prims:
                cls=c.get_class().get_name()
                mesh='-'
                if hasattr(c,'get_skeletal_mesh_asset'):
                    m=c.get_skeletal_mesh_asset();  mesh=m.get_name() if m else 'NONE'
                elif hasattr(c,'get_static_mesh'):
                    m=c.get_static_mesh(); mesh=m.get_name() if m else 'NONE'
                try: nv=c.get_num_vertices() if hasattr(c,'get_num_vertices') else -1
                except: nv=-1
                b=c.bounds.box_extent if hasattr(c,'bounds') else None
                out.append("   %-28s mesh=%-16s vis=%s"%(cls, mesh, c.is_visible()))
open(r"C:/Users/Ommei/.claude/jobs/db244521/tmp/_fab_result.txt","w").write("\n".join(out))
print("\n".join(out))
