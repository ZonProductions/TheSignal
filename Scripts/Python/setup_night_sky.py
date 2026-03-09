"""Set up a tech campus at night atmosphere.
Fixes: visible night sky, moonlight, ground plane, proper exposure."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

# ========== Find existing actors ==========
dir_light = None
sky_light = None
pp_volume = None
sky_atmo = None
fog_actor = None

for a in all_actors:
    cls = a.get_class().get_name()
    label = a.get_actor_label()
    if cls == 'DirectionalLight' and label == 'Light Source':
        dir_light = a
    elif cls == 'SkyLight':
        sky_light = a
    elif cls == 'PostProcessVolume':
        pp_volume = a
    elif 'SkyAtmosphere' in cls:
        sky_atmo = a
    elif 'ExponentialHeightFog' in cls:
        fog_actor = a

# ========== 1. DirectionalLight → Moon ==========
if dir_light:
    light_comp = dir_light.get_component_by_class(unreal.DirectionalLightComponent)
    if light_comp:
        # Moon ~30° above horizon, coming from the side
        dir_light.set_actor_rotation(unreal.Rotator(roll=0, pitch=-30, yaw=220), False)
        light_comp.set_editor_property('intensity', 10.0)  # Bright enough to see
        light_comp.set_editor_property('light_color', unreal.Color(r=160, g=180, b=230, a=255))
        light_comp.set_editor_property('use_temperature', True)
        light_comp.set_editor_property('temperature', 7500.0)
        light_comp.set_editor_property('atmosphere_sun_light', True)
        light_comp.set_editor_property('atmosphere_sun_light_index', 0)
        light_comp.set_editor_property('cast_shadows', True)
        print("DirectionalLight → moon configured")

# ========== 2. SkyLight → Night ambient fill ==========
if sky_light:
    sky_comp = sky_light.get_component_by_class(unreal.SkyLightComponent)
    if sky_comp:
        sky_comp.set_editor_property('intensity', 3.0)
        sky_comp.set_editor_property('light_color', unreal.Color(r=30, g=40, b=70, a=255))
        print("SkyLight → night ambient configured")

# ========== 3. SkyAtmosphere ==========
if not sky_atmo:
    sky_atmo = eas.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    if sky_atmo:
        sky_atmo.set_actor_label("NightSkyAtmosphere")
        print("SkyAtmosphere spawned")
else:
    print(f"SkyAtmosphere exists: {sky_atmo.get_actor_label()}")

# ========== 4. ExponentialHeightFog ==========
if not fog_actor:
    fog_actor = eas.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 100))
    if fog_actor:
        fog_actor.set_actor_label("NightFog")
        print("ExponentialHeightFog spawned")
else:
    print(f"ExponentialHeightFog exists: {fog_actor.get_actor_label()}")

if fog_actor:
    fog_comp = fog_actor.get_component_by_class(unreal.ExponentialHeightFogComponent)
    if fog_comp:
        fog_comp.set_fog_density(0.005)
        fog_comp.set_fog_height_falloff(0.2)
        fog_comp.set_fog_inscattering_color(unreal.LinearColor(r=0.01, g=0.015, b=0.04, a=1.0))
        fog_comp.set_volumetric_fog(True)
        fog_comp.set_volumetric_fog_scattering_distribution(0.2)
        fog_comp.set_volumetric_fog_albedo(unreal.Color(r=20, g=25, b=50, a=255))
        print("ExponentialHeightFog → night configured")

# ========== 5. PostProcessVolume → Visible night ==========
if pp_volume:
    settings = pp_volume.get_editor_property('settings')
    # Use auto exposure so interior lights still look right
    settings.set_editor_property('override_auto_exposure_method', True)
    settings.set_editor_property('auto_exposure_method', unreal.AutoExposureMethod.AEM_HISTOGRAM)
    settings.set_editor_property('override_auto_exposure_bias', True)
    settings.set_editor_property('auto_exposure_bias', 0.0)
    settings.set_editor_property('override_auto_exposure_min_brightness', True)
    settings.set_editor_property('auto_exposure_min_brightness', 0.03)
    settings.set_editor_property('override_auto_exposure_max_brightness', True)
    settings.set_editor_property('auto_exposure_max_brightness', 8.0)
    # Subtle cool tint
    settings.set_editor_property('override_white_temp', True)
    settings.set_editor_property('white_temp', 5800.0)
    settings.set_editor_property('override_white_tint', True)
    settings.set_editor_property('white_tint', -0.15)
    # Bloom for light glow
    settings.set_editor_property('override_bloom_intensity', True)
    settings.set_editor_property('bloom_intensity', 0.6)
    # Subtle vignette
    settings.set_editor_property('override_vignette_intensity', True)
    settings.set_editor_property('vignette_intensity', 0.25)
    pp_volume.set_editor_property('settings', settings)
    print("PostProcessVolume → night grading configured")

# ========== 6. Ground Plane ==========
# Spawn a large dark ground plane so there's visible terrain
existing_ground = [a for a in all_actors if a.get_actor_label() == 'NightGround']
if existing_ground:
    ground = existing_ground[0]
    print(f"Ground plane already exists")
else:
    ground = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -15))
    if ground:
        ground.set_actor_label("NightGround")
        smc = ground.get_component_by_class(unreal.StaticMeshComponent)
        if smc:
            # Use engine Plane mesh
            plane_mesh = unreal.load_asset("/Engine/BasicShapes/Plane")
            if plane_mesh:
                smc.set_static_mesh(plane_mesh)
            smc.set_world_scale3d(unreal.Vector(200, 200, 1))  # 200m x 200m
            smc.set_editor_property('cast_shadow', False)
            # Dark asphalt-like material
            dark_mat = unreal.load_asset("/Game/TheSignal/Materials/M_DoorSignDark")
            if dark_mat:
                smc.set_material(0, dark_mat)
        print("Ground plane spawned (200m x 200m dark surface)")

print("\n=== Night sky setup complete ===")
