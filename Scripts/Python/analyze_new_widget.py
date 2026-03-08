import json, subprocess

# Get the new widget's graph via MCP
result = subprocess.run(
    ['curl', '-s', 'http://localhost:9847/api/blueprint/graph',
     '-H', 'Content-Type: application/json',
     '-d', json.dumps({"name": "/Game/Dialogue/WBP_DialogueWidget", "graph": "EventGraph"})],
    capture_output=True, text=True, timeout=30
)

data = json.loads(result.stdout)
# MCP returns {content: [{type, text}]}
if 'content' in data:
    text = data['content'][0]['text']
elif isinstance(data, list):
    text = data[0]['text']
else:
    text = str(data)

graph = json.loads(text)
nodes = graph['nodes']
node_map = {n['id']: n for n in nodes}

print(f"Total nodes: {len(nodes)}")

# Find key nodes
print("\n=== Key nodes ===")
for n in nodes:
    title = n.get('title','').replace('\n',' ')
    cls = n.get('class','')
    nid = n.get('id','')
    if any(kw in title for kw in ['ShowContinue', 'Show Continue', 'SimulateNext', 'Simulate Next',
                                    'DisplayNpcNode', 'ToNpcReply', 'To Npc Reply',
                                    'GoToNpcReply', 'Go to Npc Reply',
                                    'PlayPlayerReply', 'Play Player Reply',
                                    'Clear Children']):
        # Get exec connections
        exec_in = []
        exec_out = []
        for p in n.get('pins',[]):
            if p.get('type') == 'exec':
                for c in p.get('connections',[]):
                    src = node_map.get(c['nodeId'],{})
                    st = src.get('title','').replace('\n',' ')[:40]
                    if p.get('direction') == 'Input':
                        exec_in.append(f"{st} ({c['nodeId'][:12]}..:{c['pinName']})")
                    else:
                        exec_out.append(f"{st} ({c['nodeId'][:12]}..:{c['pinName']})")
        print(f"\n  [{cls[:20]}] {title[:60]}")
        print(f"  ID: {nid}")
        if exec_in: print(f"  EXEC IN:  {exec_in}")
        if exec_out: print(f"  EXEC OUT: {exec_out}")
