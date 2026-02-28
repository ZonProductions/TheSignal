import unreal
bp_class = unreal.load_object(None, '/Game/Player/Blueprints/BP_GraceTactical.BP_GraceTactical_C')
cdo = unreal.get_default_object(bp_class)
cap = cdo.capsule_component
if cap:
    hh = cap.get_editor_property('capsule_half_height')
    r = cap.get_editor_property('capsule_radius')
    print(f'Capsule HalfHeight: {hh}, Radius: {r}')
beh = cdo.get_editor_property('base_eye_height')
print(f'BaseEyeHeight: {beh}')
