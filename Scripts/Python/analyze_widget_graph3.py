import json

with open(r"C:\Users\Ommei\.claude\projects\C--Users-Ommei-workspace-TheSignal\8c7d7d48-3075-43f3-b784-dc1b1a77b926\tool-results\mcp-blueprint-mcp-get_blueprint_graph-1772841627577.txt", "r") as f:
    raw = f.read()

data = json.loads(raw)
text = data[0]['text']
graph = json.loads(text)
nodes = graph['nodes']
node_map = {n['id']: n for n in nodes}

# Trace the ShowContinueButtonWithText event chain
print("=== ShowContinueButtonWithText event chain ===")
event_id = '2BF21D064809C7C008EE5D82265CE8F4'
visited = set()
current = event_id
while current and current not in visited:
    visited.add(current)
    node = node_map.get(current)
    if not node:
        break
    title = node.get('title','').replace('\n',' ')[:60]
    print(f"  {title} ({current})")
    # Find 'then' or output exec pin
    next_id = None
    for p in node.get('pins', []):
        if p.get('direction') == 'Output' and p.get('type') == 'exec':
            for c in p.get('connections', []):
                next_id = c['nodeId']
                print(f"    -> {c['pinName']} on {c['nodeId'][:12]}...")
    current = next_id

# Find SimulateNextClicked custom event
print("\n=== SimulateNextClicked / SimulateNextHovered events ===")
for n in nodes:
    title = n.get('title','').replace('\n',' ')
    nid = n.get('id','')
    if 'Simulate' in title:
        print(f"  {title} ({n.get('class','')}) id={nid}")
        for p in n.get('pins',[]):
            conns = p.get('connections',[])
            if conns:
                for c in conns:
                    src = node_map.get(c['nodeId'],{})
                    st = src.get('title','').replace('\n',' ')[:40]
                    print(f"    {p['name']} ({p['direction']}): {st} ({c['nodeId'][:12]}..:{c['pinName']})")

# Find ClearChildren node in PlayPlayerReply chain
print("\n=== PlayPlayerReply chain around ShowContinue (4A0F) ===")
sc_id = '4A0F5CD44E499EEE69B91E9CE49BC504'
sc_node = node_map.get(sc_id)
if sc_node:
    for p in sc_node.get('pins',[]):
        conns = p.get('connections',[])
        if conns:
            for c in conns:
                src = node_map.get(c['nodeId'],{})
                st = src.get('title','').replace('\n',' ')[:50]
                print(f"  {p['name']} ({p['direction']}): {st} id={c['nodeId']} pin={c['pinName']}")
