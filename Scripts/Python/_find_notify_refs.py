import unreal

pkg = "/Game/Audio/Signal/SFX_Phone_Notify"
ar = unreal.AssetRegistryHelpers.get_asset_registry()

opts = unreal.AssetRegistryDependencyOptions()
opts.include_hard_package_references = True
opts.include_soft_package_references = True

refs = ar.get_referencers(pkg, opts)
refs = refs or []
unreal.log_warning("NOTIFY_REFERENCERS count=%d" % len(refs))
for r in refs:
    unreal.log_warning("  ref: %s" % str(r))

# Also scan the current level for AmbientSound / AudioComponent actors pointing at it,
# and list every sound asset in the Signal folder so we can see what exists.
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    for comp in a.get_components_by_class(unreal.AudioComponent):
        s = comp.get_editor_property("sound")
        if s:
            unreal.log_warning("LEVEL_AUDIO actor=%s sound=%s" % (a.get_actor_label(), s.get_name()))
