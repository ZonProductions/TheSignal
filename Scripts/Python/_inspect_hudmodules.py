import unreal
# re-bake first
exec(open(r'C:/Users/Ommei/workspace/TheSignal/Scripts/Python/bake_objectives.py').read())
print("----")
da = unreal.load_asset("/Game/EasyGameUI/EasyHudBuilder/Core/HudModulesDefinition/DA_HudModulesDefinition")
print("DA class:", da.get_class().get_name())
# dump top-level editor properties
for p in ["HudModulesList","ModulesList","HudModules","Modules","ModulesDefinition","ContextsDefinition","HudContexts"]:
    try:
        v = da.get_editor_property(p)
        print("PROP", p, "=", str(v)[:1500])
    except Exception as e:
        pass
