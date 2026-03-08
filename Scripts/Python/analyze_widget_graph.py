import json

with open(r"C:\Users\Ommei\.claude\projects\C--Users-Ommei-workspace-TheSignal\8c7d7d48-3075-43f3-b784-dc1b1a77b926\tool-results\mcp-blueprint-mcp-get_blueprint_graph-1772841627577.txt", "r") as f:
    raw = f.read()

data = json.loads(raw)
text = data[0]['text']
graph = json.loads(text)
nodes = graph['nodes']

# Build node lookup
node_map = {n['id']: n for n in nodes}

# Find all ShowContinueButtonWithText call nodes
print("=== ShowContinueButtonWithText CALL nodes ===")
for n in nodes:
    title = n.get('title','')
    cls = n.get('class','')
    nid = n.get('id','')
    if 'ShowContinue' in title and 'CallFunction' in cls:
        print(f'\nNODE: {title}')
        print(f'  ID: {nid}')
        for p in n.get('pins',[]):
            conns = p.get('connections',[])
            if conns or p.get('name') in ['execute','then','self','ContinueText']:
                for c in conns:
                    src_node = node_map.get(c['nodeId'], {})
                    src_title = src_node.get('title', '?')
                    print(f"  PIN {p['name']} ({p['direction']}) -> {src_title[:50]} ({c['nodeId'][:12]}..:{c['pinName']})")
                if not conns:
                    print(f"  PIN {p['name']} ({p['direction']}) default='{p.get('defaultValue','')}'")

# Find the ToNpcReply custom event
print("\n=== ToNpcReply and related events ===")
for n in nodes:
    title = n.get('title','')
    cls = n.get('class','')
    nid = n.get('id','')
    if any(kw in title for kw in ['ToNpcReply', 'DisplayNpcNode', 'RemoveFromParent', 'GoToNpcReply']):
        print(f'\nNODE: {title} ({cls})')
        print(f'  ID: {nid}')
        for p in n.get('pins',[]):
            conns = p.get('connections',[])
            if conns:
                for c in conns:
                    src_node = node_map.get(c['nodeId'], {})
                    src_title = src_node.get('title', '?')
                    print(f"  PIN {p['name']} ({p['direction']}) -> {src_title[:50]} ({c['nodeId'][:12]}..:{c['pinName']})")
