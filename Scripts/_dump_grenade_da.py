import unreal

da = unreal.load_asset('/Game/Core/Items/DA_ExplosiveGrenade')
assert da, 'DA not found'
cls = da.get_class()
unreal.log(f'=== {da.get_name()} class {cls.get_name()} — all editor properties:')
# Walk the UClass property chain via reflection
prop_names = []
struct = cls
field = unreal.SystemLibrary  # placeholder
# Python-exposed attribute names (pythonized BP properties)
for name in dir(da):
    if name.startswith('_'):
        continue
    try:
        val = da.get_editor_property(name)
        unreal.log(f'  {name} = {val}')
        prop_names.append(name)
    except Exception:
        pass
unreal.log(f'({len(prop_names)} readable properties)')
