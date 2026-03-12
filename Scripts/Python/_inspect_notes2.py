import unreal

bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes')
print(f"Loaded: {bp.get_name()}, Class: {bp.get_class().get_name()}")

# Try different ways to access WidgetTree
# Method 1: find_object with subobject path
path = bp.get_path_name()
print(f"BP path: {path}")

wt = unreal.find_object(None, path + '.WidgetTree')
if wt:
    print(f"Found WidgetTree via find_object: {wt.get_name()} ({wt.get_class().get_name()})")
else:
    print("find_object('WidgetTree') returned None")

# Method 2: try direct attribute
try:
    wt2 = bp.widget_tree
    print(f"bp.widget_tree = {wt2}")
except Exception as e:
    print(f"bp.widget_tree error: {e}")

# Method 3: WidgetTree_0 (UE naming convention for subobjects)
wt3 = unreal.find_object(None, path + '.WidgetTree_0')
if wt3:
    print(f"Found WidgetTree_0: {wt3.get_name()} ({wt3.get_class().get_name()})")
else:
    print("find_object('WidgetTree_0') returned None")

# Method 4: list all inner objects of this BP
print("\n--- Searching for WidgetTree in all loaded objects ---")
# Try to find by iterating
for suffix in ['WidgetTree', 'WidgetTree_0', 'WidgetTree_1']:
    obj = unreal.find_object(None, f"{path}.{suffix}")
    if obj:
        print(f"  FOUND: {path}.{suffix} -> {obj.get_class().get_name()}")

# Method 5: Check if WidgetBlueprintGeneratedClass has widget tree ref
gen = bp.generated_class()
print(f"\nGeneratedClass: {gen.get_name()}")

# Let's see what properties WidgetBlueprint exposes
print("\n--- WidgetBlueprint class properties ---")
for prop_name in dir(bp):
    if 'widget' in prop_name.lower() or 'tree' in prop_name.lower():
        print(f"  {prop_name}")
