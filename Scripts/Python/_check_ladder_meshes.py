import unreal

# Check bounds and pivot for each ladder mesh piece across all 5 styles
styles = {
    1: {'FootBar': 'SM_Ladder1_FootBar', 'BotL': 'SM_Ladder1_BottomLeft', 'BotR': 'SM_Ladder1_BottomRight',
        'MidL': 'SM_Ladder1_MidLeft_WithoutPanel', 'MidR': 'SM_Ladder1_MidRight_WithoutPanel'},
    2: {'FootBar': 'SM_Ladder2_Footbar', 'BotL': 'SM_Ladder2_BottomLeft', 'BotR': 'SM_Ladder2_BottomRIght',
        'MidL': 'SM_Ladder2_LeftMidWithoutPanel', 'MidR': 'SM_Ladder2_RightMidWithoutPanel',
        'TopL': 'SM_Ladder2_TopLeft', 'TopR': 'SM_Ladder2_TopRight'},
    3: {'FootBar': 'SM_Ladder3_FootBar', 'BotL': 'SM_Ladder3_BottomLeft', 'BotR': 'SM_Ladder3_BottomRight',
        'MidL': 'SM_Ladder3_MidLeft_WithoutPanel', 'MidR': 'SM_Ladder3_MidRight_WithoutPanel'},
    4: {'FootBar': 'SM_Ladder4_Footbar', 'BotL': 'SM_Ladder4_BottomLeft', 'BotR': 'SM_Ladder4_BottomRight',
        'MidL': 'SM_Ladder4_MidLeft_WithoutPanel', 'MidR': 'SM_Ladder4_MidRight_WithoutPanel',
        'TopL': 'SM_Ladder4_TopLeft', 'TopR': 'SM_Ladder4_TopRight'},
    5: {'FootBar': 'SM_Ladder5_FootBar', 'BotL': 'SM_Ladder5_BottomLeft', 'BotR': 'SM_Ladder5_BottomRight',
        'MidL': 'SM_Ladder5_MidLeft_WithoutPanel', 'MidR': 'SM_Ladder5_MidRight_WithoutPanel',
        'TopL': 'SM_Ladder5_TopL', 'TopR': 'SM_Ladder5_TopR'},
}

# Also check the prototype
proto_path = '/Game/LadderClimbingSystem/Meshes/Ladders/PrototypeLadder/SM_Ladder'
proto = unreal.load_asset(proto_path)
if proto:
    bb = proto.get_bounding_box()
    print(f'Prototype SM_Ladder: min=({bb.min.x:.1f},{bb.min.y:.1f},{bb.min.z:.1f}) max=({bb.max.x:.1f},{bb.max.y:.1f},{bb.max.z:.1f})')
    ext = (bb.max - bb.min)
    print(f'  Extent: ({ext.x:.1f},{ext.y:.1f},{ext.z:.1f})')

for style_n, pieces in styles.items():
    print(f'\n=== Style {style_n} ===')
    base = f'/Game/LadderClimbingSystem/Meshes/Ladders/Ladder{style_n}'
    for piece_name, asset_name in pieces.items():
        path = f'{base}/{asset_name}'
        mesh = unreal.load_asset(path)
        if mesh:
            bb = mesh.get_bounding_box()
            ext = (bb.max - bb.min)
            center = (bb.max + bb.min) * 0.5
            print(f'  {piece_name}: min=({bb.min.x:.1f},{bb.min.y:.1f},{bb.min.z:.1f}) max=({bb.max.x:.1f},{bb.max.y:.1f},{bb.max.z:.1f}) size=({ext.x:.1f},{ext.y:.1f},{ext.z:.1f}) center=({center.x:.1f},{center.y:.1f},{center.z:.1f})')
        else:
            print(f'  {piece_name}: NOT FOUND at {path}')
