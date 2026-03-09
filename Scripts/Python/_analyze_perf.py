"""Analyze CSV profiling data and identify bottlenecks."""
import csv
import os

csv_path = "C:/Users/Ommei/workspace/TheSignal/Saved/Profiling/CSV/Profile(20260309_020244).csv"

with open(csv_path, 'r') as f:
    reader = csv.reader(f)
    headers = next(reader)
    rows = list(reader)

print(f"Captured {len(rows)} frames\n")

# Find column indices for key metrics
def find_col(name):
    for i, h in enumerate(headers):
        if h == name:
            return i
    return -1

key_metrics = {
    'FrameTime': find_col('FrameTime'),
    'GameThreadTime': find_col('GameThreadTime'),
    'RenderThreadTime': find_col('RenderThreadTime'),
    'GPUTime': find_col('GPUTime'),
    'RHIThreadTime': find_col('RHIThreadTime'),
    'DrawCalls': find_col('RHI/DrawCalls'),
    'PrimitivesDrawn': find_col('RHI/PrimitivesDrawn'),
    'TotalActorCount': find_col('ActorCount/TotalActorCount'),
    'StaticMeshActors': find_col('ActorCount/StaticMeshActor'),
    'SpotLights': find_col('ActorCount/SpotLight'),
    'RectLights': find_col('ActorCount/RectLight'),
    'PointLights': find_col('ActorCount/PointLight'),
    'GPUSceneInstances': find_col('GPUSceneInstanceCount'),
    'GPUMemUsedMB': find_col('GPUMem/UsedMB'),
    'GPUMemTotalMB': find_col('GPUMem/TotalMB'),
    'ShadowMapsUpdated': find_col('LightCount/UpdatedShadowMaps'),
    'ShadowCacheMB': find_col('ShadowCacheUsageMB'),
    'VirtualShadowPages': find_col('SceneCulling/NumStaticInstances'),
    'NaniteScalingShadow': find_col('DynamicNaniteScalingShadow'),
    'NaniteScalingPrimary': find_col('DynamicNaniteScalingPrimary'),
    'BasepassDrawCalls': find_col('DrawCall/Basepass'),
    'ShadowDrawCalls': find_col('DrawCall/ShadowDepths'),
    'PrepassDrawCalls': find_col('DrawCall/Prepass'),
    'LightDrawCalls': find_col('DrawCall/Lights'),
}

# Render thread breakdown
render_breakdown = {}
for h in headers:
    if h.startswith('Exclusive/RenderThread/') and not h.startswith('Exclusive/RenderThread/EventWait'):
        render_breakdown[h.replace('Exclusive/RenderThread/', '')] = find_col(h)

def get_vals(col_idx):
    vals = []
    for row in rows:
        try:
            v = float(row[col_idx])
            vals.append(v)
        except (ValueError, IndexError):
            pass
    return vals

def stats(vals):
    if not vals:
        return 0, 0, 0
    return min(vals), sum(vals)/len(vals), max(vals)

# Print key metrics
print("=== TIMING (ms) ===")
for name in ['FrameTime', 'GameThreadTime', 'RenderThreadTime', 'GPUTime', 'RHIThreadTime']:
    idx = key_metrics[name]
    if idx >= 0:
        mn, avg, mx = stats(get_vals(idx))
        fps_avg = 1000.0 / avg if avg > 0 else 0
        print(f"  {name:25s}: avg={avg:7.2f}  min={mn:7.2f}  max={mx:7.2f}  (avg {fps_avg:.0f} fps)")

print("\n=== DRAW CALLS ===")
for name in ['DrawCalls', 'PrimitivesDrawn', 'BasepassDrawCalls', 'ShadowDrawCalls', 'PrepassDrawCalls', 'LightDrawCalls']:
    idx = key_metrics[name]
    if idx >= 0:
        mn, avg, mx = stats(get_vals(idx))
        print(f"  {name:25s}: avg={avg:9.0f}  min={mn:9.0f}  max={mx:9.0f}")

print("\n=== SCENE COMPLEXITY ===")
for name in ['TotalActorCount', 'StaticMeshActors', 'GPUSceneInstances', 'SpotLights', 'RectLights', 'PointLights']:
    idx = key_metrics[name]
    if idx >= 0:
        mn, avg, mx = stats(get_vals(idx))
        print(f"  {name:25s}: avg={avg:9.0f}")

print("\n=== GPU MEMORY ===")
for name in ['GPUMemUsedMB', 'GPUMemTotalMB']:
    idx = key_metrics[name]
    if idx >= 0:
        mn, avg, mx = stats(get_vals(idx))
        print(f"  {name:25s}: avg={avg:9.1f} MB")

print("\n=== SHADOWS ===")
for name in ['ShadowMapsUpdated', 'ShadowCacheMB']:
    idx = key_metrics[name]
    if idx >= 0:
        mn, avg, mx = stats(get_vals(idx))
        print(f"  {name:25s}: avg={avg:9.1f}  max={mx:9.1f}")

print("\n=== RENDER THREAD BREAKDOWN (top 10 by avg ms) ===")
render_times = []
for name, idx in render_breakdown.items():
    if idx >= 0:
        vals = get_vals(idx)
        if vals:
            mn, avg, mx = stats(vals)
            render_times.append((name, avg, mx))

render_times.sort(key=lambda x: -x[1])
for name, avg, mx in render_times[:10]:
    print(f"  {name:45s}: avg={avg:7.3f}  max={mx:7.3f}")

# Identify bottleneck
print("\n=== BOTTLENECK ANALYSIS ===")
gt_avg = stats(get_vals(key_metrics['GameThreadTime']))[1] if key_metrics['GameThreadTime'] >= 0 else 0
rt_avg = stats(get_vals(key_metrics['RenderThreadTime']))[1] if key_metrics['RenderThreadTime'] >= 0 else 0
gpu_avg = stats(get_vals(key_metrics['GPUTime']))[1] if key_metrics['GPUTime'] >= 0 else 0

bottleneck = max([('Game Thread (CPU)', gt_avg), ('Render Thread (Draw)', rt_avg), ('GPU', gpu_avg)], key=lambda x: x[1])
print(f"  PRIMARY BOTTLENECK: {bottleneck[0]} at {bottleneck[1]:.2f} ms")

if gpu_avg > gt_avg and gpu_avg > rt_avg:
    print("  -> GPU bound. Likely causes: Lumen GI, Virtual Shadow Maps, too many lights, high poly count")
elif rt_avg > gt_avg:
    print("  -> Draw thread bound. Likely causes: too many draw calls, too many individual actors")
else:
    print("  -> Game thread bound. Likely causes: too many ticking actors, physics, AI")
