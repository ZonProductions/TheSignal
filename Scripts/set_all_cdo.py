"""
Set ALL CDO properties on BP_GraceCharacter, PC_Grace, GM_TheSignal.
Run this via MCP Python endpoint after every C++ rebuild:
  exec(open('C:/Users/Ommei/workspace/TheSignal/Scripts/set_all_cdo.py').read())

Sets: MovementConfig, ALL input actions (15+), bAutoSpawnWeapon,
      BulletDecalMaterials, IMC, GameMode classes.
      (StartingWeaponItem: read-only log — inherited from the C++ ctor, never set here.)
"""
import unreal

eal = unreal.EditorAssetLibrary

# ── Helper ─────────────────────────────────────────────────────────
def get_cdo(bp_asset):
    gen_class = bp_asset.generated_class()
    return unreal.get_default_object(gen_class)

def load(path):
    asset = unreal.load_asset(path)
    if not asset:
        unreal.log_error(f'FAILED to load: {path}')
    return asset

# ── Load all assets ────────────────────────────────────────────────

# Blueprints
bp_grace = load('/Game/Core/Player/BP_GraceCharacter')
bp_pc    = load('/Game/Core/Player/PC_Grace')
bp_gm    = load('/Game/Core/Framework/GM_TheSignal')

# Data Assets
da_movement = load('/Game/Core/Data/DA_GraceMovement_Default')
# NOTE: StartingWeaponItem is deliberately NOT set here. The C++ ctor (ZP_GraceCharacter.cpp)
# is its single source of truth and BP_GraceCharacter INHERITS it. Setting it from this script
# serializes a BP override that shadows future C++ changes — that's how the pistol regression
# happened (this script held DA_Grace_Pistol long after the design moved to a pipe start).

# Input Mapping Context
imc_grace = load('/Game/Core/Input/IMC_Grace')

# Core Input Actions
ia_move     = load('/Game/Core/Input/Actions/IA_Move')
ia_look     = load('/Game/Core/Input/Actions/IA_Look')
ia_sprint   = load('/Game/Core/Input/Actions/IA_Sprint')
ia_jump     = load('/Game/Core/Input/Actions/IA_Jump')
ia_interact = load('/Game/Core/Input/Actions/IA_Interact')

# Try loading additional core actions (may be in different paths)
ia_crouch = load('/Game/Core/Input/Actions/IA_Crouch')
ia_peek   = load('/Game/Core/Input/Actions/IA_Peek')
ia_aim    = load('/Game/Core/Input/Actions/IA_Aim')
ia_fire   = load('/Game/Core/Input/Actions/IA_Fire')
ia_reload = load('/Game/Core/Input/Actions/IA_Reload')

# Inventory menu (still uses Moonville's action — Tab key)
ia_inv_menu = load('/Game/InventorySystemPro/Blueprints/Input/InventoryCharacter/IA_InventoryMenuOpen')

# Weapon slot input actions — USE MOONVILLE'S actions that IMC_Grace ALREADY maps.
# IMC_Grace permanently maps: IA_InventorySlot0→One, Slot1→Two, Slot2→Three, Slot3→Four.
# Python IMC modifications (unmap_key/map_key) don't persist to disk — save_asset is broken for IMCs.
# So we bind to the EXISTING mappings: IA_InventorySlot0-3 which fire on Keys 1-4.
ia_slot0    = load('/Game/InventorySystemPro/Blueprints/Input/InventoryCharacter/IA_InventorySlot0')
ia_slot1    = load('/Game/InventorySystemPro/Blueprints/Input/InventoryCharacter/IA_InventorySlot1')
ia_slot2    = load('/Game/InventorySystemPro/Blueprints/Input/InventoryCharacter/IA_InventorySlot2')
ia_slot3    = load('/Game/InventorySystemPro/Blueprints/Input/InventoryCharacter/IA_InventorySlot3')

# Bullet decal materials (UWC pack — TICKET-015 Metal_Solid set).
# Old /Game/UniversalWallClutter/... paths were stale and silently loaded nothing.
decal_mats = []
for i in range(1, 6):
    mat = load(f'/Game/UWC_Bullet_Holes/Instances/Decals/Metal_Solid/MI_Metal_Solid_{i}')
    if mat:
        decal_mats.append(mat)

# Map input action
ia_map = load('/Game/Core/Input/IA_Map')

# Tab cycling input actions
ia_tab_left = load('/Game/Core/Input/Actions/IA_TabCycleLeft')
ia_tab_right = load('/Game/Core/Input/Actions/IA_TabCycleRight')

# Inventory Tab Widget blueprint
wbp_inv_tab = load('/Game/Blueprints/UI/WBP_InventoryTab')

# Map Widget blueprint
wbp_map = load('/Game/User_Interface/WBP_Map')

# ── 1. BP_GraceCharacter CDO ──────────────────────────────────────
cdo = get_cdo(bp_grace)

# Movement config
if da_movement:
    cdo.set_editor_property('MovementConfig', da_movement)

# Core input actions
action_map = {
    'MoveAction': ia_move,
    'LookAction': ia_look,
    'SprintAction': ia_sprint,
    'JumpAction': ia_jump,
    'InteractAction': ia_interact,
    'CrouchAction': ia_crouch,
    'PeekAction': ia_peek,
    'AimAction': ia_aim,
    'FireAction': ia_fire,
    'ReloadAction': ia_reload,
}
for prop_name, action in action_map.items():
    if action:
        try:
            cdo.set_editor_property(prop_name, action)
            unreal.log(f'  {prop_name} = {action.get_name()}')
        except Exception as e:
            unreal.log_error(f'  {prop_name} FAILED: {e}')
    else:
        unreal.log(f'  {prop_name} = NOT FOUND (skipped)')

# Inventory input actions
inv_action_map = {
    'InventoryMenuAction': ia_inv_menu,
    'InventorySlot0Action': ia_slot0,
    'InventorySlot1Action': ia_slot1,
    'InventorySlot2Action': ia_slot2,
    'InventorySlot3Action': ia_slot3,
}
for prop_name, action in inv_action_map.items():
    if action:
        try:
            cdo.set_editor_property(prop_name, action)
            unreal.log(f'  {prop_name} = {action.get_name()}')
        except Exception as e:
            unreal.log_error(f'  {prop_name} FAILED: {e}')
    else:
        unreal.log_error(f'  {prop_name} = NOT FOUND!')

# Starting weapon item: NOT set here (inherited from the C++ ctor default — see note at the top).
# Log the effective value so the post-rebuild output still surfaces a wrong ctor line at a glance.
try:
    unreal.log(f"  StartingWeaponItem (inherited from C++) = {cdo.get_editor_property('StartingWeaponItem')}")
except Exception as e:
    unreal.log_error(f'  StartingWeaponItem read FAILED: {e}')

# Auto-spawn weapon = false (inventory manages weapon lifecycle)
try:
    cdo.set_editor_property('bAutoSpawnWeapon', True)
except:
    pass

# Map input action
if ia_map:
    try:
        cdo.set_editor_property('MapAction', ia_map)
        unreal.log(f'  MapAction = {ia_map.get_name()}')
    except Exception as e:
        unreal.log_error(f'  MapAction FAILED: {e}')

# Tab cycling
for prop, action in [('TabCycleLeftAction', ia_tab_left), ('TabCycleRightAction', ia_tab_right)]:
    if action:
        try:
            cdo.set_editor_property(prop, action)
            unreal.log(f'  {prop} = {action.get_name()}')
        except Exception as e:
            unreal.log_error(f'  {prop} FAILED: {e}')

# Bullet decal materials (TArray<TSoftObjectPtr<UMaterialInterface>>)
if decal_mats:
    try:
        cdo.set_editor_property('BulletDecalMaterials', decal_mats)
        unreal.log(f'  BulletDecalMaterials = {len(decal_mats)} materials')
    except Exception as e:
        unreal.log_error(f'  BulletDecalMaterials FAILED: {e}')

unreal.log('[set_all_cdo] BP_GraceCharacter CDO configured')

# ── 2. PC_Grace CDO ───────────────────────────────────────────────
if bp_pc and imc_grace:
    cdo_pc = get_cdo(bp_pc)
    cdo_pc.set_editor_property('DefaultMappingContext', imc_grace)
    cdo_pc.set_editor_property('DefaultMappingPriority', 1)
    unreal.log('[set_all_cdo] PC_Grace CDO: DefaultMappingContext = IMC_Grace (priority 1)')

    # Inventory Tab Widget class
    if wbp_inv_tab:
        try:
            cdo_pc.set_editor_property('InventoryTabWidgetClass', wbp_inv_tab.generated_class())
            unreal.log(f'  InventoryTabWidgetClass = {wbp_inv_tab.get_name()}')
        except Exception as e:
            unreal.log_error(f'  InventoryTabWidgetClass FAILED: {e}')

    # Map Widget class (legacy standalone — kept as fallback)
    if wbp_map:
        try:
            cdo_pc.set_editor_property('MapWidgetClass', wbp_map.generated_class())
            unreal.log(f'  MapWidgetClass = {wbp_map.get_name()}')
        except Exception as e:
            unreal.log_error(f'  MapWidgetClass FAILED: {e}')

# ── 3. GM_TheSignal CDO ───────────────────────────────────────────
if bp_gm and bp_grace and bp_pc:
    cdo_gm = get_cdo(bp_gm)
    cdo_gm.set_editor_property('DefaultPawnClass', bp_grace.generated_class())
    cdo_gm.set_editor_property('PlayerControllerClass', bp_pc.generated_class())
    unreal.log('[set_all_cdo] GM_TheSignal CDO: DefaultPawn + Controller set')

# ── 4. WBP_HUD CDO — weapon icon textures ─────────────────────────
# HUD assigns these to its Icon_* images in NativeConstruct (C++), so the
# designer never hand-assigns brushes. 5.7 note: UI textures must be
# never_stream=True or they render white (see fix_icon_neverstream.py).
wbp_hud = load('/Game/Blueprints/UI/WBP_HUD')
if wbp_hud:
    cdo_hud = get_cdo(wbp_hud)
    hud_icon_map = {
        'PistolIconTexture':  '/Game/icons/Icon_Pistol',
        'RifleIconTexture':   '/Game/icons/Icon_Rifle',
        'ShotgunIconTexture': '/Game/icons/Icon_Shotgun',
        'PipeIconTexture':    '/Game/icons/Icon_Pipe',
        'GrenadeIconTexture': '/Game/icons/Icon_Grenade',
    }
    for prop, path in hud_icon_map.items():
        tex = load(path)
        if tex:
            try:
                cdo_hud.set_editor_property(prop, tex)
                unreal.log(f'  {prop} = {tex.get_name()}')
            except Exception as e:
                unreal.log_error(f'  {prop} FAILED: {e}')
    eal.save_asset('/Game/Blueprints/UI/WBP_HUD')
    unreal.log('[set_all_cdo] WBP_HUD icon textures set + saved')

# ── Save ───────────────────────────────────────────────────────────
eal.save_asset('/Game/Core/Player/BP_GraceCharacter')
eal.save_asset('/Game/Core/Player/PC_Grace')
eal.save_asset('/Game/Core/Framework/GM_TheSignal')
unreal.log('[set_all_cdo] All Blueprints saved')

# ── Verify critical properties ─────────────────────────────────────
cdo = get_cdo(bp_grace)
for p in ['InventorySlot0Action', 'InventorySlot1Action', 'InventorySlot2Action', 'InventorySlot3Action', 'InventoryMenuAction']:
    try:
        val = cdo.get_editor_property(p)
        unreal.log(f'  VERIFY {p} = {val.get_name() if val else "NULL"}')
    except:
        unreal.log_error(f'  VERIFY {p} = FAILED TO READ')
