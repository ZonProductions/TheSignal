import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = eas.get_all_level_actors()
ctx = actors[0] if actors else None

found = 0
for a in actors:
    lbl = a.get_actor_label()
    if ("Void" in lbl) or ("Mote" in lbl):
        found += 1
        tags = [str(t) for t in a.tags]
        unreal.log_warning("VOID_ACTOR %s | class=%s | tags=%s | loc=%s" % (
            lbl, a.get_class().get_name(), tags, a.get_actor_location()))
unreal.log_warning("void-ish actors found=%d" % found)

if ctx:
    vt = unreal.GameplayStatics.get_all_actors_with_tag(ctx, "Voidmote")
    et = unreal.GameplayStatics.get_all_actors_with_tag(ctx, "Enemy")
    unreal.log_warning("TAGGED Voidmote=%d Enemy=%d" % (len(vt), len(et)))
    for a in vt:
        unreal.log_warning("  Voidmote-tagged: %s loc=%s" % (a.get_actor_label(), a.get_actor_location()))
