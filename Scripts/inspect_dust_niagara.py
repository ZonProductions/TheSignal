import unreal
sys = unreal.load_asset("/Game/HorrorLight/NS_DustMote")
print("system:", sys.get_name() if sys else None, type(sys).__name__ if sys else "")
# emitter handles
try:
    handles = sys.get_editor_property("emitter_handles")
except Exception as e:
    handles = None
    print("emitter_handles err:", e)
# Fallback API
funcs = [m for m in dir(sys) if 'emitter' in m.lower()]
print("emitter-related methods:", funcs)
