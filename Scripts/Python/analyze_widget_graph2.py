import json

with open(r"C:\Users\Ommei\.claude\projects\C--Users-Ommei-workspace-TheSignal\8c7d7d48-3075-43f3-b784-dc1b1a77b926\tool-results\mcp-blueprint-mcp-get_blueprint_graph-1772841627577.txt", "r") as f:
    raw = f.read()

data = json.loads(raw)
text = data[0]['text']
graph = json.loads(text)
nodes = graph['nodes']
node_map = {n['id']: n for n in nodes}

# Search more broadly - titles may have newlines
print("=== All nodes with 'Continue' or 'ToNpc' in title ===")
for n in nodes:
    title = n.get('title','').replace('\n',' ')
    cls = n.get('class','')
    nid = n.get('id','')
    if 'Continue' in title or 'ToNpc' in title or 'Npc Reply' in title:
        print(f'\n  [{cls}] {title}')
        print(f'  ID: {nid}')
        for p in n.get('pins',[]):
            conns = p.get('connections',[])
            if conns or p.get('name') == 'execute':
                conn_strs = []
                for c in conns:
                    src = node_map.get(c['nodeId'],{})
                    stitle = src.get('title','?').replace('\n',' ')[:40]
                    conn_strs.append(f"{stitle} ({c['nodeId'][:12]}..:{c['pinName']})")
                print(f"    {p['name']} ({p['direction']}): {conn_strs if conn_strs else 'NONE'}")

print("\n\n=== Tracing what feeds into each ShowContinue call ===")
for n in nodes:
    title = n.get('title','').replace('\n',' ')
    nid = n.get('id','')
    if 'Show Continue Button' in title and 'CallFunction' in n.get('class',''):
        # Find what's connected to the execute input
        for p in n.get('pins',[]):
            if p.get('name') == 'execute' and p.get('direction') == 'Input':
                for c in p.get('connections',[]):
                    src = node_map.get(c['nodeId'],{})
                    stitle = src.get('title','?').replace('\n',' ')[:60]
                    print(f'\nShowContinue ({nid[:12]}..)')
                    print(f'  Fed by: {stitle} pin={c["pinName"]}')
                    # Now trace back one more level
                    for sp in src.get('pins',[]):
                        if sp.get('name') == 'execute' and sp.get('direction') == 'Input':
                            for sc in sp.get('connections',[]):
                                ss = node_map.get(sc['nodeId'],{})
                                sstitle = ss.get('title','?').replace('\n',' ')[:60]
                                print(f'  Fed by (2): {sstitle} pin={sc["pinName"]}')
