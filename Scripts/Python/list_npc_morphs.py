import unreal

mesh = unreal.load_asset('/Game/CharacterCustomizer/CharacterCustomizer_Core/Pawns/CC_Dummy/CC_Dummy_AnimMesh')
morphs = mesh.get_editor_property('morph_targets')
unreal.log(f'Total morph targets: {len(morphs)}')
for m in sorted(morphs, key=lambda x: str(x.get_name())):
    unreal.log(f'  {m.get_name()}')
