"""
capture_map.py — Automated map texture capture for The Signal

Captures top-down orthographic views of map areas defined by AZP_MapVolume actors.
Strips visual noise (lighting, fog, particles) for clean floor-plan-style results.

Usage: Run in UE5 editor via Python console or MCP endpoint:
  exec(open('C:/Users/Ommei/workspace/TheSignal/Scripts/Python/capture_map.py').read())

Prerequisites:
  - AZP_MapVolume actors placed in level with AreaID set
  - CaptureHeight set below ceiling height on each volume
  - Ceiling/roof actors hidden or camera below them
"""

import unreal
import os


def capture_all_maps(resolution=2048):
    """Capture map textures for all MapVolume actors in the current level."""

    world = unreal.EditorLevelLibrary.get_editor_world()

    # Find all MapVolume actors
    all_actors = unreal.EditorLevelLibrary.get_all_level_actors()
    volumes = [a for a in all_actors if 'ZP_MapVolume' in a.get_class().get_name()]

    if not volumes:
        unreal.log_warning('[MapCapture] No AZP_MapVolume actors found in level.')
        return

    unreal.log(f'[MapCapture] Found {len(volumes)} map volume(s)')

    for volume in volumes:
        area_id = volume.get_editor_property('area_id')
        if not area_id or str(area_id) == 'None':
            unreal.log_warning(f'[MapCapture] Volume {volume.get_name()} has no AreaID - skipping')
            continue

        capture_area(volume, resolution)


def capture_area(volume, resolution=2048):
    """Capture a single map area from a MapVolume actor."""

    world = unreal.EditorLevelLibrary.get_editor_world()
    area_id = str(volume.get_editor_property('area_id'))
    capture_height = volume.get_editor_property('capture_height')

    # Get volume bounds
    bounds_comp = volume.get_editor_property('area_bounds')
    center = volume.get_actor_location()
    extent = bounds_comp.get_scaled_box_extent() if bounds_comp else unreal.Vector(x=1000, y=1000, z=500)

    # Camera position: above center of area, looking straight down
    cam_pos = unreal.Vector(x=center.x, y=center.y, z=center.z + capture_height)
    cam_rot = unreal.Rotator(roll=0, pitch=-90, yaw=-90)

    # Ortho width covers the larger axis (capture is square)
    ortho_width = max(extent.x, extent.y) * 2.0

    unreal.log(f'[MapCapture] Capturing "{area_id}" at ({cam_pos.x:.0f}, {cam_pos.y:.0f}, {cam_pos.z:.0f}), ortho={ortho_width:.0f}')

    # Create render target
    rt = unreal.RenderingLibrary.create_render_target2d(
        world, resolution, resolution,
        unreal.TextureRenderTargetFormat.RTF_RGBA8)

    # Spawn SceneCapture2D
    capture_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SceneCapture2D, cam_pos, cam_rot)

    if not capture_actor:
        unreal.log_error('[MapCapture] Failed to spawn SceneCapture2D')
        return

    # Configure capture component
    comp = capture_actor.get_editor_property('capture_component2d')
    comp.set_editor_property('projection_type', unreal.CameraProjectionMode.ORTHOGRAPHIC)
    comp.set_editor_property('ortho_width', ortho_width)
    comp.set_editor_property('texture_target', rt)

    # Strip visual noise for clean map capture
    show_flags = []
    flags_to_disable = [
        'Fog', 'Atmosphere', 'Bloom', 'EyeAdaptation',
        'MotionBlur', 'ScreenSpaceReflections', 'AmbientOcclusion',
        'DynamicShadows', 'VolumetricFog',
        'Particles', 'Translucency', 'Decals',
        'DepthOfField', 'Grain', 'Vignette',
        'ToneCurve', 'LensFlares',
    ]
    for flag_name in flags_to_disable:
        flag = unreal.EngineShowFlagsSetting()
        flag.set_editor_property('show_flag_name', flag_name)
        flag.set_editor_property('enabled', False)
        show_flags.append(flag)

    comp.set_editor_property('show_flag_settings', show_flags)

    # Capture the scene
    comp.capture_scene()

    # Export to disk as HDR, then import as Texture2D
    export_dir = 'C:/Users/Ommei/workspace/TheSignal/Saved/MapCaptures'
    os.makedirs(export_dir, exist_ok=True)
    export_name = f'T_Map_{area_id}'

    unreal.RenderingLibrary.export_render_target(
        world, rt, export_dir, export_name)

    # UE5 exports RGBA8 render targets as PNG but without extension — rename
    raw_path = f'{export_dir}/{export_name}'
    png_path = f'{export_dir}/{export_name}.png'
    if os.path.exists(raw_path) and not os.path.exists(png_path):
        os.rename(raw_path, png_path)

    unreal.log(f'[MapCapture] Exported to {png_path}')

    # Import back into project as a Texture2D asset
    content_path = '/Game/TheSignal/Textures/Maps'

    if os.path.exists(png_path):
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', png_path)
        task.set_editor_property('destination_path', content_path)
        task.set_editor_property('destination_name', export_name)
        task.set_editor_property('automated', True)
        task.set_editor_property('save', True)
        task.set_editor_property('replace_existing', True)

        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        asset_tools.import_asset_tasks([task])

        unreal.log(f'[MapCapture] Imported as {content_path}/{export_name}')
    else:
        unreal.log_warning(f'[MapCapture] PNG file not found at {png_path} - manual import needed')

    # Cleanup
    capture_actor.destroy_actor()
    unreal.log(f'[MapCapture] Done capturing area "{area_id}"')


def capture_single(area_id, resolution=2048):
    """Capture a specific area by AreaID."""

    all_actors = unreal.EditorLevelLibrary.get_all_level_actors()
    for actor in all_actors:
        if 'ZP_MapVolume' in actor.get_class().get_name():
            aid = str(actor.get_editor_property('area_id'))
            if aid == area_id:
                capture_area(actor, resolution)
                return

    unreal.log_error(f'[MapCapture] No MapVolume found with AreaID "{area_id}"')


# Auto-run: capture all map volumes in the current level
capture_all_maps()
