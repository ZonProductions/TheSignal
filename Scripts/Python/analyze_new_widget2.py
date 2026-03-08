import json

with open(r"C:\Users\Ommei\.claude\projects\C--Users-Ommei-workspace-TheSignal\8c7d7d48-3075-43f3-b784-dc1b1a77b926\tool-results\mcp-blueprint-mcp-get_blueprint_graph-1772846052660.txt", "r") as f:
    raw = f.read()

data = json.loads(raw)
text = data[0]['text']
graph = json.loads(text)
nodes = graph['nodes']
node_map = {n['id']: n for n in nodes}

print(f"Total nodes: {len(nodes)}")

# Find all key nodes
keywords = ['ShowContinue', 'Show Continue', 'SimulateNext', 'Simulate Next',
            'DisplayNpcNode', 'ToNpcReply', 'To Npc Reply',
            'GoToNpcReply', 'Go to Npc', 'PlayPlayerReply', 'Play Player Reply',
            'Clear Children', 'currentlySelectedReply']

for n in nodes:
    title = n.get('title','').replace('\n',' ')
    cls = n.get('class','')
    nid = n.get('id','')
    if any(kw in title for kw in keywords):
        exec_in = []
        exec_out = []
        for p in n.get('pins',[]):
            if p.get('type') == 'exec':
                for c in p.get('connections',[]):
                    src = node_map.get(c['nodeId'],{})
                    st = src.get('title','').replace('\n',' ')[:50]
                    if p.get('direction') == 'Input':
                        exec_in.append(f"{st} ({c['nodeId'][:16]}:{c['pinName']})")
                    else:
                        exec_out.append(f"{st} ({c['nodeId'][:16]}:{c['pinName']})")
        print(f"\n[{cls[:30]}] {title[:70]}")
        print(f"  ID: {nid}")
        if exec_in: print(f"  IN:  {exec_in}")
        if exec_out: print(f"  OUT: {exec_out}")
