import unreal
# What Niagara Python API is available?
ndir = [a for a in dir(unreal) if 'Niagara' in a]
print("Niagara classes:", ndir)
nel = [m for m in dir(unreal.NiagaraEditorLibrary)] if hasattr(unreal,'NiagaraEditorLibrary') else "NO NiagaraEditorLibrary"
print("NiagaraEditorLibrary:", nel)
# Search asset registry for existing dust/particle Niagara systems
ar = unreal.AssetRegistryHelpers.get_asset_registry()
assets = ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Niagara","NiagaraSystem"), True)
hits=[]
for a in assets:
    n = str(a.asset_name).lower()
    if any(k in n for k in ["dust","mote","fog","ambient","float","particle","smoke"]):
        hits.append(str(a.package_name))
print("candidate Niagara systems (", len(assets), "total ):")
for h in hits[:40]:
    print("  ", h)
