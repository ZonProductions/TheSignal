import unreal
asset = unreal.load_asset('/Game/Dialogue/Dialogue_Test')
if asset:
    print(f"Class: {asset.get_class().get_name()}")
    # Check if Data array is accessible
    try:
        data = asset.get_editor_property('data')
        print(f"Data array length: {len(data)}")
        for i, node in enumerate(data):
            nid = node.get_editor_property('id')
            text = node.get_editor_property('text')
            is_player = node.get_editor_property('is_player')
            links = node.get_editor_property('links')
            print(f"  Node {i}: id={nid}, player={is_player}, text='{text}', links={list(links)}")
    except Exception as e:
        print(f"Error accessing Data: {e}")
    # Check other top-level properties
    try:
        name = asset.get_editor_property('name')
        print(f"Name: {name}")
    except:
        pass
    try:
        nid = asset.get_editor_property('next_node_id')
        print(f"NextNodeId: {nid}")
    except:
        pass
