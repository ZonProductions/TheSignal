import unreal

sys = unreal.load_asset("/Game/HorrorLight/NS_DustMote")
print("system:", type(sys).__name__)

# What Niagara editing API exists?
ns_methods = [m for m in dir(sys) if not m.startswith("__")]
print("NiagaraSystem methods/props:", [m for m in ns_methods if any(
    k in m.lower() for k in ("emitter", "renderer", "exposed", "parameter", "variable"))])

# Try to enumerate emitters + renderers (best-effort; Niagara py is limited)
for getter in ["get_emitter_handles", "emitter_handles"]:
    try:
        h = getattr(sys, getter)() if callable(getattr(sys, getter, None)) else sys.get_editor_property(getter)
        print(getter, "->", h)
    except Exception as e:
        print(getter, "ERR", str(e)[:60])

# Niagara-related global classes available for authoring
print("Has NiagaraEditor classes:",
      [c for c in dir(unreal) if "Niagara" in c and ("Renderer" in c or "Factory" in c or "Script" in c)][:20])

# Default cube + a candidate emissive material source
print("Cube exists:", unreal.EditorAssetLibrary.does_asset_exist("/Engine/BasicShapes/Cube"))
