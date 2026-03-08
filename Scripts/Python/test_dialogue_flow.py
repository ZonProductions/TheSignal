import unreal

d = unreal.load_asset('/Game/Dialogue/Dialogue_Test')
data = d.get_editor_property('data')

unreal.log(f'Dialogue has {len(data)} nodes')

# Simulate the flow: start with data[0], call GetNextNodes
node0 = data[0]
unreal.log(f'Node 0: id={node0.get_editor_property("id")}, text="{node0.get_editor_property("text")}", links={list(node0.get_editor_property("links"))}')

# GetNextNodes for node 0
next0 = d.get_next_nodes(node0) if hasattr(d, 'get_next_nodes') else None
if next0 is not None:
    unreal.log(f'GetNextNodes(node0) returned {len(next0)} nodes')
    for n in next0:
        unreal.log(f'  id={n.get_editor_property("id")}, player={n.get_editor_property("is_player")}, text="{str(n.get_editor_property("text"))[:40]}"')
else:
    unreal.log('get_next_nodes not available via Python, checking manually')
    # Manual check: follow links
    links0 = list(node0.get_editor_property('links'))
    unreal.log(f'Node 0 links: {links0}')
    for link_id in links0:
        for nd in data:
            if nd.get_editor_property('id') == link_id:
                unreal.log(f'  -> Node {link_id}: player={nd.get_editor_property("is_player")}, text="{str(nd.get_editor_property("text"))[:40]}"')

# Now check node 1 (first NPC line)
node1 = data[1]
links1 = list(node1.get_editor_property('links'))
unreal.log(f'\nNode 1: id={node1.get_editor_property("id")}, links={links1}')
for link_id in links1:
    for nd in data:
        if nd.get_editor_property('id') == link_id:
            unreal.log(f'  -> Node {link_id}: player={nd.get_editor_property("is_player")}, text="{str(nd.get_editor_property("text"))[:40]}"')

# Check if GetNextNodes is a Blueprint-callable function
try:
    result = d.get_next_nodes(node1)
    unreal.log(f'\nGetNextNodes(node1) = {len(result)} nodes')
    for n in result:
        unreal.log(f'  id={n.get_editor_property("id")}, player={n.get_editor_property("is_player")}')
except Exception as e:
    unreal.log(f'\nGetNextNodes failed: {e}')

# Also check the next_node_id (auto-set field)
for nd in data:
    nid = nd.get_editor_property('id')
    try:
        nnid = nd.get_editor_property('next_node_id')
        unreal.log(f'  Node {nid}: next_node_id={nnid}')
    except:
        pass
