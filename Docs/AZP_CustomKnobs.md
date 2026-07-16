# AZP_ Custom Knob Registry — The Signal

_Generated 2026-07-03 from the AZP_ exposure + rename sweep. Regenerate with
`scratchpad/gen_docs.py` after adding knobs, or extend by hand using the same table shape._

## What this is
Every **custom tunable value** in `Source/TheSignal`, exposed as an editor knob where possible.
All custom values carry the **`AZP_` prefix** (bools: `bAZP_`), so:
- **In code:** grep `AZP_` to find every custom point.
- **In the editor:** type `AZP` into any Details-panel search box to filter a Blueprint down to only the custom knobs.

## Conventions and deliberate exceptions
- House knob style: `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Sub")`.
- **USTRUCT members are NOT prefixed** (marked in tables): `ZP_ObjectiveDef`/`ZP_SubObjectiveDef` load from `Content/Data/Objectives.json` by field name (`FJsonObjectConverter` ignores redirects); `FZP_NoteEntry` is SaveGame-serialized. Their owning `FZP_`/`ZP_` type already marks them custom.
- `AZP_TransitLocation.DisplayName` keeps its old name (shares a name with a struct member).
- `meta=(BindWidget)` widget pointers, delegates, and component subobjects are not values and were not renamed.
- **Function/delegate PARAMETERS are never `AZP_`-prefixed.** Blueprint call/event nodes reference
  parameters by pin name and CoreRedirects do not cover pins — a renamed param breaks every wired BP
  call ("In use pin no longer exists" → Blueprint BAD). Renamed params from the sweep were reverted
  2026-07-03 (deposit library `RequiredItems`, dialogue `DialogueID`, Kinemation `InterpSpeed`).
- Values whose exposure column says *internal (C++ only)* are custom tunables the sweep deliberately kept code-side (or whose literal-promotion was unsafe, e.g. used in static functions — see the per-class apply reports).

## Serialized-data safety
- Every pre-existing renamed UPROPERTY has a `[CoreRedirects]` PropertyRedirect in `Config/DefaultEngine.ini` (659 entries), so **saved BP defaults and level-instance overrides survive** (e.g. Shambler `AZP_AttackDamage` keeps the tuned 12.5).
- **Old save files caveat:** SaveGame-flagged renamed props (`AZP_ObjectiveStateSlot`, `bAZP_AutoPersist`, `AZP_DialogueID`) may not restore from saves made before 2026-07-03.
- Reusable Python scripts were updated to the new names; one-off `_`-prefixed diagnostics scripts still use OLD names (deliberately stale).

## Registry — 1022 knobs across 76 classes (AZP_OozelingBase v3.3 erupt-cutoff +1, v3.2 burst-sound +1, v3.1 eruption-fuse +4, v3 touch-burst +4, v2.2 crowding-facing +1, v2.1 auto-bind +2, v2 aggro/chase/death-fall +19 — all 2026-07-13; SM_Surface + UZP_WarmupGateSubsystem 2026-07-12). NOTE: BP_Oozeling's 7 anim slots are now STAMPED on the BP CDO (Scripts/Python/stamp_oozeling_anim_defaults.py, 2026-07-13) — tune clips in the BP Details panel; the C++ lazy defaults only fill slots left empty.

### AZP_AmbientMusicPlayer
_Level-placed 2D ambient music actor that plays an assigned SoundBase at random sparse intervals with fade in/out._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_SoundToPlay` | `TObjectPtr<USoundBase>` | `null` | Music | EditAnywhere + BP | The SoundBase asset the ambient player loops on its random interval schedule. |
| `AZP_Volume` | `float` | `0.4f` | Music | EditAnywhere + BP | Playback volume multiplier (0-1) applied to the audio component and used as the fade-in target level. |
| `AZP_FadeInTime` | `float` | `3.0f` | Music | EditAnywhere + BP | Seconds the music takes to fade in when a play cycle starts. |
| `AZP_FadeOutTime` | `float` | `3.0f` | Music | EditAnywhere + BP | Seconds of fade-out, started this many seconds before the sound's natural end. |
| `AZP_MinInterval` | `float` | `10.0f` | Music | EditAnywhere + BP | Minimum silence in seconds between two plays of the ambient track. |
| `AZP_MaxInterval` | `float` | `30.0f` | Music | EditAnywhere + BP | Maximum silence in seconds between two plays of the ambient track. |
| `AZP_FirstPlayDelayMin` | `float` | `1.0f` | Music | EditAnywhere + BP | Lower bound of the random delay before the very first play after BeginPlay (FMath::RandRange(1.0f, 5.0f)). |
| `AZP_FirstPlayDelayMax` | `float` | `5.0f` | Music | EditAnywhere + BP | Upper bound of the random delay before the very first play after BeginPlay (FMath::RandRange(1.0f, 5.0f)). |

### AZP_CardReaderPanel
_Wall-mounted card reader panel: on E-press it checks the player's Moonville inventory (via reflection) for a required key item, consumes it, unlocks/opens a linked LockableDoor plus any radius-auto-locked InteractDoors, flips its status light red->green, and sets objective flags._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_RequiredItemDA` | `TSoftObjectPtr<UObject>` | `null` | CardReader | EditAnywhere + BP | The Moonville PDA_Item data asset the player must carry for this reader to unlock. |
| `AZP_RequiredItemName` | `FText` | `FText::FromString(TEXT("Key Card"))` | CardReader | EditAnywhere + BP | Player-facing display name of the required item, substituted into the missing-item HUD message. |
| `AZP_LinkedDoor` | `TObjectPtr<AZP_LockableDoor>` | `null` | CardReader | EditInstanceOnly + BP | The slide/gate LockableDoor actor this panel unlocks and opens on successful card use (per-instance level wiring). |
| `AZP_DoorLockRadius` | `float` | `300.f` | CardReader | EditAnywhere + BP | Radius (UU) around the panel within which InteractDoors are auto-locked at BeginPlay and later unlocked with the card. |
| `bAZP_ConsumeKeyOnUse` | `bool` | `true` | CardReader | EditAnywhere + BP | Whether the key item is removed from the player's inventory after unlocking (false = reusable key card). |
| `AZP_PromptText` | `FText` | `FText::FromString(TEXT("Use Card Reader"))` | CardReader | EditAnywhere + BP | HUD interaction prompt shown while the player is inside the panel's overlap volume. |
| `AZP_MissingItemMessage` | `FText` | `FText::FromString(TEXT("Required: {0}"))` | CardReader | EditAnywhere + BP | HUD format string shown when the player lacks the key item; {0} is replaced by RequiredItemName. |
| `AZP_AccessGrantedMessage` | `FText` | `FText::FromString(TEXT("Access Granted"))` | CardReader | EditAnywhere + BP | HUD message shown when the card is accepted and the door unlocks. |
| `AZP_ObjectiveFlagOnTry` | `FName` | `NAME_None` | CardReader|Objective | EditAnywhere + BP | Objective flag set on the ObjectiveSubsystem the first time the player interacts with the (locked) reader, used to reveal hidden sub-objectives. |
| `AZP_ObjectiveFlagOnUnlock` | `FName` | `NAME_None` | CardReader|Objective | EditAnywhere + BP | Objective flag set on the ObjectiveSubsystem when the reader is successfully unlocked, used to complete access-gated objective steps. |
| `AZP_LockedLightColor` | `FLinearColor` | `FLinearColor(0.8f, 0.1f, 0.1f)` | CardReader|Light | EditAnywhere + BP | Status light color while the panel is locked (red), set in the constructor. |
| `AZP_UnlockedLightColor` | `FLinearColor` | `FLinearColor(0.1f, 0.8f, 0.1f)` | CardReader|Light | EditAnywhere + BP | Status light color after a successful unlock (green), hardcoded inside UseKey. |

### AZP_CrawlerBase
_Base Character class for all creatures: swaps in UZP_CrawlerMovementComponent as default CMC, owns health/behavior components, applies tentacle material post-spawn, and runs the freeze-on-ground death sequence._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_CreatureSeed` | `int32` | `125` | Creature Config | EditAnywhere + BP | Random seed fed to the Monster Randomizer so a creature variant's procedural appearance is reproducible. |
| `AZP_MinLegCount` | `int32` | `6` | Creature Config | EditAnywhere + BP | Minimum number of tentacle legs the Monster Randomizer spawns for this creature variant. |
| `AZP_MaxLegCount` | `int32` | `6` | Creature Config | EditAnywhere + BP | Maximum number of tentacle legs the Monster Randomizer spawns for this creature variant. |
| `AZP_MinGeneralScale` | `float` | `0.25f` | Creature Config | EditAnywhere + BP | Lower bound of the creature's randomized overall body scale. |
| `AZP_MaxGeneralScale` | `float` | `0.3f` | Creature Config | EditAnywhere + BP | Upper bound of the creature's randomized overall body scale. |
| `AZP_MinLegScale` | `float` | `0.9f` | Creature Config | EditAnywhere + BP | Lower bound of the per-leg randomized scale for tentacle legs. |
| `AZP_MaxLegScale` | `float` | `1.1f` | Creature Config | EditAnywhere + BP | Upper bound of the per-leg randomized scale for tentacle legs. |
| `AZP_CreatureSpeedMultiplier` | `float` | `0.75f` | Creature Config | EditAnywhere + BP | Global movement speed multiplier applied to this creature variant. |
| `AZP_TentacleMaterial` | `TObjectPtr<UMaterialInterface>` | `null` | Creature Config | EditAnywhere + BP | Material override applied to all tentacle legs and body meshes 4.5s after spawn, replacing MI_Flesh; null means keep pack materials. |
| `AZP_RuntimeRoughnessOverride` | `float` | `0.12f` | Creature Config | EditAnywhere + BP | Roughness value forced onto a dynamic material instance of TentacleMaterial to blur SSR floor reflection bleed; 0 keeps the material's original roughness. |
| `AZP_TentacleMaterialSwapDelay` | `float` | `4.5f` | Creature Config|Material Swap | EditAnywhere + BP | Seconds after BeginPlay before TentacleMaterial is applied; must exceed BP_Monster_Pawn's 3s Monster Randomizer delay so legs exist when the swap runs. |
| `AZP_CorpseLifeSpan` | `float` | `30.0f` | Death | EditAnywhere + BP | Seconds the frozen corpse and its orphaned leg/body/eye actors persist before destruction (SetLifeSpan at ZP_CrawlerBase.cpp:374 and ZP_CrawlerBase.cpp:379 — one tunable, two sites). |

### AZP_DialogueTrigger
_Placeable box-trigger actor that plays an assigned dialogue via the player's DialogueManager when the player enters, one-shot or repeatable._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_DialogueData` | `TObjectPtr<UZP_DialogueData>` | `null` | Dialogue | EditAnywhere + BP | The dialogue asset played when the player enters the trigger volume. |
| `bAZP_TriggerOnce` | `bool` | `true` | Dialogue | EditAnywhere + BP | If true the trigger disables itself after its first activation. |
| `bAZP_Enabled` | `bool` | `true` | Dialogue | EditAnywhere + BP | Starting enabled state of the trigger; also toggled at runtime by other systems and set false after first fire when bTriggerOnce. |

### AZP_DoorSign
_Solid sign panel + text render that covers existing door text; line-traces backward and auto-attaches to the door/wall actor so it moves with the door._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_SignBackgroundMesh` | `UStaticMesh*` | `/Engine/BasicShapes/Cube` | Door Sign|Appearance | EditAnywhere + BP | Mesh used for the solid background panel that covers the original door text. |
| `AZP_SignBackgroundMaterial` | `UMaterialInterface*` | `/Game/TheSignal/Materials/M_DoorSignDark` | Door Sign|Appearance | EditAnywhere + BP | Material on the background panel (dark cover material created by Scripts/Python/create_doorsign_material.py). |
| `AZP_SignPanelScale` | `FVector` | `FVector(0.005f, 0.3f, 0.1f)` | Door Sign|Appearance | EditAnywhere + BP | World scale of the background panel: ~0.5cm thick (X), 30cm wide (Y), 10cm tall (Z) — the sign's physical footprint on the door. |
| `AZP_DefaultSignText` | `FText` | `"ROOM 101"` | Door Sign|Text | EditAnywhere + BP | Neutral placeholder text the sign spawns with; the real text is authored per-instance on the SignText component (matches the dev's expose-player-facing-text rule). |
| `AZP_SignTextSize` | `float` | `8.f` | Door Sign|Text | EditAnywhere + BP | World size (cm) of the rendered sign text. |
| `AZP_AttachTraceDistance` | `float` | `300.f` | Door Sign|Attach | EditAnywhere + BP | How far backward (-X, cm) the sign traces to find the door/wall surface it auto-attaches to. |

### AZP_Elevator
_In-map kinematic elevator car that FInterps its platform between relative-Z stops and carries riders via engine based-movement; driven by AZP_TransitPanel._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_MoveSpeed` | `float` | `200.f` | Elevator | EditAnywhere + BP | Constant elevator travel speed in UU/s (real-elevator feel ~150-250 per header comment). |
| `AZP_ArriveTolerance` | `float` | `1.f` | Elevator | EditAnywhere + BP | Snap tolerance in UU within which the car counts as arrived at its target stop. |
| `AZP_MoveSound` | `TObjectPtr<USoundBase>` | `SFX_Elevator_Move (ctor FObjectFinder)` | Elevator\|Audio | EditAnywhere + BP | Looping travel sound started when the car starts moving, faded out on arrival; None = silent travel. |
| `AZP_MoveSoundCarry` | `EZP_SFXCarry` | `Close` | Elevator\|Audio | EditAnywhere + BP | How far the travel loop carries (Close ~30 m: audible riding the car or near the shaft only). |
| `AZP_MoveSoundVolume` | `float` | `1.f` | Elevator\|Audio | EditAnywhere + BP | Volume multiplier for the travel loop. |
| `AZP_MoveSoundFadeOut` | `float` | `0.4f` | Elevator\|Audio | EditAnywhere + BP | Seconds the travel loop fades out after the car parks. |
| `AZP_ArriveSound` | `TObjectPtr<USoundBase>` | `SFX_Elevator_Beep (ctor FObjectFinder)` | Elevator\|Audio | EditAnywhere + BP | One-shot played at the car every time it parks at a stop. |
| `AZP_ArriveSoundCarry` | `EZP_SFXCarry` | `Close` | Elevator\|Audio | EditAnywhere + BP | Carry profile for the arrival beep. |
| `AZP_ArriveSoundVolume` | `float` | `1.f` | Elevator\|Audio | EditAnywhere + BP | Volume multiplier for the arrival beep. |

### AZP_FloorSign
_Drag-and-drop floor number sign actor; FloorNumber picks MI_FloorSign_1..6 material automatically in editor and at runtime._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_FloorNumber` | `int32` | `1` | Floor Sign | EditAnywhere + BP | Floor number (1-6) the sign displays; drives which MI_FloorSign_N material is applied. |
| `AZP_FloorSignMaterialPathFormat` | `FString` | `"/Game/TheSignal/Materials/MI_FloorSign_%d.MI_FloorSign_%d"` | Floor Sign | EditAnywhere + BP | Hardcoded material-instance path pattern the sign loads per floor number; promoting lets BP children point at a different sign material family without code. |

### AZP_GraceCharacter
_First-person player character (Marcus; 'Grace' is legacy naming): thin shell owning camera/view-model meshes/components and input routing, plus block, dodge, grab-struggle, ladder, footsteps, fall damage, flashlight and the Moonville inventory/container reflection bridge._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_RangedArmsOffset` | `FVector` | `FVector::ZeroVector` | Appearance | EditAnywhere + BP | Live position offset for the ranged arm view-model (arms+hands+sleeve moved together) on top of the leader-posed Kinemation aim. |
| `AZP_RangedArmsRotation` | `FRotator` | `FRotator::ZeroRotator` | Appearance | EditAnywhere + BP | Live rotation offset for the ranged arm view-model. |
| `AZP_RangedArmsScale` | `float` | `1.0f` | Appearance | EditAnywhere + BP | Uniform scale of the ranged arm view-model meshes. |
| `AZP_MarcusIdle` | `TObjectPtr<UAnimSequenceBase>` | `null (runtime-loaded /Game/Marcus/Anims/Marcus_M_Neutral_Stand_Idle_Loop)` | Appearance|MarcusClips | internal (C++ only) | Native CCMH idle clip played on Marcus's visible body via SingleNode; currently overwritten by a hardcoded LoadObject path in SetupMarcusAppearance (ZP_GraceCharacter.cpp:4324). |
| `AZP_MarcusWalk` | `TObjectPtr<UAnimSequenceBase>` | `null (runtime-loaded Marcus_M_Neutral_Walk_Loop_F)` | Appearance|MarcusClips | internal (C++ only) | Native CCMH walk clip for Marcus's visible body (hardcoded path at cpp:4325). |
| `AZP_MarcusRun` | `TObjectPtr<UAnimSequenceBase>` | `null (runtime-loaded Marcus_M_Neutral_Run_Loop_F)` | Appearance|MarcusClips | internal (C++ only) | Native CCMH run clip for Marcus's visible body (hardcoded path at cpp:4326). |
| `AZP_MarcusCrouchIdle` | `TObjectPtr<UAnimSequenceBase>` | `null (runtime-loaded Marcus_M_Neutral_Crouch_Idle_Loop)` | Appearance|MarcusClips | internal (C++ only) | Native CCMH crouch-idle clip for Marcus's visible body (hardcoded path at cpp:4327). |
| `AZP_MarcusCrouchWalk` | `TObjectPtr<UAnimSequenceBase>` | `null (runtime-loaded Marcus_M_Neutral_Crouch_Loop_F)` | Appearance|MarcusClips | internal (C++ only) | Native CCMH crouch-walk clip for Marcus's visible body (hardcoded path at cpp:4328). |
| `AZP_MarcusBodyYaw` | `float` | `0.0f` | Appearance | EditAnywhere + BP | Extra yaw for Marcus's visible body on top of the -90 base facing (applied every frame in Tick). |
| `AZP_MarcusBodyPitch` | `float` | `0.0f` | Appearance | EditAnywhere + BP | Pitch/tilt correction for Marcus's visible body. |
| `AZP_SpineBendThresholdDeg` | `float` | `45.f` | Appearance|SpineBend | EditAnywhere + BP | Camera look-down angle below which Marcus's spine starts bending forward so the camera never sees inside the chest. |
| `AZP_SpineBendMaxDeg` | `float` | `55.f` | Appearance|SpineBend | EditAnywhere + BP | Maximum total spine bend (deg) at straight-down look; 0 disables. |
| `AZP_SpineBendInterpSpeed` | `float` | `12.f` | Appearance|SpineBend | EditAnywhere + BP | Easing speed of the spine bend toward its target each frame. |
| `AZP_SpineBendBoneLocalAxis` | `FVector` | `FVector(0.f, 1.f, 0.f)` | Appearance|SpineBend | EditAnywhere + BP | Bone-local hinge axis for the forward spine bend (flip/swap if the body leans the wrong way). |
| `AZP_ReloadCamOffset` | `FVector` | `FVector(3.0f, 0.0f, 0.0f)` | Camera | EditAnywhere + BP | Camera offset held during reload so the leaning body doesn't clip the lens. |
| `AZP_SwitchCamOffset` | `FVector` | `FVector(3.0f, 0.0f, 0.0f)` | Camera | EditAnywhere + BP | Camera offset held during weapon switch. |
| `AZP_SwingCamOffset` | `FVector` | `FVector(4.0f, 0.0f, 0.0f)` | Camera | EditAnywhere + BP | Camera offset held during a melee swing. |
| `AZP_BlockCamOffset` | `FVector` | `FVector(4.0f, 0.0f, 0.0f)` | Camera | EditAnywhere + BP | Camera offset held while blocking (moves the view, not the arms). |
| `AZP_DodgeCamOffsetMelee` | `FVector` | `FVector(15.0f, 0.0f, 0.0f)` | Camera | EditAnywhere + BP | Camera offset during the dodge clearance window while a melee weapon is up. |
| `AZP_DodgeCamOffsetRanged` | `FVector` | `FVector(0.0f, 0.0f, 0.0f)` | Camera | EditAnywhere + BP | Camera offset during the dodge clearance window while ranged. |
| `AZP_CameraPitchDownLimitDeg` | `float` | `55.f` | Camera|Pitch | EditAnywhere + BP | Max degrees the camera may pitch down; clamps both look input and animation-driven camera dives. |
| `AZP_CameraPitchUpLimitDeg` | `float` | `80.f` | Camera|Pitch | EditAnywhere + BP | Max degrees the camera may pitch up. |
| `AZP_WeaponActionOffsetSpeed` | `float` | `10.0f` | Camera | EditAnywhere + BP | Lerp speed of the per-action camera offset (higher = snappier). |
| `AZP_MeleeHandLOffset` | `FVector` | `FVector::ZeroVector` | Appearance | EditAnywhere + BP | Left-hand grip offset for the melee view-model, applied live by the post-process hand layer (UZP_MeleeHandsAnimInstance). |
| `AZP_MeleeHandROffset` | `FVector` | `FVector::ZeroVector` | Appearance | EditAnywhere + BP | Right-hand grip offset for the melee view-model. |
| `AZP_BlockHandLOffset` | `FVector` | `FVector::ZeroVector` | Appearance | EditAnywhere + BP | Optional extra left-hand grip offset added on top only while blocking. |
| `AZP_BlockHandROffset` | `FVector` | `FVector::ZeroVector` | Appearance | EditAnywhere + BP | Optional extra right-hand grip offset added on top only while blocking. |
| `AZP_MeleeViewOffset` | `FVector` | `FVector(0.0f, 0.0f, -155.0f)` | Appearance | EditAnywhere + BP | Framing position of the melee weapon-arm view-model under the camera. |
| `AZP_MeleeViewRotation` | `FRotator` | `FRotator(0.0f, -90.0f, 0.0f)` | Appearance | EditAnywhere + BP | Framing rotation of the melee view-model. |
| `AZP_MeleeViewScale` | `float` | `1.0f` | Appearance | EditAnywhere + BP | Uniform scale of the melee view-model. |
| `AZP_BlockViewOffset` | `FVector` | `FVector::ZeroVector` | Appearance | EditAnywhere + BP | Extra arm-rig placement added on top of MeleeViewOffset only while blocking (moves the arms, not the camera). |
| `AZP_FlashlightClickSound` | `TObjectPtr<USoundBase>` | `/Game/CharacterCustomizer/Components/Tools/Tool_Flashlight/Click (ctor finder)` | Flashlight | EditDefaultsOnly + BP | Sound played on flashlight toggle. |
| `AZP_FootstepData` | `TObjectPtr<UZP_FootstepData>` | `null (lazy-defaults to /Game/Core/Data/DA_Footsteps at cpp:2222)` | Footsteps | EditAnywhere + BP | The footstep surface table DataAsset (sounds + volume/pitch per surface). |
| `AZP_FootstepSounds` | `TArray<TObjectPtr<USoundBase>>` | `SW_Footstep_1..6 (Moonville, ctor loop cpp:350-359)` | Footsteps | EditDefaultsOnly + BP | Hard fallback footstep set used only when FootstepData is missing/unmatched. |
| `AZP_FootstepVolume` | `float` | `0.4f` | Footsteps | EditAnywhere + BP | Base volume of the player's own footstep foley. |
| `AZP_SprintFootstepVolumeMul` | `float` | `1.25f` | Footsteps | EditAnywhere + BP | Extra volume multiplier on sprint steps. |
| `AZP_FootstepWalkStride` | `float` | `170.f` | Footsteps | EditAnywhere + BP | Distance (UU) between footsteps while walking. |
| `AZP_FootstepSprintStride` | `float` | `150.f` | Footsteps | EditAnywhere + BP | Distance (UU) between footsteps while sprinting (must stay well under WalkStride x speed ratio). |
| `AZP_FlashlightInterpSpeed` | `float` | `8.0f` | Flashlight | EditDefaultsOnly + BP | How quickly the chest flashlight tracks the camera (lower = more chest lag). |
| `AZP_FlashlightPitchOffset` | `float` | `-5.0f` | Flashlight | EditDefaultsOnly + BP | Downward pitch offset of the flashlight from camera direction. |
| `AZP_MovementConfig` | `TObjectPtr<UZP_GraceMovementConfig>` | `/Game/Core/Data/DA_GraceMovement_Default (ctor finder cpp:366)` | Config | EditDefaultsOnly + BP | DataAsset with all movement tuning values, propagated to GameplayComp and CMC. |
| `AZP_WeaponClass` | `TSubclassOf<AActor>` | `null` | Kinemation|Weapon | EditDefaultsOnly + BP | Blueprint class of weapon to spawn, propagated to KinemationComp. |
| `AZP_GunCollisionReach` | `float` | `75.0f` | Kinemation|Weapon | EditDefaultsOnly + BP | How far the gun reaches ahead of the camera; the capsule is held this far off walls so the muzzle never clips. 0 disables. |
| `AZP_GunCollisionRadius` | `float` | `6.0f` | Kinemation|Weapon | EditDefaultsOnly + BP | Radius of the muzzle clearance probe sphere. |
| `AZP_BulletDecalMaterials` | `TArray<TSoftObjectPtr<UMaterialInterface>>` | `MI_BulletHole_Metal_01..03 (ctor cpp:461-463)` | Kinemation|Hitscan | EditDefaultsOnly + BP | Decal materials for bullet impacts, propagated to KinemationComp. |
| `AZP_LocomotionSkeletalMesh` | `TObjectPtr<USkeletalMesh>` | `null (set in BP child)` | Locomotion | EditDefaultsOnly + BP | Skeletal mesh assigned to the hidden locomotion Mesh. |
| `AZP_IdleAnimation` | `TObjectPtr<UAnimSequenceBase>` | `null (set in BP child)` | Locomotion | EditDefaultsOnly + BP | Idle clip for the hidden locomotion mesh (SingleNode speed switching). |
| `AZP_WalkAnimation` | `TObjectPtr<UAnimSequenceBase>` | `null (set in BP child)` | Locomotion | EditDefaultsOnly + BP | Walk clip for the hidden locomotion mesh. |
| `AZP_RunAnimation` | `TObjectPtr<UAnimSequenceBase>` | `null (set in BP child)` | Locomotion | EditDefaultsOnly + BP | Run clip for the hidden locomotion mesh. |
| `AZP_CrouchIdleAnimation` | `TObjectPtr<UAnimSequenceBase>` | `null (set in BP child)` | Locomotion | EditDefaultsOnly + BP | Crouch-idle clip for the hidden locomotion mesh. |
| `AZP_CrouchWalkAnimation` | `TObjectPtr<UAnimSequenceBase>` | `null (set in BP child)` | Locomotion | EditDefaultsOnly + BP | Crouch-walk clip for the hidden locomotion mesh. |
| `AZP_LadderClimbUpAnimation` | `TObjectPtr<UAnimSequenceBase>` | `null (set via _setup_ladder.py)` | Locomotion|Ladder | EditDefaultsOnly + BP | Ladder climb-up loop clip (position-scrubbed by height on the ladder). |
| `AZP_LadderClimbDownAnimation` | `TObjectPtr<UAnimSequenceBase>` | `null (set via _setup_ladder.py)` | Locomotion|Ladder | EditDefaultsOnly + BP | Ladder climb-down loop clip. |
| `AZP_LadderIdleAnimation` | `TObjectPtr<UAnimSequenceBase>` | `null (set via _setup_ladder.py)` | Locomotion|Ladder | EditDefaultsOnly + BP | Ladder idle (holding on) clip. |
| `AZP_LadderTopExitAnimation` | `TObjectPtr<UAnimSequenceBase>` | `null (A_Climb_Up_out_Right_UE5 via _setup_ladder.py)` | Locomotion|Ladder | EditDefaultsOnly + BP | Animated climb-over-the-top ladder exit clip (root motion carries the body up). |
| `AZP_LadderTopExitPlayRate` | `float` | `2.0f` | Locomotion|Ladder | EditDefaultsOnly + BP | Playback speed of the climb-over-the-top exit (anim + camera). |
| `AZP_MoveAction` | `TObjectPtr<UInputAction>` | `/Game/Core/Input/Actions/IA_Move (ctor finder)` | Input | EditDefaultsOnly + BP | Enhanced Input action for movement. |
| `AZP_LookAction` | `TObjectPtr<UInputAction>` | `/Game/Core/Input/Actions/IA_Look (ctor finder)` | Input | EditDefaultsOnly + BP | Enhanced Input action for looking. |
| `AZP_SprintAction` | `TObjectPtr<UInputAction>` | `/Game/Core/Input/Actions/IA_Sprint (ctor finder)` | Input | EditDefaultsOnly + BP | Enhanced Input action for sprint toggle. |
| `AZP_JumpAction` | `TObjectPtr<UInputAction>` | `/Game/Core/Input/Actions/IA_Jump (ctor finder)` | Input | EditDefaultsOnly + BP | Enhanced Input action for jump (repurposed as dodge). |
| `AZP_BackpedalSpeedMul` | `float` | `0.4f` | Movement | EditAnywhere + BP | Backpedal speed multiplier — backward input is scaled by this so backing away is not a free escape. |
| `AZP_DodgeImpulse` | `float` | `1200.f` | Dodge | EditDefaultsOnly + BP | Horizontal launch velocity (cm/s) applied on dodge; ~1200 = a 3m lunge with ground braking. |
| `AZP_DodgeStaminaCostPercent` | `float` | `20.f` | Dodge | EditDefaultsOnly + BP | Stamina consumed per dodge as a percent of max; dodge blocked below this. |
| `AZP_DodgeLockWindow` | `float` | `0.5f` | Dodge | EditDefaultsOnly + BP | Seconds after a dodge during which sprint and weapon swapping are locked out. |
| `AZP_DodgeCooldown` | `float` | `0.8f` | Dodge | EditDefaultsOnly + BP | Cooldown between dodges (seconds). |
| `AZP_DodgeAnim` | `TSoftObjectPtr<UAnimSequenceBase>` | `/Game/FPPMeleeAnimset/Animations/SwordnShield/FPP_sns_Dodge (ctor)` | Dodge | EditDefaultsOnly + BP | Dodge clip for the melee view-model (currently unused at runtime — dodge holds the idle grip; left wired for a future fitted clip). |
| `AZP_FallDamageMinDistance` | `float` | `300.f` | Fall Damage | EditAnywhere + BP | Drop distance (cm, peak-to-landing) below which a fall deals no damage. |
| `AZP_FallDamageMaxDistance` | `float` | `600.f` | Fall Damage | EditAnywhere + BP | Drop distance at/above which a fall deals max (lethal) damage. |
| `AZP_FallDamageMinAmount` | `float` | `30.f` | Fall Damage | EditAnywhere + BP | Damage dealt at exactly the minimum fall distance. |
| `AZP_FallDamageMaxAmount` | `float` | `100.f` | Fall Damage | EditAnywhere + BP | Damage dealt at/above the maximum fall distance. |
| `AZP_BlockDamageReductionMul` | `float` | `0.25f` | Combat|Block | EditDefaultsOnly + BP | Incoming damage multiplier while blocking (0.25 = 75% reduction). |
| `AZP_BlockHitStaminaPercent` | `float` | `33.3f` | Combat|Block | EditAnywhere + BP | Percent of max stamina a blocked hit costs; insufficient stamina = guard break at full damage. |
| `AZP_BlockMinStaminaFractionToStart` | `float` | `0.334f` | Combat|Block | EditAnywhere + BP | Minimum stamina fraction required to raise (or re-raise) the guard. |
| `AZP_BlockStaggerDuration` | `float` | `1.2f` | Combat|Stagger | EditAnywhere + BP | Seconds the attacker is staggered after a successful block (the counter window). |
| `AZP_BlockStaggerCooldown` | `float` | `1.5f` | Combat|Stagger | EditAnywhere + BP | Minimum seconds between block-staggers (safety floor against stagger spam). |
| `AZP_HitStaggerDuration` | `float` | `0.f` | Combat|Stagger | EditAnywhere + BP | Seconds an enemy is staggered by a landed melee hit; 0 = hits never stagger (intended design). |
| `AZP_HitStaggerCooldown` | `float` | `0.0f` | Combat|Stagger | EditAnywhere + BP | Minimum seconds between melee-hit staggers on the same enemy chain. |
| `bAZP_MeleeCommitEnabled` | `bool` | `true` | Combat|Melee Feel | EditAnywhere + BP | Master toggle for the committed-swing camera/lock behaviour (whip + lunge + movement/aim lock + impact kick). |
| `bAZP_MeleeLockMove` | `bool` | `true` | Combat|Melee Feel | EditAnywhere + BP | Freeze movement input during the commit window. |
| `bAZP_MeleeLockAim` | `bool` | `true` | Combat|Melee Feel | EditAnywhere + BP | Freeze look/aim input during the commit window (the swing owns the view). |
| `AZP_MeleeCommitDuration` | `float` | `0.5f` | Combat|Melee Feel | EditAnywhere + BP | Seconds the commit window (lock + camera whip) lasts. |
| `AZP_MeleeWhipYaw` | `float` | `13.f` | Combat|Melee Feel | EditAnywhere + BP | Peak yaw (deg) the view whips for a LEFT/RIGHT swing. |
| `AZP_MeleeWhipPitch` | `float` | `10.f` | Combat|Melee Feel | EditAnywhere + BP | Peak pitch (deg) the view dips for a FORWARD/overhead swing. |
| `AZP_MeleeLungeSpeed` | `float` | `250.f` | Combat|Melee Feel | EditAnywhere + BP | Forward step-in speed (cm/s) launched at swing start (collision-safe lunge toward aim). |
| `AZP_MeleeKickYaw` | `float` | `6.f` | Combat|Melee Feel | EditAnywhere + BP | Impact-kick peak yaw (deg), signed by swing direction — the directional snap on contact. |
| `AZP_MeleeKickPitch` | `float` | `7.f` | Combat|Melee Feel | EditAnywhere + BP | Impact-kick peak pitch (deg) — the view jolts up on contact (all directions). |
| `AZP_MeleeKickDuration` | `float` | `0.14f` | Combat|Melee Feel | EditAnywhere + BP | Impact-kick decay time (seconds) back to zero — sharp = small. |
| `AZP_MeleeToBlockGracePeriod` | `float` | `0.5f` | Combat|Melee Feel | EditAnywhere + BP | Grace window (seconds) at the END of a swing during which a pending block press cancels the return-to-idle tail and raises the guard. Larger = block sooner after a swing (0 = wait out the full clip). Opens the block window earlier than KinemationComp's `AZP_MeleeBlockCancelFraction`. |
| `AZP_BlockLoopAnim` | `TSoftObjectPtr<UAnimSequenceBase>` | `/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockLoop (ctor)` | Combat|Block | EditDefaultsOnly + BP | Held block pose loop (standing still). |
| `AZP_BlockWalkAnim` | `TSoftObjectPtr<UAnimSequenceBase>` | `/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockWalk (ctor)` | Combat|Block | EditDefaultsOnly + BP | Block pose while walking. |
| `AZP_BlockStartAnim` | `TSoftObjectPtr<UAnimSequenceBase>` | `/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockStart (ctor)` | Combat|Block | EditDefaultsOnly + BP | Pose-in motion before the block loop. |
| `AZP_BlockStopAnim` | `TSoftObjectPtr<UAnimSequenceBase>` | `/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockStop (ctor)` | Combat|Block | EditDefaultsOnly + BP | Pose-out motion after block release. |
| `AZP_BlockImpact1Anim` | `TSoftObjectPtr<UAnimSequenceBase>` | `/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockImpact1 (ctor)` | Combat|Block | EditDefaultsOnly + BP | Random blocked-hit reaction clip 1. |
| `AZP_BlockImpact2Anim` | `TSoftObjectPtr<UAnimSequenceBase>` | `/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockImpact2 (ctor)` | Combat|Block | EditDefaultsOnly + BP | Random blocked-hit reaction clip 2. |
| `AZP_BlockImpact3Anim` | `TSoftObjectPtr<UAnimSequenceBase>` | `/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockImpact3 (ctor)` | Combat|Block | EditDefaultsOnly + BP | Random blocked-hit reaction clip 3. |
| `AZP_MeleeIdleHoldAnim` | `TSoftObjectPtr<UAnimSequenceBase>` | `/Game/TheSignal/Animations/Melee/A_MeleePipe_Idle (ctor)` | Combat|Block | EditDefaultsOnly + BP | Return-to-hold pose after dodge/block release; MUST match Kinemation's MeleeIdleAnim (proven bug when mismatched). |
| `AZP_MashGainPerPress` | `float` | `0.12f` | Grab|Struggle | EditAnywhere + BP | Escape meter gain per LMB press during the grab struggle (~9 clean presses to escape). |
| `AZP_GrabMinMunchTime` | `float` | `0.5f` | Grab|Struggle | EditAnywhere + BP | Minimum bite beat (s) before a mash press may flip the pair to the Wrestle clips (latch-glitch fix); 0 = instant. |
| `AZP_MashDecayPerSecond` | `float` | `0.25f` | Grab|Struggle | EditAnywhere + BP | Escape meter drain per second between mash presses. |
| `AZP_StruggleTimeLimit` | `float` | `4.0f` | Grab|Struggle | EditAnywhere + BP | Seconds to reach the escape threshold before the knockdown fail state. |
| `bAZP_EscapeHoldMode` | `bool` | `false` | Grab|Struggle | EditAnywhere + BP | Accessibility: hold attack to fill the escape meter instead of tapping (TLOU/RE4R hold mode). |
| `AZP_HoldFillPerSecond` | `float` | `0.5f` | Grab|Struggle | EditAnywhere + BP | Escape meter fill per second while attack is held (Hold accessibility mode). |
| `AZP_GrabTickDamagePerSecond` | `float` | `3.125f` | Grab|Damage | EditAnywhere + BP | Ticking damage per second while held by a grab (ratio-derived from Shambler AttackDamage). |
| `AZP_GrabMinTrappedTime` | `float` | `2.f` | Grab|Struggle | EditAnywhere + BP | Minimum seconds trapped before an escape can complete, even with a full meter. |
| `AZP_FailDamageChunk` | `float` | `0.f` | Grab|Damage | EditAnywhere + BP | Extra damage chunk on struggle failure, on top of the accumulated ticks. |
| `AZP_PostEscapeGrabImmunity` | `float` | `3.0f` | Grab|Rules | EditAnywhere + BP | Seconds after breaking free during which no enemy may grab (anti-chain-grab). |
| `AZP_GrabCamBack` | `float` | `100.f` | Grab|Camera | EditAnywhere + BP | Over-the-shoulder grab camera: distance behind the player. |
| `AZP_GrabCamRight` | `float` | `55.f` | Grab|Camera | EditAnywhere + BP | Over-the-shoulder grab camera: offset to the right. |
| `AZP_GrabCamUp` | `float` | `25.f` | Grab|Camera | EditAnywhere + BP | Over-the-shoulder grab camera: offset above the head. |
| `AZP_GrabCamBlendIn` | `float` | `0.5f` | Grab|Camera | EditAnywhere + BP | 1P-to-3P camera blend seconds at grab entry (smootherstep-eased). |
| `AZP_GrabFaceBlendTime` | `float` | `0.35f` | Grab|Camera | EditAnywhere + BP | Seconds the view swings onto the grabber at the latch; 0 = old instant snap. |
| `AZP_GrabCamBlendOut` | `float` | `0.45f` | Grab|Camera | EditAnywhere + BP | 3P-to-1P camera blend seconds at grab exit. |
| `AZP_GrabVignetteHold` | `float` | `0.45f` | Grab|HUD | EditAnywhere + BP | Damage-vignette floor held while grabbed. |
| `AZP_GrabFlashlightDimMul` | `float` | `0.35f` | Grab|Flashlight | EditAnywhere + BP | Flashlight intensity multiplier while grabbed (1 = unchanged, 0 = off). |
| `AZP_GrabArmLRotation` | `FRotator` | `FRotator::ZeroRotator` | Grab|Pose | EditAnywhere + BP | Additive bone-local rotation on Marcus's left upper arm during the 3P grapple (dial out clipping). |
| `AZP_GrabArmRRotation` | `FRotator` | `FRotator::ZeroRotator` | Grab|Pose | EditAnywhere + BP | Additive bone-local rotation on Marcus's right upper arm during the 3P grapple. |
| `AZP_GrabPromptText` | `FText` | `NSLOCTEXT("TheSignal", "GrabMashPrompt", "PRESS TO BREAK FREE")` | Grab|HUD | EditAnywhere + BP | Player-facing mash prompt shown while grabbed — placeholder, dev authors the real wording in BP details. |
| `AZP_InteractAction` | `TObjectPtr<UInputAction>` | `/Game/Core/Input/Actions/IA_Interact (ctor finder)` | Input | EditDefaultsOnly + BP | Enhanced Input action for interact (E). |
| `AZP_CrouchAction` | `TObjectPtr<UInputAction>` | `/Game/Core/Input/Actions/IA_Crouch (ctor finder)` | Input | EditDefaultsOnly + BP | Enhanced Input action for crouch toggle. |
| `AZP_PeekAction` | `TObjectPtr<UInputAction>` | `/Game/Core/Input/Actions/IA_Peek (ctor finder)` | Input|Tactical | EditDefaultsOnly + BP | Enhanced Input action for lean peek (Q). |
| `AZP_AimAction` | `TObjectPtr<UInputAction>` | `/Game/Core/Input/Actions/IA_Aim (ctor finder)` | Input|Tactical | EditDefaultsOnly + BP | Enhanced Input action for ADS / block (RMB). |
| `AZP_FireAction` | `TObjectPtr<UInputAction>` | `/Game/Core/Input/Actions/IA_Fire (ctor finder)` | Input|Tactical | EditDefaultsOnly + BP | Enhanced Input action for fire / swing / grab-mash (LMB). |
| `AZP_ReloadAction` | `TObjectPtr<UInputAction>` | `/Game/Core/Input/Actions/IA_Reload (ctor finder)` | Input|Tactical | EditDefaultsOnly + BP | Enhanced Input action for reload (R). |
| `AZP_MapAction` | `TObjectPtr<UInputAction>` | `/Game/Core/Input/IA_Map (ctor finder)` | Input | EditDefaultsOnly + BP | Enhanced Input action for toggling the map (M). |
| `AZP_TabCycleLeftAction` | `TObjectPtr<UInputAction>` | `/Game/Core/Input/Actions/IA_TabCycleLeft (ctor finder)` | Input|Inventory | EditDefaultsOnly + BP | Enhanced Input action for cycling inventory tabs left. |
| `AZP_TabCycleRightAction` | `TObjectPtr<UInputAction>` | `/Game/Core/Input/Actions/IA_TabCycleRight (ctor finder)` | Input|Inventory | EditDefaultsOnly + BP | Enhanced Input action for cycling inventory tabs right. |
| `AZP_InventoryMenuAction` | `TObjectPtr<UInputAction>` | `Moonville IA_InventoryMenuOpen (ctor finder)` | Input|Inventory | EditDefaultsOnly + BP | Enhanced Input action for open/close inventory (Tab). |
| `AZP_InventorySlot0Action` | `TObjectPtr<UInputAction>` | `Moonville IA_InventorySlot0 (ctor finder)` | Input|Inventory | EditDefaultsOnly + BP | Quick-slot input action (key 1) — note runtime path actually raw-polls keys in Tick. |
| `AZP_InventorySlot1Action` | `TObjectPtr<UInputAction>` | `Moonville IA_InventorySlot1 (ctor finder)` | Input|Inventory | EditDefaultsOnly + BP | Quick-slot input action (key 2). |
| `AZP_InventorySlot2Action` | `TObjectPtr<UInputAction>` | `Moonville IA_InventorySlot2 (ctor finder)` | Input|Inventory | EditDefaultsOnly + BP | Quick-slot input action (key 3). |
| `AZP_InventorySlot3Action` | `TObjectPtr<UInputAction>` | `Moonville IA_InventorySlot3 (ctor finder)` | Input|Inventory | EditDefaultsOnly + BP | Quick-slot input action (key 4). |
| `AZP_FlashlightAction` | `TObjectPtr<UInputAction>` | `Moonville IA_InventoryFlashlight (ctor finder)` | Input | EditDefaultsOnly + BP | Enhanced Input action for flashlight toggle (F / Right Shoulder). |
| `AZP_StartingWeaponItem` | `TSoftObjectPtr<UObject>` | `/Game/Core/Items/DA_Pipe (ctor cpp:458)` | Inventory | EditDefaultsOnly + BP | PDA_Item granted to inventory at BeginPlay (starting weapon). Historically flipped pistol/pipe by the .gcs resync — verify after rename. |
| `AZP_DodgeClearanceWindow` | `float` | `0.5f` | Dodge | EditDefaultsOnly | Duration (s) the dodge holds extra forward camera clearance (covers the launch lean). |
| `AZP_BlockClearanceWindow` | `float` | `0.4f` | Combat|Block | EditDefaultsOnly | Duration (s) of the transient block forward-clearance camera nudge. |
| `AZP_GrabAnimEntry` | `TObjectPtr<UAnimSequenceBase>` | `null (lazy-loaded /Game/Marcus/GrabAnims/A_Marcus_GrabEntry at cpp:5223)` | Grab|Anims | internal (C++ only) | Marcus grab-entry clip (CCMH, MarcusBody); currently a hardcoded lazy-load path in LoadGrabAnims. |
| `AZP_GrabAnimMunch` | `TObjectPtr<UAnimSequenceBase>` | `null (lazy-loaded A_Marcus_GrabMunch at cpp:5224)` | Grab|Anims | internal (C++ only) | Marcus bite-loop grab clip. |
| `AZP_GrabAnimWrestle` | `TObjectPtr<UAnimSequenceBase>` | `null (lazy-loaded A_Marcus_GrabWrestle at cpp:5225)` | Grab|Anims | internal (C++ only) | Marcus wrestle-loop grab clip. |
| `AZP_GrabAnimKick` | `TObjectPtr<UAnimSequenceBase>` | `null (lazy-loaded A_Marcus_GrabKick at cpp:5226)` | Grab|Anims | internal (C++ only) | Marcus kick-escape grab clip. |
| `AZP_GrabAnimPush` | `TObjectPtr<UAnimSequenceBase>` | `null (lazy-loaded A_Marcus_GrabPush at cpp:5227)` | Grab|Anims | internal (C++ only) | Marcus push-escape grab clip. |
| `AZP_GrabAnimKnockdownFP` | `TObjectPtr<UAnimSequenceBase>` | `null (lazy-loaded A_Marcus_KnockdownFP at cpp:5228)` | Grab|Anims | internal (C++ only) | FP knockdown clip (hidden Mesh, camera rides it); contains the built-in rise. |
| `AZP_GrabAnimGetUpBack` | `TObjectPtr<UAnimSequenceBase>` | `null (lazy-loaded A_Marcus_GetUpBack at cpp:5229)` | Grab|Anims | internal (C++ only) | Get-up clip for the dead GetUp phase (kept in case a knockdown clip without a built-in rise ever replaces the current one). |
| `AZP_AmmoWeaponMappings` | `static const FAmmoWeaponMapping[]` | `{9mm->Pistol/Viper, Buckshot->Shotgun/Herrington/SRM, 556->Rifle/AK/TR15}` | Inventory|LootFilter | internal (C++ only) | Ammo-name-pattern to weapon-name-pattern table used to strip locker ammo the player has no weapon for; data-driven candidate. |
| `AZP_FlashlightIntensity` | `float` | `25000.0f` | Flashlight | EditAnywhere + BP | iPhone-torch beam intensity — corridor throw of the chest spotlight (dev-tuned profile). |
| `AZP_FlashlightInnerConeAngle` | `float` | `16.0f` | Flashlight | EditAnywhere + BP | Concentrated hotspot cone of the flashlight beam (distance punch). |
| `AZP_FlashlightOuterConeAngle` | `float` | `40.0f` | Flashlight | EditAnywhere + BP | Wide phone-LED spill cone for close range. |
| `AZP_FlashlightAttenuationRadius` | `float` | `5000.0f` | Flashlight | EditAnywhere + BP | Potential reach (50 m) of the flashlight beam. |
| `AZP_FlashlightFillIntensity` | `float` | `670.0f` | Flashlight | EditAnywhere + BP | Ambient fill point-light intensity simulating flashlight bounce. |
| `AZP_FlashlightFillAttenuationRadius` | `float` | `600.0f` | Flashlight | EditAnywhere + BP | Radius of the fill light's ambient coverage. |
| `AZP_AutoExposureBias` | `float` | `-0.5f` | Camera|Exposure | EditAnywhere + BP | SH2-style darkness: negative exposure bias on the player post-process (darker without crushing). |
| `AZP_AutoExposureMinBrightness` | `float` | `0.2f` | Camera|Exposure | EditAnywhere + BP | Exposure floor — lets eyes adapt to moonlit night while sealed dark rooms stay dark (session-64 tuned). |
| `AZP_AutoExposureMaxBrightness` | `float` | `1.2f` | Camera|Exposure | EditAnywhere + BP | Exposure ceiling of the player's clamped auto-exposure window. |
| `AZP_PlayerBloomIntensity` | `float` | `0.2f` | Camera|Exposure | EditAnywhere + BP | Bloom on the player post-process — kept low so darkness stays crisp. |
| `AZP_CameraForwardOffset` | `float` | `20.0f` | Camera | EditAnywhere + BP | Camera sits 20cm forward of the FPCamera socket so melee/dodge/block spine leans never put the view inside the body (dev-proven). |
| `AZP_LocoWalkSpeedThreshold` | `float` | `10.0f` | Locomotion | EditAnywhere + BP | Ground speed above which idle switches to walk clips (also cpp:1114, 1133, 1138 for the Marcus body mirror). |
| `AZP_LocoRunSpeedThreshold` | `float` | `150.0f` | Locomotion | EditAnywhere + BP | Ground speed above which walk switches to run clips (also cpp:1137). |
| `AZP_FootstepMinSpeed` | `float` | `60.f` | Footsteps | EditAnywhere + BP | Speed below which no footsteps play (standing/drifting). |
| `AZP_FootstepCrouchStrideMul` | `float` | `0.85f` | Footsteps | EditAnywhere + BP | Stride multiplier for crouch steps. |
| `AZP_FootstepCrouchVolumeMul` | `float` | `0.5f` | Footsteps | EditAnywhere + BP | Volume multiplier for crouch steps. |
| `AZP_FootstepPitchMinDefault` | `float` | `0.92f` | Footsteps | EditAnywhere + BP | Default footstep pitch-jitter floor when no surface row matches. |
| `AZP_FootstepPitchMaxDefault` | `float` | `1.08f` | Footsteps | EditAnywhere + BP | Default footstep pitch-jitter ceiling when no surface row matches. |
| `AZP_InteractTraceDistance` | `float` | `300.0f` | Interaction | EditAnywhere + BP | Crosshair line-trace reach (UU) for pickups/containers/doors on E press. |
| `AZP_DoorLOSDotThreshold` | `float` | `0.5f` | Interaction | EditAnywhere + BP | Cosine of the look-at cone (~60 deg) required for a door overlap to consume the E press. |
| `AZP_HitFlinchStrength` | `float` | `pitch -2.0, yaw +-1.5, scale = clamp(damage/25, 0.5..3.0)` | Camera|Flinch | internal (C++ only) | Camera flinch jolt on taking damage (pitch/yaw amounts and damage-scaling constants at cpp:3883-3885); one grouped tune. |
| `AZP_VignetteHealthThreshold` | `float` | `0.5f` | Health|Vignette | EditAnywhere + BP | Health fraction below which the low-health vignette starts (also the map range start at cpp:3711). |
| `AZP_VignetteMaxIntensity` | `float` | `1.5f` | Health|Vignette | EditAnywhere + BP | Vignette intensity at 0% HP. |
| `AZP_LadderRungSpacing` | `float` | `23.5f` | Locomotion|Ladder | EditAnywhere + BP | UU per ladder rung; drives climb anim scrubbing and rung snapping (repeated at cpp:1008 and cpp:5936 — arguably belongs on AZP_Ladder). |
| `AZP_LadderStandoffDist` | `float` | `75.f` | Locomotion|Ladder | EditAnywhere + BP | Distance the capsule stands off the ladder center while climbing. |
| `AZP_LadderTopExitPush` | `float` | `120.f` | Locomotion|Ladder | EditAnywhere + BP | UU pushed toward/past the ladder center onto the upper floor at top exit (also cpp:6204). |
| `AZP_LadderTopClearance` | `float` | `80.f` | Locomotion|Ladder | EditAnywhere + BP | Offset below the ladder top that caps the highest climbable position (also cpp:1009). |
| `AZP_GrabAllSweepTicks` | `int32` | `20` | Inventory|Pickup | EditAnywhere + BP | Frames the grab-the-whole-pile sweep keeps grabbing next-closest pickups after one E press (also cpp:2077). |
| `AZP_ContainerReachDistance` | `float` | `500.f` | Inventory|Container | EditAnywhere + BP | Max distance for an in-use container to count as arm's reach (guards against stale in-use flags hijacking E). |
| `AZP_MarcusBodyScale` | `float` | `0.869f` | Appearance | EditAnywhere + BP | Uniform scale of the CCMH Marcus body that drops his eyes to the FP camera height (measured 124/143). |
| `AZP_BlockWalkSpeedThreshold` | `float` | `50.f` | Combat|Block | EditAnywhere + BP | Ground speed above which the held block switches from BlockLoop to BlockWalk. |

### AZP_GrenadeProjectile
_Throwable grenade projectile spawned by UZP_KinemationComponent::ThrowProjectile(); bounces, then detonates after a fuse with two-tier radial damage, Niagara FX, and Far-carry SFX._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_FuseTime` | `float` | `5.0f` | Grenade|Explosion | EditDefaultsOnly + BP | Seconds after spawn before the grenade detonates. |
| `AZP_InnerRadius` | `float` | `200.f` | Grenade|Explosion | EditDefaultsOnly + BP | Blast inner radius in UU - targets inside take full InnerDamage. |
| `AZP_OuterRadius` | `float` | `500.f` | Grenade|Explosion | EditDefaultsOnly + BP | Blast outer radius in UU - damage falls off linearly to OuterDamage at this edge. |
| `AZP_InnerDamage` | `float` | `100.f` | Grenade|Explosion | EditDefaultsOnly + BP | Damage dealt at point blank (within InnerRadius). |
| `AZP_OuterDamage` | `float` | `50.f` | Grenade|Explosion | EditDefaultsOnly + BP | Minimum damage dealt at the outer edge of the blast radius. |
| `AZP_ExplosionFX` | `TObjectPtr<UNiagaraSystem>` | `/Game/InventorySystemPro/ExampleContent/Common/Effects/Particles/Explosion/NS_Grenade_Explosion` | Grenade|Explosion | EditDefaultsOnly + BP | Niagara explosion effect spawned at the detonation point (default loaded by path in the constructor). |
| `AZP_ExplosionSound` | `TObjectPtr<USoundBase>` | `/Game/InventorySystemPro/ExampleContent/Common/Sounds/Weapons/Explosions/SC_Grenade_Explosion` | Grenade|Explosion | EditDefaultsOnly + BP | Explosion sound cue played via UZP_SFXStatics with the Far carry profile. |
| `AZP_GrenadeThrowSpeed` | `float` | `800.f` | Grenade|Physics | EditAnywhere + BP | Grenade launch speed - same 800.f is written to both ProjectileMovement->InitialSpeed (cpp:42) and MaxSpeed (cpp:43); controls throw distance/arc feel. Promotion must still apply the value to the component in the constructor. |
| `AZP_GrenadeBounciness` | `float` | `0.3f` | Grenade|Physics | EditAnywhere + BP | How much velocity the grenade keeps on each surface bounce (ProjectileMovement->Bounciness). |
| `AZP_GrenadeFriction` | `float` | `0.5f` | Grenade|Physics | EditAnywhere + BP | Surface friction applied to the grenade while sliding after bounces (ProjectileMovement->Friction). |
| `AZP_GrenadeGravityScale` | `float` | `1.5f` | Grenade|Physics | EditAnywhere + BP | Gravity multiplier giving the grenade a heavy lobbed arc instead of a rocket trajectory (ProjectileMovement->ProjectileGravityScale). |

### AZP_InteractDoor
_Lightweight interactable door trigger (Rotate/Slide) for simple unlocked doors; moves an external DoorActor or its own built-in DoorMesh, with away-from-player swing and lock/unlock support._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_DoorActor` | `TObjectPtr<AActor>` | `null` | Door | EditInstanceOnly + BP | Optional external level actor to move as the door panel; leave empty for a self-contained door using DoorMesh. |
| `AZP_OpenMode` | `EZP_InteractDoorMode` | `EZP_InteractDoorMode::Rotate` | Door | EditAnywhere + BP | Whether the door opens by rotating on a hinge or by sliding along an offset. |
| `AZP_OpenAngle` | `float` | `90.f` | Door | EditAnywhere + BP | Yaw in degrees the door swings when opening (Rotate mode only); sign is chosen at runtime to swing away from the interactor. |
| `AZP_SlideOffset` | `FVector` | `FVector(0.f, 150.f, 0.f)` | Door | EditAnywhere + BP | Translation offset applied to the door when open (Slide mode only). |
| `AZP_InterpSpeed` | `float` | `4.f` | Door | EditAnywhere + BP | FInterpTo speed of the open/close animation - higher is snappier, lower is a slow creaking swing. |
| `bAZP_Locked` | `bool` | `false` | Door | EditAnywhere + BP | If true the door refuses interaction (shows 'Locked' prompt) until Unlock() is called by a card reader or other system. |
| `AZP_FrameCollisionSilenceRadius` | `float` | `10.f` | Door|Setup | EditAnywhere + BP | Distance (UU) within which a co-located StaticMeshActor is treated as the door's frame and gets its collision disabled at BeginPlay (BigCompany pack frames block the doorway); level-dependent, a designer may need to widen it for offset frame pivots. |
| `AZP_OpenDuration` | `float` | `0.f` | Door | EditAnywhere + BP | EXACT seconds a full open/close takes, eased. 0 = OFF (default; door keeps the normal AZP_InterpSpeed feel). Set per instance only where a specific time matters (hangar door 6-8s). |
| `AZP_OpenSound` | `TObjectPtr<USoundBase>` | `null` | Door|Audio | EditAnywhere + BP | Per-instance sound played at the door each time it STARTS opening (SFXStatics carry model; not replayed on save-load end-state). Speed of opening = existing `AZP_InterpSpeed`. |
| `AZP_ObjectiveOverride` | `FName` | `NAME_None` | Door|Objective | EditAnywhere + BP | Objective/flag id that auto-opens this door on completion (flag OR main objective; e.g. FUSE_BOX). Already-set on level load = door starts open instantly (no replay). Added 2026-07-03 fuse-box beat. |
| `AZP_RequiredItem` | `TSoftObjectPtr<UObject>` | `null` | Door\|Key | EditAnywhere + BP | Moonville PDA_Item the player must hold to open this door (e.g. DA_Security_Key). Set ⇒ door starts LOCKED; interact while holding it ⇒ unlock + open. None = no item gate. |
| `bAZP_ConsumeKeyOnUnlock` | `bool` | `false` | Door\|Key | EditAnywhere + BP | Remove one required item from inventory on unlock (default: key is kept, reusable). |
| `AZP_UnlockedMessage` | `FText` | `"Door unlocked"` | Door\|Key | EditAnywhere + BP | HUD message shown the moment the key item unlocks the door. |
| `AZP_UnlockedMessageDuration` | `float` | `2.5f` | Door\|Key | EditAnywhere + BP | Seconds door HUD messages stay on screen before auto-hiding (both AZP_UnlockedMessage and the AZP_LockedPromptText shown on a blocked attempt). |
| `bAZP_UnlockFromOtherSide` | `bool` | `false` | Door\|Key | EditAnywhere + BP | Locked door unlocks WITHOUT the required item when interacted from AZP_UnlockSide (security door that opens freely from inside). Same unlock sound + message as the key unlock. |
| `AZP_UnlockSide` | `EZP_DoorUnlockSide` | `Front` | Door\|Key | EditAnywhere + BP (EditCondition bAZP_UnlockFromOtherSide) | Which side unlocks free — Front = trigger actor's +X (red arrow), Back = −X. Flip here instead of rotating the door. |
| `AZP_InteractCooldown` | `float` | `1.f` | Door | EditAnywhere + BP | Seconds between ACCEPTED interact presses; inside the window a press does nothing (no toggle, no sounds, no messages). |
| `AZP_HandleSound` | `TObjectPtr<USoundBase>` | `SFX_Door_Handle (ctor FObjectFinder)` | Door\|Audio | EditAnywhere + BP | Handle/knob sound on EVERY accepted interact attempt (locked rattle and normal grab alike). |
| `AZP_HandleSoundCarry` | `EZP_SFXCarry` | `Close` | Door\|Audio | EditAnywhere + BP | Carry profile for the handle sound. |
| `AZP_HandleSoundVolume` | `float` | `1.f` | Door\|Audio | EditAnywhere + BP | Volume multiplier for the handle sound. |
| `AZP_UnlockSound` | `TObjectPtr<USoundBase>` | `SFX_Door_Unlock (ctor FObjectFinder)` | Door\|Audio | EditAnywhere + BP | Sound at the door when the key item unlocks it (plays alongside AZP_OpenSound as the door swings). |
| `AZP_UnlockSoundCarry` | `EZP_SFXCarry` | `Close` | Door\|Audio | EditAnywhere + BP | Carry profile for the unlock clunk. |
| `AZP_UnlockSoundVolume` | `float` | `1.f` | Door\|Audio | EditAnywhere + BP | Volume multiplier for the unlock sound. |
| `AZP_LinkedElevator` | `TObjectPtr<AZP_Elevator>` | `null` | Door\|Elevator | EditInstanceOnly + BP | Elevator whose arrival drives this door: car parks within AZP_ElevatorZMargin of the door's Z → door unlocks + opens. For landing/shaft doors, one per floor. |
| `AZP_ElevatorZMargin` | `float` | `200.f` | Door\|Elevator | EditAnywhere + BP | "Same floor" Z tolerance (UU) between the parked car's pivot and this door actor. |
| `bAZP_CloseWhenElevatorLeaves` | `bool` | `true` | Door\|Elevator | EditAnywhere + BP | Close AND re-lock the door when the linked car departs this floor (no opening onto an empty shaft); also starts the door locked when the car begins elsewhere. |
| `AZP_LockedPromptText` | `FText` | `TEXT("Locked")` | Door|UI | EditAnywhere + BP | Player-facing interaction prompt shown when the door is locked - currently hardcoded FText::FromString("Locked"), violates the project's expose-player-facing-text rule and should become an editable FText knob. |

### AZP_KeyPickup
_World-placed interactable pickup that grants a Moonville PDA_Item data asset to the player's inventory on interact, then destroys itself (key items, keycards, collectibles)._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_ItemDataAsset` | `TSoftObjectPtr<UObject>` | `null` | Pickup | EditAnywhere + BP | The Moonville PDA_Item data asset granted to the player's inventory when this pickup is used. |
| `AZP_ItemAmount` | `int32` | `1` | Pickup | EditAnywhere + BP | How many copies of the item are added to inventory per pickup. |
| `AZP_PromptText` | `FText` | `FText::FromString(TEXT("Pick Up"))` | Pickup | EditAnywhere + BP | HUD interaction prompt text shown when the player is in range of the pickup. |

### AZP_Ladder
_Climbable ladder actor: modular 5-style visual assembly (ISM rungs/rails) auto-built from LadderHeight, plus interaction trigger that hands the player off to AZP_GraceCharacter ladder movement._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_LadderStyle` | `EZP_LadderStyle` | `EZP_LadderStyle::Style1` | Ladder|Config | EditAnywhere + BP | Which of the 5 climbing-pack visual styles the modular ladder assembly uses for rungs, rails and top caps. |
| `AZP_LadderHeight` | `float` | `585.f` | Ladder|Config | EditAnywhere + BP | Total climbable ladder height in UU; drives rung/rail instance counts and auto-positions TopExitPoint and the interaction volume. |
| `AZP_ClimbSpeed` | `float` | `61.4f` | Ladder|Config | EditAnywhere + BP | Vertical climb speed in cm/s consumed by AZP_GraceCharacter's ladder movement (ZP_GraceCharacter.cpp:1038 reads Ladder->ClimbSpeed); 61.4 = 2x anim-synced speed. |
| `AZP_FootBarSpread` | `float (static constexpr, file-scope)` | `23.5f` | Ladder|Assembly | internal (C++ only) | Vertical spacing between rung (footbar) ISM instances in UU; from BP_MasterLadder and matched to pack mesh geometry, so exposing it risks rung/mesh misalignment. |
| `AZP_SideDistance` | `float (static constexpr, file-scope)` | `23.5f` | Ladder|Assembly | internal (C++ only) | Vertical spacing between mid side-rail ISM instances (and MidL/R ISM base offset) in UU; from BP_MasterLadder, matched to pack mesh geometry. |
| `AZP_MountStandoffX` | `float` | `-100.f` | Ladder|Config | EditAnywhere + BP | Local-X distance in front of the ladder face where the player mounts/exits and where the trigger centers; repeated for BottomAttachPoint (cpp:79), TopExitPoint (cpp:84, cpp:237) and InteractionVolume (cpp:90, cpp:247) — one knob keeps them in sync. |
| `AZP_InteractionPocketExtent` | `FVector (X/Y half-extents + Z padding)` | `X=60.f, Y=50.f, Z=HalfHeight+50.f (Z padding 50.f)` | Ladder|Interaction | EditAnywhere + BP | Half-extents of the front-facing mount trigger pocket (container-fix pattern preventing mounting through walls/from the side); X=60 keeps the box short of the ladder face, Y=50 stops sideways bleed, +50 Z padding above/below the climbable span; sites ZP_Ladder.cpp:89 (ctor placeholder) and ZP_Ladder.cpp:246 (authoritative). |
| `AZP_InteractionPromptText` | `FText` | `TEXT("Climb Ladder")` | Ladder|UI | EditAnywhere + BP | Player-facing HUD interaction prompt shown when in range of the ladder; per house rule player-facing text must be an exposed FText, not baked in code. |

### AZP_LockableDoor
_Lockable barrier actor (FacilitySystemsManager): starts Locked, unlocked via card reader panel, opens by yaw rotation (hinged) or translation (sliding gate) with FInterpTo._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_InitialState` | `EZP_DoorState` | `EZP_DoorState::Locked` | Door | EditAnywhere + BP | State the door starts in at BeginPlay (Locked/Closed/Opening/Open) — per-instance level setup. |
| `AZP_OpenMode` | `EZP_DoorOpenMode` | `EZP_DoorOpenMode::Rotate` | Door | EditAnywhere + BP | Whether the door opens by hinged rotation (Rotate) or by translating along SlideOffset (Slide). |
| `AZP_OpenAngle` | `float` | `90.f` | Door | EditAnywhere + BP | Target yaw delta in degrees added to the pivot's initial yaw when fully open (Rotate mode only). |
| `AZP_SlideOffset` | `FVector` | `FVector(0.f, 300.f, 0.f)` | Door | EditAnywhere + BP | Relative offset the pivot slides to when fully open (Slide mode only). |
| `AZP_OpenInterpSpeed` | `float` | `3.f` | Door | EditAnywhere + BP | FInterpTo/VInterpTo speed of the door's opening motion — higher opens faster. |

### AZP_MapPickup
_World-placed interactable pickup that unlocks the minimap for an area (calls UZP_MapComponent::DiscoverMap), optionally auto-spawning a matching AZP_MapVolume from a Dev-Tools-exported bounds file, then destroys itself._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_AreaID` | `FName` | `NAME_None (auto-generated from actor name in BeginPlay if empty)` | Map | EditAnywhere + BP | Which map area this pickup unlocks; LOCKED RULE in CLAUDE.md that it must string-match MapVolume.AreaID and generate_floor_plan.py AREA_ID or no map shows. |
| `AZP_MapTexture` | `TObjectPtr<UTexture2D>` | `null` | Map | EditAnywhere + BP | The floor-plan texture displayed by the map widget for this area; copied onto the auto-spawned MapVolume, and gates auto-volume creation (no texture = no volume). |
| `AZP_AreaDisplayName` | `FText` | `FText::FromString(TEXT("Floor Map"))` | Map | EditAnywhere + BP | Player-facing display name shown on the map widget for this area (per the 'expose player-facing text' rule, dev authors this per instance). |
| `bAZP_AutoCreateVolume` | `bool` | `true` | Map | EditAnywhere + BP | Feature toggle: auto-spawn an AZP_MapVolume covering this floor at BeginPlay; disable when placing MapVolumes by hand. |
| `AZP_MapBoundsMin` | `FVector2D` | `FVector2D::ZeroVector` | Map | EditAnywhere + BP | World-space min XY bounds used to render the map texture; set by the Dev Tools export pipeline (note: BeginPlay currently reads the map_bounds_F<N>.txt file rather than this property). |
| `AZP_MapBoundsMax` | `FVector2D` | `FVector2D::ZeroVector` | Map | EditAnywhere + BP | World-space max XY bounds used to render the map texture; set by the Dev Tools export pipeline. |
| `AZP_PromptText` | `FText` | `FText::FromString(TEXT("Pick Up Map"))` | Pickup | EditAnywhere + BP | Player-facing HUD interaction prompt shown when the player is in range of this pickup. |
| `AZP_InteractionVolumeExtent` | `FVector` | `FVector(100.f, 100.f, 80.f) (relative offset FVector(0,0,40) at ZP_MapPickup.cpp:30)` | Pickup|Interaction | EditAnywhere + BP | Box extent of the pickup's interaction/overlap volume - how close the player must be for the prompt; this project has a history of interaction volumes needing shrink/offset tuning (open-through-walls fix). |
| `AZP_GlowIntensity` | `float` | `300.f` | Pickup|Glow | EditAnywhere + BP | Point-light intensity of the pickup glow highlight (visual feel: how brightly the map pickup advertises itself in a dark horror level). |
| `AZP_GlowAttenuationRadius` | `float` | `150.f` | Pickup|Glow | EditAnywhere + BP | Attenuation radius of the pickup glow light - how far the glow spills into the level. |
| `AZP_GlowColor` | `FLinearColor` | `FLinearColor(0.4f, 0.7f, 0.9f)` | Pickup|Glow | EditAnywhere + BP | Color of the pickup glow light (currently cool blue, the project's color-code for map pickups). |
| `AZP_FloorZThresholds` | `float[4]` | `MyZ > 1400 -> F5, > 900 -> F4, > 400 -> F3, > 0 -> F2, else F1 (sites: ZP_MapPickup.cpp:65-68)` | Map|AutoVolume | internal (C++ only) | Hardcoded Z-height bands that decide which Building1 floor number this pickup belongs to when picking the map_bounds_F<N>.txt export file - level-specific and will be wrong on any other map. |
| `AZP_FallbackMapBounds` | `float[4]` | `MinX=-5000, MinY=-2000, MaxX=3000, MaxY=6000 (sites: ZP_MapPickup.cpp:93-94)` | Map|AutoVolume | internal (C++ only) | Rough whole-level XY bounds used for the auto-spawned MapVolume when no Dev Tools bounds export file is found - level-specific magic numbers. |
| `AZP_AutoVolumeHalfHeight` | `float` | `300.f` | Map|AutoVolume | EditAnywhere + BP | Z half-extent (UU) of the auto-spawned MapVolume's AreaBounds box - must cover the floor's playable height band for map display to trigger. |

### AZP_MapVolume
_Level-placed volume defining a map area's bounds; sets/clears the player's current area on overlap and carries the capture configuration for the floor-plan map texture scripts._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_AreaID` | `FName` | `NAME_None (unset)` | Map | EditAnywhere + BP | Unique area id; LOCKED contract — must match ZP_MapPickup.AreaID and the AREA_ID constant in Scripts/Python/generate_floor_plan.py or no map shows. |
| `AZP_AreaDisplayName` | `FText` | `empty` | Map | EditAnywhere + BP | Player-facing area name shown on the map widget. |
| `AZP_MapTexture` | `TObjectPtr<UTexture2D>` | `null` | Map | EditAnywhere + BP | The floor-plan texture displayed for this area; assigned by generate_floor_plan.py / capture_map.py or by hand. |
| `AZP_CaptureHeight` | `float` | `280.0f` | Map|Capture | EditAnywhere + BP | Height above the volume center for the capture camera in the photo-capture map pipeline (capture_map.py); must sit below the ceiling. |

### AZP_NPC
_Base actor for Character Customizer-powered NPCs (Interactive/Crowd/Corpse roles); implements IZP_Interactable and routes interaction into the dialogue system._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_NPCRole` | `EZP_NPCNPCRole` | `EZP_NPCNPCRole::Interactive` | NPC | EditAnywhere + BP | Selects the NPC behavior role: Interactive (dialogue), Crowd (no interaction), or Corpse (examine). |
| `AZP_DialogueData` | `TObjectPtr<UZP_DialogueData>` | `null` | NPC | EditAnywhere + BP | The dialogue data asset played when the player interacts with this NPC (ignored for Crowd role). |
| `AZP_InteractionPromptOverride` | `FText` | `FText() (empty)` | NPC | EditAnywhere + BP | Custom HUD prompt text; empty falls back to a role-derived prompt (Talk/Examine). |
| `bAZP_InteractOnce` | `bool` | `false` | NPC | EditAnywhere + BP | If true, the NPC's interaction disables after the first use (one-shot NPC). |

### AZP_ObjectiveContainer
_Interactable container/device that consumes a configured set of inventory items to unlock, setting an objective flag, opening a linked door, unlocking auto-locked nearby doors, and firing a BP OnUnlocked hook; persistence derived from the objective flag._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_RequiredItems` | `TArray<FZP_RequiredItem>` | `empty array` | ObjectiveContainer | EditAnywhere + BP | The item data assets and counts the player must hold to unlock this container; empty means never unlockable. |
| `bAZP_ConsumeItemsOnUnlock` | `bool` | `true` | ObjectiveContainer | EditAnywhere + BP | Whether the required items are removed from the player's inventory when the container unlocks. |
| `AZP_InteractionPadding` | `float` | `80.f` | ObjectiveContainer | EditAnywhere + BP | Extra half-extent in UU added around the mesh bounds when FitBoxToMeshBounds sizes the interaction trigger at BeginPlay. |
| `AZP_PromptText` | `FText` | `FText::FromString(TEXT("Use"))` | ObjectiveContainer | EditAnywhere + BP | Player-facing interaction prompt shown when in range of the locked container. |
| `AZP_MissingItemsMessage` | `FText` | `FText::FromString(TEXT("Required items missing"))` | ObjectiveContainer | EditAnywhere + BP | Player-facing HUD message shown when interacting without holding all required items. |
| `AZP_UnlockedMessage` | `FText` | `FText::FromString(TEXT("Complete"))` | ObjectiveContainer | EditAnywhere + BP | Player-facing HUD message shown when the container successfully unlocks. |
| `AZP_ObjectiveFlagOnUnlock` | `FName` | `NAME_None` | ObjectiveContainer|Objective | EditAnywhere + BP | Objective flag set on UZP_ObjectiveSubsystem at unlock; also drives free save-persistence (flag set on load = container restored as unlocked). |
| `AZP_LinkedDoor` | `TObjectPtr<AZP_LockableDoor>` | `null` | ObjectiveContainer | EditInstanceOnly + BP | Optional AZP_LockableDoor that gets unlocked and opened when the container completes (per-instance level reference). |
| `AZP_DoorLockRadius` | `float` | `0.f` | ObjectiveContainer | EditAnywhere + BP | Radius in UU within which AZP_InteractDoors are auto-locked at BeginPlay and unlocked on completion (0 disables the behavior). |
| `AZP_UnlockedLightColor` | `FLinearColor` | `FLinearColor(0.1f, 0.8f, 0.1f)` | ObjectiveContainer|Feedback | EditAnywhere + BP | Status light color when the container is unlocked (green); hardcoded identically at ZP_ObjectiveContainer.cpp:58 (save-restore path) and ZP_ObjectiveContainer.cpp:270 (Unlock path) — one knob covers both sites. Locked-red counterpart lives as the StatusLight component default (ctor .cpp:41), already editable per-instance on the component. |

### AZP_ObjectiveReactor
_Data-driven "objective complete -> world reactions" (lean slice of Docs/ObjectiveReactor_Plan.md): retints emergency lights, plays stingers; idempotent on save/load._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_ListenId` | `FName` | `NAME_None` | Reactor | EditAnywhere + BP | Progression flag OR main-objective id the reactor fires on (e.g. FUSE_BOX). |
| `AZP_StartObjectiveOnBeginPlay` | `FName` | `NAME_None` | Reactor | EditAnywhere + BP | Main objective started once at BeginPlay — per-level campaign bootstrap (e.g. RESEARCH1). Most recently started main owns the HUD tracker. |
| `AZP_StartupSound` | `TObjectPtr<USoundBase>` | `null` | Reactor|Audio | EditAnywhere + BP | Optional one-shot played 2D once at BeginPlay (scene-setting cue, e.g. C1_POWER_OUT). |
| `AZP_CompleteSound` | `TObjectPtr<USoundBase>` | `null` | Reactor|Audio | EditAnywhere + BP | One-shot played 2D when the listened id completes (e.g. C1_FUSE_BOX_COMPLETE). |
| `bAZP_RetintLights` | `bool` | `true` | Reactor|Lights | EditAnywhere + BP | Whether completion retints every matching light in the level. |
| `AZP_LightTargetColor` | `FLinearColor` | `White` | Reactor|Lights | EditAnywhere + BP | Color matched lights fade to (power restored = white). |
| `AZP_LightFadeTime` | `float` | `2.0f` | Reactor|Lights | EditAnywhere + BP | Seconds of the light fade; 0 = instant snap. |
| `AZP_MatchMinRed` | `int32` | `100` | Reactor|Lights | EditAnywhere + BP | Emergency-red match rule: light color R must be at least this. |
| `AZP_MatchDominance` | `float` | `0.6f` | Reactor|Lights | EditAnywhere + BP | ...and G,B both <= R * this ratio (0.6 catches 255/93/93 and 169/0/27). |

### AZP_ObjectiveTrigger
_Drop-in level box volume that fires one configurable action (set flag / start-complete objective or sub-objective) on the GameInstance ObjectiveSubsystem when the player first walks in._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_Action` | `EZP_ObjectiveTriggerAction` | `EZP_ObjectiveTriggerAction::SetFlag` | ObjectiveTrigger | EditAnywhere + BP | Which ObjectiveSubsystem action fires when the player enters the volume (SetFlag, CompleteSubObjective, CompleteObjective, StartObjective). |
| `AZP_TargetId` | `FName` | `NAME_None` | ObjectiveTrigger | EditAnywhere + BP | The flag / objective id / sub-objective id passed to the chosen action (e.g. 'reached_empty_floor'). Set per placed instance in the level. |
| `bAZP_OneShot` | `bool` | `true` | ObjectiveTrigger | EditAnywhere + BP | Whether the trigger fires only once and then ignores further overlaps (true) or fires on every entry (false). |
| `AZP_RequiresActiveObjective` | `FName` | `NAME_None` | ObjectiveTrigger | EditAnywhere + BP | Optional gate: the trigger only fires while this named main objective is active (None = no gate), stopping arrival triggers firing before the player is on that leg of the campaign. |

### AZP_OozeClimbPath
_The Oozeling's patrol route actor — a placeable, distinctly-named alias of AZP_ScytheerClimbPath. Introduces NO new knobs; all four (`AZP_PerPointWallNormals`, `AZP_PointsPerAdd`, `AZP_DefaultStep`, `AZP_NormalProbeDistance`) plus the Add Points To End / Snap Wall Normals To Geometry buttons are inherited — see AZP_ScytheerClimbPath._

### AZP_OozelingBase
_Oozeling (BigBlob pack) wall-crawling kamikaze slime: ping-pongs along an AZP_OozeClimbPath spline wall-climbing via per-point wall normals (MOVE_None + spline-direct drive, the Scytheer technique), random navmesh wander fallback. THE ATTACK = TOUCH BURST (v3, 2026-07-13): it doesn't swing — on contact (AZP_TouchRange, LOS-gated) it splats itself onto the player (Death2 via the normal death flow, corpse persists) and leaves a dissolving-ooze DoT (default 5% max HP/s x 5s ≈ 25%). Aggro: range + two-way LOS (+ navmesh reachability when grounded — closed doors block); routing = ceiling → Drop (free-fall, chase on landing), wall → WallDescent (sprint the spline toward its GROUND END — direction-agnostic), grounded → Chase (nav pathing with off-mesh direct-drive fallback, reach padding stripped so it closes to true contact). De-aggro walks back (ReturnToPatrol). Death: SFX at the kill moment; an elevated body free-falls and the death clip plays on landing. Whole-clip SingleNode animation (walk/run/idle/fall/hit/die), shootable + IZP_Staggerable + IZP_Revivable. Anim slots lazy-fill from /Game/BigBlob/Animations/; SFX slots lazy-fill from /Game/Audio/Oozeling/ (assets don't exist yet — silent until the dev drops audio there)._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_DetectionRange` | `float` | `1000.f` | Oozeling\|Detect | EditAnywhere + BP | Straight-line distance at which the player is considered for aggro (two-way LOS still gates it; navmesh reachability additionally gates GROUNDED aggro only — a clinging body isn't on the navmesh). |
| `AZP_LoseSightTime` | `float` | `4.f` | Oozeling\|Detect | EditAnywhere + BP | Seconds without line of sight before a chasing Oozeling de-aggros. |
| `AZP_GiveUpRange` | `float` | `2200.f` | Oozeling\|Detect | EditAnywhere + BP | Distance beyond which a chasing Oozeling abandons pursuit regardless of sight. |
| `AZP_MaxReachablePathLength` | `float` | `1600.f` | Oozeling\|Detect | EditAnywhere + BP | Max navmesh path length (UU) that counts as "same geometry" for grounded aggro — a closed door blocks the navmesh so aggro won't fire through it. |
| `AZP_LOSEyeZOffset` | `float` | `30.f` | Oozeling\|Detect | EditAnywhere + BP | Height above the actor pivot the LOS trace starts from (the Oozeling's 'eye'). |
| `AZP_LOSTargetZOffset` | `float` | `40.f` | Oozeling\|Detect | EditAnywhere + BP | Height above the target's pivot the LOS trace aims at (roughly chest height). |
| `AZP_OnWallZThreshold` | `float` | `100.f` | Oozeling\|Detect | EditAnywhere + BP | Z above the path's GROUND END (lower endpoint, found automatically — either authoring direction works) that counts as "on the wall" — routes aggro through WallDescent; half this = the descent's "reached the floor" band; also flags an elevated ReturnToPatrol goal (reroutes to the ground end). |
| `AZP_CeilingNormalZ` | `float` | `-0.35f` | Oozeling\|Detect | EditAnywhere + BP | Ceiling classification: path wall-normal Z <= this (pointing down) at the current spline distance = ceiling-clinging, so aggro DROPS instead of running the spline down. -0.35 tolerates sloped ceilings. |
| `AZP_PatrolPath` | `TObjectPtr<AZP_ScytheerClimbPath>` | `null` | Oozeling\|Patrol | EditAnywhere + BP | The climb-path spline the Oozeling ping-pongs along (assign an AZP_OozeClimbPath; any climb path type works). Unset = auto-bind (below) or random navmesh wander. |
| `bAZP_AutoBindPath` | `bool` | `true` | Oozeling\|Patrol | EditAnywhere + BP | SELF-HEAL: empty AZP_PatrolPath at BeginPlay binds the nearest climb path within AZP_AutoBindPathRadius — a C++ rebuild's reinstancing ref-wipe can no longer strand a placed Oozeling. Explicit assignment always wins; false = never auto-bind. |
| `AZP_AutoBindPathRadius` | `float` | `3000.f` | Oozeling\|Patrol | EditAnywhere + BP | Search radius (UU) for the BeginPlay auto-bind. |
| `AZP_PatrolTurnRate` | `float` | `360.f` | Oozeling\|Patrol | EditAnywhere + BP | Max rotational speed (deg/s) while patrolling — caps the endpoint 180° reversal (~0.5s at 360) so it never snaps in one frame. |
| `AZP_WanderSpeed` | `float` | `60.f` | Oozeling\|Move | EditAnywhere + BP | Crawl speed (UU/s) along the patrol spline AND for the navmesh wander fallback. |
| `AZP_ChaseSpeed` | `float` | `280.f` | Oozeling\|Move | EditAnywhere + BP | Run speed (UU/s) for the Chase state AND the WallDescent sprint down the spline. |
| `AZP_ChaseAcceptanceRadius` | `float` | `90.f` | Oozeling\|Move | EditAnywhere + BP | Chase move acceptance radius (reach padding stripped in code — the request would otherwise self-complete ~205 UU out). MUST stay below AZP_TouchRange or the chase completes outside contact and the burst never fires. Also defines the crowding exemption for the stuck watchdog (x1.2). |
| `AZP_CombatTurnRate` | `float` | `300.f` | Oozeling\|Move | EditAnywhere + BP | Max yaw speed (deg/s) tracking the player while jammed at crowding range without contact — keeps a cornered body visibly stalking instead of freezing. |
| `AZP_TouchRange` | `float` | `140.f` | Oozeling\|Attack | EditAnywhere + BP | Contact distance (UU, center-to-center) that latches the blob and lights the eruption fuse during Chase, gated on two-way LOS (no through-wall latches). Capsules physically meet at ~110 (Oozeling 55 + player 55) — default gives ~30 UU grace. Must stay above AZP_ChaseAcceptanceRadius. |
| `AZP_TouchFuseTime` | `float` | `1.0f` | Oozeling\|Attack | EditAnywhere + BP | THE touch-to-damage time: seconds between latching (contact) and the burst — the melee counterplay window. Kill it before the fuse ends = clean death, NO burst/DoT. It cannot be stagger-interrupted while erupting (only death defuses). 0 = instant eruption. |
| `AZP_BurstRadius` | `float` | `250.f` | Oozeling\|Attack | EditAnywhere + BP | Radius (UU) the victim must still be inside at DETONATION for the ooze DoT to land — dodging/sprinting out during the fuse makes it pop harmlessly (it still dies). Keep comfortably above AZP_TouchRange. |
| `AZP_TouchDamagePercentPerSecond` | `float` | `5.f` | Oozeling\|Attack | EditAnywhere + BP | Percent of the VICTIM'S max health (its UZP_HealthComponent AZP_MaxHealth) dealt per DoT tick after the burst, via the standard ApplyDamage pipeline — the player's own TakeDamage gates (death, knockdown i-frames, block) decide whether a tick lands. |
| `AZP_TouchDamageDuration` | `float` | `5.f` | Oozeling\|Attack | EditAnywhere + BP | Total seconds the dissolving-ooze DoT lasts (ticks = duration/interval, min 1; defaults 5s x 5%/tick = ~25% of the player's max health). The timer runs on the CORPSE actor (never destroyed); ReviveEnemy clears a running DoT. |
| `AZP_TouchDamageInterval` | `float` | `1.f` | Oozeling\|Attack | EditAnywhere + BP | Seconds between DoT ticks (clamped >= 0.05 in code). The first tick lands at the touch itself. |
| `AZP_ChaseStuckMoveThreshold` | `float` | `30.f` | Oozeling\|Move | EditAnywhere + BP | Movement (UU) below which the chase/return body counts as 'not moving' for stuck detection. |
| `AZP_ChaseStuckRepathTime` | `float` | `2.f` | Oozeling\|Move | EditAnywhere + BP | Seconds stuck during Chase before retrying MoveToActor with a wider acceptance radius. |
| `AZP_ChaseStuckGiveUpTime` | `float` | `4.f` | Oozeling\|Move | EditAnywhere + BP | Seconds of no movement (while NOT crowding the player) before Chase de-aggros; also the ReturnToPatrol no-progress window before it snap-rejoins the spline. |
| `AZP_WanderRadius` | `float` | `600.f` | Oozeling\|Wander | EditAnywhere + BP | Radius (UU) around the current position for random wander destinations when no patrol path is set. |
| `AZP_WanderArriveRadius` | `float` | `120.f` | Oozeling\|Wander | EditAnywhere + BP | 2D distance to the wander destination that counts as arrival, triggering the pause-then-repick cycle. |
| `AZP_PauseMin` | `float` | `1.5f` | Oozeling\|Wander | EditAnywhere + BP | Minimum idle pause (s) between wander legs. |
| `AZP_PauseMax` | `float` | `3.5f` | Oozeling\|Wander | EditAnywhere + BP | Maximum idle pause (s) between wander legs. |
| `AZP_MoveAcceptanceRadius` | `float` | `80.f` | Oozeling\|Wander | EditAnywhere + BP | Acceptance/arrival radius for wander + return-to-patrol MoveToLocation calls. |
| `AZP_FallUprightRate` | `float` | `540.f` | Oozeling\|Fall | EditAnywhere + BP | Deg/s the falling body rights itself toward upright (a wall/ceiling frame can start it sideways or inverted; 540 = full flip in ~0.33s). |
| `AZP_FallTimeout` | `float` | `5.f` | Oozeling\|Fall | EditAnywhere + BP | Safety: max seconds a fall (Drop or death fall) may last before resolving where it is (chase / corpse-finalize) — covers a body that never gets a Landed event. |
| `AZP_MaxHealth` | `float` | `100.f` | Oozeling\|Damage | EditAnywhere + BP | Max health pushed into the auto-attached UZP_HealthComponent at BeginPlay. |
| `AZP_BodyShotDamage` | `float` | `20.f` | Oozeling\|Damage | EditAnywhere + BP | Flat per-bullet damage — the blob is one uniform body, no headshot zone (5 shots = dead at defaults). |
| `AZP_HitReactCooldown` | `float` | `1.0f` | Oozeling\|Damage | EditAnywhere + BP | Min seconds between hit-react flinches so sustained fire can't perma-stunlock. |
| `AZP_WalkAnim` | `TObjectPtr<UAnimSequence>` | `null` (lazy: AS_BigBlob_Walk_FW) | Oozeling\|Anim | EditAnywhere + BP | Looping crawl clip used while moving (spline patrol + wander legs + return walk). |
| `AZP_RunAnim` | `TObjectPtr<UAnimSequence>` | `null` (lazy: AS_BigBlob_Run_FW) | Oozeling\|Anim | EditAnywhere + BP | Looping run clip for Chase and the WallDescent sprint. |
| `AZP_IdleAnim` | `TObjectPtr<UAnimSequence>` | `null` (lazy: AS_BigBlob_Idle_Breath) | Oozeling\|Anim | EditAnywhere + BP | Looping idle clip during wander pauses. |
| `AZP_FallAnim` | `TObjectPtr<UAnimSequence>` | `null` (lazy: AS_BigBlob_Jump_Loop) | Oozeling\|Anim | EditAnywhere + BP | Looping mid-air clip for the ALIVE ceiling drop (the death fall instead freezes the pose it died in). Empty = keep the walk clip. |
| `AZP_EruptAnim` | `TObjectPtr<UAnimSequence>` | BP-stamped: AS_BigBlob_Hug_Attack | Oozeling\|Anim | EditAnywhere + BP | Eruption-fuse telegraph clip, looped for long fuses (dev pick: it hugs you before it blows). Empty = keeps the idle clip. |
| `AZP_EruptAnimCutoff` | `float` | `0.f` | Oozeling\|Anim | EditAnywhere + BP | Seconds into the erupt clip to FREEZE and hold for the rest of the fuse — keeps the clip's opening (Hug_Attack's vertical rise) and cuts the flubber tail. 0 = play the full clip. Triggered on elapsed fuse time (wrap-proof); a cutoff past the clip end freezes on the final pose. |
| `AZP_EruptSound` | `TObjectPtr<USoundBase>` | `null` (lazy: /Game/Audio/Oozeling/SFX_Oozeling_Erupt) | Oozeling\|Audio | EditAnywhere + BP | One-shot when the fuse starts — the "kill it NOW" audio telegraph (Far carry). Silent until the asset exists. |
| `AZP_HitAnim` | `TObjectPtr<UAnimSequence>` | `null` (lazy: AS_BigBlob_GetHit) | Oozeling\|Anim | EditAnywhere + BP | One-shot flinch clip. Empty = no flinch (damage still applies). |
| `AZP_DieAnim` | `TObjectPtr<UAnimSequence>` | `null` (lazy: AS_BigBlob_Death2) | Oozeling\|Anim | EditAnywhere + BP | One-shot death clip, final frame held as the corpse pose — plays AFTER the body reaches the ground (death fall). Empty = freeze the pose playing at the kill. |
| `AZP_WalkPlayRate` | `float` | `1.0f` | Oozeling\|Anim | EditAnywhere + BP | Play-rate on the walk clip so the crawl cadence matches AZP_WanderSpeed without re-authoring the ~0.97s pack clip. Live-tunable in PIE. |
| `AZP_RunPlayRate` | `float` | `1.0f` | Oozeling\|Anim | EditAnywhere + BP | Play-rate on the run clip (Chase / WallDescent cadence vs AZP_ChaseSpeed). Live-tunable in PIE. |
| `AZP_AlertSound` | `TObjectPtr<USoundBase>` | `null` (lazy: /Game/Audio/Oozeling/SFX_Oozeling_Alert) | Oozeling\|Audio | EditAnywhere + BP | One-shot at the moment of aggro (Far carry). Silent until the asset exists. |
| `AZP_HitSound` | `TObjectPtr<USoundBase>` | `null` (lazy: /Game/Audio/Oozeling/SFX_Oozeling_Hit) | Oozeling\|Audio | EditAnywhere + BP | One-shot on hit flinch (Far carry). Silent until the asset exists. |
| `AZP_DeathSound` | `TObjectPtr<USoundBase>` | `null` (lazy: /Game/Audio/Oozeling/SFX_Oozeling_Death) | Oozeling\|Audio | EditAnywhere + BP | Death cry when KILLED by the player (shot/melee, incl. a fuse defuse) — fires at the kill moment (the death CLIP waits for the ground). Far carry; never replayed on load-restore. Silent until the asset exists. |
| `AZP_BurstSound` | `TObjectPtr<USoundBase>` | `null` (lazy: /Game/Audio/Oozeling/SFX_Oozeling_Burst) | Oozeling\|Audio | EditAnywhere + BP | TOUCH-DEATH sound: the burst/splat when its own eruption kills it — replaces AZP_DeathSound for that death only. Silent until the asset exists. |
| `AZP_FootstepSound` | `TObjectPtr<USoundBase>` | `null` (lazy: /Game/Audio/Oozeling/SFX_Oozeling_Squelch) | Oozeling\|Audio | EditAnywhere + BP | Movement squelch one-shot, once per AZP_FootstepStride of 2D travel (position-delta based, so it works on the teleport-driven spline patrol). |
| `AZP_FootstepVolume` | `float` | `1.f` | Oozeling\|Audio | EditAnywhere + BP | Squelch volume. 0 = silent movement. |
| `AZP_FootstepStride` | `float` | `80.f` | Oozeling\|Audio | EditAnywhere + BP | Distance (UU) of 2D travel per squelch — cadence scales with actual speed. |
| `AZP_FootstepPitchVar` | `float` | `0.08f` | Oozeling\|Audio | EditAnywhere + BP | Random pitch spread per squelch (1 ± this) so one asset never reads as a mechanical loop. |

### AZP_PlayerController
_Player controller handling Enhanced Input mapping-context setup, HUD/dialogue/map/inventory-tab widget lifecycle, pause menu toggle, dev console objective/inventory reset commands, and death fade-to-black + respawn._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_HUDWidgetClass` | `TSubclassOf<UZP_HUDWidget>` | `null (set to WBP_HUD in PC_Grace BP CDO)` | UI | EditDefaultsOnly + BP | Which widget class is spawned as the gameplay HUD on BeginPlay. |
| `AZP_DialogueWidgetClass` | `TSubclassOf<UZP_DialogueWidget>` | `null (set to WBP_DialogueBox in PC_Grace BP CDO)` | UI | EditDefaultsOnly + BP | Which widget class is spawned for dialogue subtitles/choices (added to viewport at Z 50). |
| `AZP_MapWidgetClass` | `TSubclassOf<UZP_MapWidget>` | `null (BP CDO; runtime fallback LoadClass /Game/User_Interface/WBP_Map.WBP_Map_C at ZP_PlayerController.cpp:51)` | UI | EditDefaultsOnly + BP | Which widget class is spawned as the map overlay (viewport Z 100). |
| `AZP_InventoryTabWidgetClass` | `TSubclassOf<UZP_InventoryTabWidget>` | `null (BP CDO; runtime fallback LoadClass /Game/Blueprints/UI/WBP_InventoryTab.WBP_InventoryTab_C at ZP_PlayerController.cpp:46)` | UI | EditDefaultsOnly + BP | Which widget class is spawned as the unified Tab menu (Map/Inventory/Notes, viewport Z 100). |
| `AZP_PauseAction` | `TObjectPtr<UInputAction>` | `null (set to IA_OpenPauseMenu in PC_Grace BP CDO)` | UI | EditDefaultsOnly + BP | The Enhanced Input action that toggles the pause menu. |
| `AZP_PauseMenuWidgetClass` | `TSubclassOf<UUserWidget>` | `null (set to WBP_EasyPauseMenu in PC_Grace BP CDO)` | UI | EditDefaultsOnly + BP | Which widget class is spawned as the pause menu (viewport Z 200). |
| `AZP_DefaultMappingContext` | `TObjectPtr<UInputMappingContext>` | `/Game/Core/Input/IMC_Grace (ConstructorHelpers::FObjectFinder, ZP_PlayerController.cpp:32)` | Input | EditDefaultsOnly + BP | The gameplay input mapping context applied on BeginPlay and suspended/re-armed around pause and cinematic windows. |
| `AZP_DefaultMappingPriority` | `int32` | `1` | Input | EditDefaultsOnly + BP | Priority of the default gameplay mapping context; higher wins over other contexts. |
| `AZP_LevelStartFadeInDuration` | `float` | `0.5f` | UI|Fade | EditAnywhere + BP | Seconds of camera fade from black on every level start (initial spawn and respawn reload). |
| `AZP_DeathFadeOutDuration` | `float` | `0.5f` | Respawn | EditAnywhere + BP | Seconds of the fade-to-black (with audio fade) when the pawn dies. |
| `AZP_RespawnDelay` | `float` | `1.0f` | Respawn | EditAnywhere + BP | Seconds held at black after death before the level reloads for respawn. |
| `AZP_PauseMenuButtonsToHide` | `TArray<FName>` | `{ "SaveSavegameButton", "PhotoModeBtn", "CreditsBtn" }` | UI|PauseMenu | EditAnywhere + BP | Named buttons inside the EGUI pause menu widget that get collapsed on open (unwanted menu entries). |

### AZP_SavePoint
_Computer-terminal save point actor (Save System): overlap volume shows an interaction prompt, E opens the EGUI save-manager widget via the IZP_Interactable interface._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_PromptText` | `FText` | `FText::FromString(TEXT("Save Progress"))` | SavePoint | EditDefaultsOnly + BP | Player-facing interaction prompt text shown on the HUD when in range (dev-authorable FText, per the expose-player-facing-text rule). |
| `AZP_SaveMenuWidgetClass` | `TSubclassOf<UUserWidget>` | `null (set to WBP_ESGU_SavesManagerUI in BP_SavePoint defaults)` | SavePoint | EditDefaultsOnly + BP | Widget class spawned when the player interacts — the EGUI save/load manager UI; value lives in BP_SavePoint defaults so the rename needs a CoreRedirect to survive. |

### AZP_ScytheerBase
_Ground-roaming (and spline-wall-patrolling) Scytheer enemy: wander/patrol -> alert -> chase -> 3-variant attack state machine, anim driven by frame-range slices of one AnimSequence via UAnimSingleNodeInstance (no AnimGraph)._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_DetectionRange` | `float` | `1000.f` | Scytheer|Detect | EditAnywhere + BP | Straight-line distance at which the player is considered for aggro (LOS + navmesh reachability still gate it). |
| `AZP_LoseSightTime` | `float` | `4.f` | Scytheer|Detect | EditAnywhere + BP | Seconds without line of sight before the Scytheer de-aggros. |
| `AZP_GiveUpRange` | `float` | `2200.f` | Scytheer|Detect | EditAnywhere + BP | Distance beyond which a chasing Scytheer abandons pursuit regardless of sight. |
| `AZP_MaxReachablePathLength` | `float` | `1600.f` | Scytheer|Detect | EditAnywhere + BP | Max navmesh path length (UU) that still counts as 'same geometry' for the aggro reachability gate (closed door = unreachable = no aggro). |
| `AZP_MaxHealth` | `float` | `100.f` | Scytheer|Damage | EditAnywhere + BP | Total hit points; copied into the auto-attached UZP_HealthComponent's MaxHealth at BeginPlay. |
| `AZP_BodyShotDamage` | `float` | `20.f` | Scytheer|Damage | EditAnywhere + BP | Health lost per player bullet hit below the headshot Z threshold (5 body shots = dead). |
| `AZP_HeadShotDamage` | `float` | `50.f` | Scytheer|Damage | EditAnywhere + BP | Health lost per player bullet hit at/above the headshot Z threshold. |
| `AZP_HeadshotMinZ` | `float` | `40.f` | Scytheer|Damage | EditAnywhere + BP | Hit-location height above the actor pivot at which a capsule hit counts as a headshot (capsule hits carry no bone name). |
| `AZP_AttackRange` | `float` | `200.f` | Scytheer|Attack | EditAnywhere + BP | Melee reach for the base SWIPE; within this (off AZP_SwipeCooldown) it swipes, beyond it (up to AZP_LungeRange) it lunges. Also seeds the MoveToActor acceptance radius (AttackRange - 60). |
| `AZP_AttackCooldown` | `float` | `0.7f` | Scytheer|Attack | EditAnywhere + BP | LEGACY — superseded by AZP_SwipeCooldown / AZP_LungeCooldown; kept so existing BP CDO overrides don't silently drop. No longer read by the attack logic. |
| `AZP_SwipeCooldown` | `float` | `1.0f` | Scytheer|Attack | EditAnywhere + BP | Seconds between SWIPES (the base close-range strike). |
| `AZP_LungeCooldown` | `float` | `1.8f` | Scytheer|Attack | EditAnywhere + BP | Seconds between LUNGES (the committed gap-closer). |
| `AZP_LungeRange` | `float` | `500.f` | Scytheer|Attack | EditAnywhere + BP | Max distance at which the Scytheer LUNGES. Between AZP_AttackRange and this (off cooldown + LOS) it pounces — the band that stops the player kiting/standing just out of melee. Must be > AZP_AttackRange. |
| `AZP_LungeSpeed` | `float` | `900.f` | Scytheer|Attack | EditAnywhere + BP | Forward dash speed (UU/s) during the lunge drive window; fed via AddMovementInput toward the tracked player (collision-safe, not a ballistic launch). |
| `AZP_LungeWindupTime` | `float` | `0.4f` | Scytheer|Attack | EditAnywhere + BP | WIND-UP: seconds the Scytheer coils in place (planted, facing player, pounce anim frozen on frame 1) BEFORE it dashes. The telegraph "hold before lunging"; lunge sound fires at the start of it. NOT the same as AZP_LungeDriveTime (dash duration). 0 = instant pounce. |
| `AZP_LungeDriveTime` | `float` | `0.5f` | Scytheer|Attack | EditAnywhere + BP | DASH DURATION: max seconds the body drives forward at AZP_LungeSpeed (homing) after the wind-up. Upper bound only — the dash also stops the instant it reaches melee, so bigger values look identical once closed. |
| `AZP_SwipeAttackVariant` | `int32` | `1` | Scytheer|Attack | EditAnywhere + BP | Which attack anim slice (1/2/3 -> AttackN frame range) plays for a SWIPE. Swap with AZP_LungeAttackVariant if they read backwards — no rebuild needed. |
| `AZP_LungeAttackVariant` | `int32` | `2` | Scytheer|Attack | EditAnywhere + BP | Which attack anim slice (1/2/3 -> AttackN frame range) plays for a LUNGE. Default = slice 2 (longest/most committed). |
| `AZP_AttackDamage` | `float` | `20.f` | Scytheer|Attack | EditAnywhere + BP | Damage applied to the player at the strike contact of an attack (swipe: clip midpoint; lunge: dash arrival). |
| `AZP_HitReactCooldown` | `float` | `1.0f` | Scytheer|Attack | EditAnywhere + BP | Minimum seconds between hit-react flinches so sustained fire can't perma-stunlock the Scytheer. |
| `AZP_OnWallZThreshold` | `float` | `100.f` | Scytheer|Patrol | EditAnywhere + BP | Height above the patrol spline's ground point that counts as 'on the wall', routing aggro through WallDescent instead of straight to Alert. |
| `AZP_WanderSpeed` | `float` | `60.f` | Scytheer|Move | EditAnywhere + BP | Walk speed while wandering/patrolling (also the spline-direct patrol advance rate in UU/s). |
| `AZP_ChaseSpeed` | `float` | `280.f` | Scytheer|Move | EditAnywhere + BP | Run speed while chasing the player (also the wall-descent speed along the spline). |
| `AZP_CombatTurnRate` | `float` | `300.f` | Scytheer|Move | EditAnywhere + BP | Yaw turn rate (deg/s) used to face the player during Alert/Attack/Hit states. |
| `AZP_PatrolPath` | `TObjectPtr<AZP_ScytheerClimbPath>` | `null` | Scytheer|Patrol | EditAnywhere + BP | Optional level-placed ZP_ScytheerClimbPath spline the Scytheer ping-pongs along (with wall normals) instead of random navmesh wander; assigned per instance in the level. |
| `AZP_PatrolStep` | `float` | `200.f` | Scytheer|Patrol | EditAnywhere + BP | Distance between consecutive patrol waypoints along the spline (UU) — NOTE: currently unreferenced in the .cpp; the spline-direct patrol superseded waypoint stepping. Candidate for deprecation instead of rename. |
| `AZP_PatrolTurnRate` | `float` | `360.f` | Scytheer|Patrol | EditAnywhere + BP | Max body rotation speed (deg/s) during spline patrol and wall descent so endpoint reversals don't snap 180 degrees in one frame. |
| `AZP_WanderRadius` | `float` | `600.f` | Scytheer|Wander | EditAnywhere + BP | Radius for picking random reachable navmesh wander points when no PatrolPath is set. |
| `AZP_PauseMin` | `float` | `1.5f` | Scytheer|Wander | EditAnywhere + BP | Minimum idle pause (seconds) between random wander legs. |
| `AZP_PauseMax` | `float` | `3.5f` | Scytheer|Wander | EditAnywhere + BP | Maximum idle pause (seconds) between random wander legs. |
| `AZP_AlertHoldTime` | `float` | `0.0f` | Scytheer|Alert | EditAnywhere + BP | Seconds the Scytheer holds the idle alert pose before chasing. DEFAULT 0 = no pause (fires alert SFX and runs the same frame — the Shambler wants a hold, the Scytheer does not). Raise to reinstate a stand-and-notice beat. |
| `AZP_AlertSound` | `TObjectPtr<USoundBase>` | `null (lazy-filled at BeginPlay: /Game/Audio/Scytheer/SFX_Scytheer_Alert)` | Scytheer|Audio | EditAnywhere + BP | Sound played on entering Alert or WallDescent (Far SFX carry); if left null the Scytheer placeholder asset is loaded at BeginPlay. |
| `AZP_AttackSound` | `TObjectPtr<USoundBase>` | `null (lazy-filled at BeginPlay: /Game/Audio/Scytheer/SFX_Scytheer_Attack)` | Scytheer|Audio | EditAnywhere + BP | LEGACY/fallback — now only used if AZP_SwipeSound / AZP_LungeSound is empty. Swipe and lunge each have their own sound. |
| `AZP_SwipeSound` | `TObjectPtr<USoundBase>` | `null (lazy-filled at BeginPlay: /Game/Audio/Scytheer/SFX_Scytheer_Attack)` | Scytheer|Audio | EditAnywhere + BP | Sound played when a SWIPE starts. Falls back to AZP_AttackSound if empty. |
| `AZP_LungeSound` | `TObjectPtr<USoundBase>` | `null (lazy-filled at BeginPlay: /Game/Audio/Scytheer/SFX_Scytheer_Attack)` | Scytheer|Audio | EditAnywhere + BP | Sound played when a LUNGE starts — fires at the START of the wind-up, doubling as the pounce telegraph. Falls back to AZP_AttackSound if empty. |
| `AZP_HitSound` | `TObjectPtr<USoundBase>` | `null (lazy-filled at BeginPlay: /Game/Audio/Scytheer/SFX_Scytheer_Hit)` | Scytheer|Audio | EditAnywhere + BP | Hit-confirm flinch sound (Far carry so sniping players still hear it); lazy placeholder fill if null. |
| `AZP_DeathSound` | `TObjectPtr<USoundBase>` | `null (lazy-filled at BeginPlay: /Game/Audio/Scytheer/SFX_Scytheer_Death)` | Scytheer|Audio | EditAnywhere + BP | Death cry played on entering Die (suppressed when a save-load restores a corpse); lazy placeholder fill if null. |
| `AZP_LurkSound` | `TObjectPtr<USoundBase>` | `null (lazy-filled at BeginPlay: /Game/Audio/Scytheer/SFX_Scytheer_Lurk)` | Scytheer|Audio | EditAnywhere + BP | Ambient lurk sound slot — loaded in LoadSFXDefaults but NEVER PLAYED anywhere in this class (dormant hook, possibly for future ambient loops). |
| `AZP_FootstepSound` | `TObjectPtr<USoundBase>` | `null (lazy-filled at BeginPlay: /Game/Audio/Scytheer/SFX_Scytheer_Footsteps)` | Scytheer|Audio | EditAnywhere + BP | Footstep one-shot, played once per AZP_FootstepStride of 2D travel (Room SFX carry). Distance driven from the body's own position delta so it works for navmesh gaits AND teleport-driven spline patrol; lazy placeholder fill if null. |
| `AZP_FootstepVolume` | `float` | `1.f` | Scytheer|Audio | EditAnywhere + BP | Footstep playback volume. 0 = silent feet. |
| `AZP_FootstepStride` | `float` | `80.f` | Scytheer|Audio | EditAnywhere + BP | Distance (UU) of 2D travel per footstep. Distance-based so cadence scales with actual speed (slow wander steps slowly, chase steps fast). |
| `AZP_FootstepPitchVar` | `float` | `0.08f` | Scytheer|Audio | EditAnywhere + BP | Random pitch spread per step (1 +/- this) so a single footstep asset never reads as a mechanical loop. |
| `AZP_SingleAnim` | `TObjectPtr<UAnimSequence>` | `null` | Scytheer|Anim | EditAnywhere + BP | The single source AnimSequence that all state clips (walk/run/idle/attacks/hit/die) are sliced from by frame range; set in the child BP. |
| `AZP_WalkStartFrame` | `int32` | `1` | Scytheer|Anim | EditAnywhere + BP | First frame of the walk loop slice in SingleAnim. |
| `AZP_WalkEndFrame` | `int32` | `33` | Scytheer|Anim | EditAnywhere + BP | Last frame of the walk loop slice in SingleAnim. |
| `AZP_RunStartFrame` | `int32` | `35` | Scytheer|Anim | EditAnywhere + BP | First frame of the run loop slice (Chase/WallDescent) in SingleAnim. |
| `AZP_RunEndFrame` | `int32` | `53` | Scytheer|Anim | EditAnywhere + BP | Last frame of the run loop slice in SingleAnim. |
| `AZP_IdleStartFrame` | `int32` | `55` | Scytheer|Anim | EditAnywhere + BP | First frame of the idle loop slice (Alert state) in SingleAnim. |
| `AZP_IdleEndFrame` | `int32` | `190` | Scytheer|Anim | EditAnywhere + BP | Last frame of the idle loop slice in SingleAnim. |
| `AZP_Attack1StartFrame` | `int32` | `192` | Scytheer|Anim | EditAnywhere + BP | First frame of attack variant 1 slice in SingleAnim. |
| `AZP_Attack1EndFrame` | `int32` | `222` | Scytheer|Anim | EditAnywhere + BP | Last frame of attack variant 1 slice in SingleAnim. |
| `AZP_Attack2StartFrame` | `int32` | `223` | Scytheer|Anim | EditAnywhere + BP | First frame of attack variant 2 slice in SingleAnim. |
| `AZP_Attack2EndFrame` | `int32` | `288` | Scytheer|Anim | EditAnywhere + BP | Last frame of attack variant 2 slice in SingleAnim. |
| `AZP_Attack3StartFrame` | `int32` | `291` | Scytheer|Anim | EditAnywhere + BP | First frame of attack variant 3 slice in SingleAnim. |
| `AZP_Attack3EndFrame` | `int32` | `321` | Scytheer|Anim | EditAnywhere + BP | Last frame of attack variant 3 slice in SingleAnim. |
| `AZP_HitStartFrame` | `int32` | `323` | Scytheer|Anim | EditAnywhere + BP | First frame of the hit-flinch slice in SingleAnim. |
| `AZP_HitEndFrame` | `int32` | `353` | Scytheer|Anim | EditAnywhere + BP | Last frame of the hit-flinch slice in SingleAnim. |
| `AZP_DieStartFrame` | `int32` | `354` | Scytheer|Anim | EditAnywhere + BP | First frame of the death slice in SingleAnim. |
| `AZP_DieEndFrame` | `int32` | `500` | Scytheer|Anim | EditAnywhere + BP | Last frame of the death slice; ALSO anchors SecondsPerFrame (ClipLen / DieEndFrame), so it must equal the clip's true final frame for all other markers to map correctly. |
| `AZP_AttackHitRangeMultiplier` | `float` | `1.4f` | Scytheer|Attack | EditAnywhere + BP | Grace multiplier on AttackRange at the swing's damage midpoint — the player must be within AttackRange * this (and visible) for the swipe to connect. |
| `AZP_ChaseStuckGiveUpTime` | `float` | `4.f` | Scytheer|Move | EditAnywhere + BP | Seconds of not moving during Chase before the Scytheer de-aggros and returns to patrol/wander (anti door-jam watchdog). |
| `AZP_ChaseStuckRepathTime` | `float` | `2.f` | Scytheer|Move | EditAnywhere + BP | Seconds stuck during Chase before retrying MoveToActor with a wider acceptance radius (the 2.5f re-hold at cpp:432 is mechanism, not a knob). |
| `AZP_ChaseStuckMoveThreshold` | `float` | `30.f` | Scytheer|Move | EditAnywhere + BP | Movement (UU) below which the chase body counts as 'not moving' for stuck detection (compared squared at cpp:405). |
| `AZP_ChaseAcceptanceOffset` | `float` | `60.f` | Scytheer|Move | EditAnywhere + BP | Subtracted from AttackRange to form the MoveToActor acceptance radius so the body closes inside swing distance; same 60.f used at cpp:431 (relaxed retry, floor 200) and cpp:437 (normal, floor 40). |
| `AZP_WanderArriveRadius` | `float` | `120.f` | Scytheer|Wander | EditAnywhere + BP | 2D distance to the random wander destination that counts as arrival, triggering the pause-then-repick cycle. |
| `AZP_MoveAcceptanceRadius` | `float` | `80.f` | Scytheer|Move | EditAnywhere + BP | Acceptance/arrival radius for non-chase MoveToLocation calls — ReturnToPatrol arrival + goal (cpp:479, cpp:486, cpp:607) and wander-point moves (cpp:766). |
| `AZP_LOSEyeZOffset` | `float` | `30.f` | Scytheer|Detect | EditAnywhere + BP | Height above the actor pivot the LOS trace starts from (the Scytheer's 'eye'); repeated in the aggro debug trace at cpp:246. |
| `AZP_LOSTargetZOffset` | `float` | `40.f` | Scytheer|Detect | EditAnywhere + BP | Height above the target's pivot the LOS trace aims at (roughly chest height on the player); repeated in the aggro debug trace at cpp:247. |

### AZP_ScytheerClimbPath
_Spline-backed level actor defining a Scytheer climb route; each spline point carries a wall normal (head direction) lerped along the path, with editor buttons to extend the path and snap normals to geometry._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_PerPointWallNormals` | `TArray<FVector>` | `empty (auto-resized to spline point count, padded with FVector::UpVector)` | Path|Scytheer | EditAnywhere + BP | Per-spline-point world-space surface normal (the Scytheer's head/up direction) that the creature lerps between while traveling the path. |
| `AZP_PointsPerAdd` | `int32` | `1` | Path|Authoring | EditAnywhere | How many spline points a single 'Add Points To End' editor button click appends to the path (WITH_EDITORONLY_DATA authoring knob). |
| `AZP_DefaultStep` | `float` | `200.f` | Path|Authoring | EditAnywhere | Spacing (UU) used for newly appended spline points when the path has fewer than 2 points to extrapolate a direction from (WITH_EDITORONLY_DATA authoring knob). |
| `AZP_NormalProbeDistance` | `float` | `100.f` | Path|Authoring | EditAnywhere | How far (UU) the 'Snap Wall Normals To Geometry' editor button line-traces around each spline point to find the surface that point rests on (WITH_EDITORONLY_DATA authoring knob). |

### AZP_TransitLocation
_Placeable marker actor for an in-map elevator stop (floor); AZP_TransitPanel destinations reference it to derive travel Z and hide the current floor._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_LocationId` | `FName` | `NAME_None` | Transit | EditAnywhere + BP | Stable identifier for this elevator stop (e.g. Floor1), used for logs/debugging and to identify the stop. |
| `DisplayName` | `FText` | `(empty FText)` | Transit | EditAnywhere + BP | Optional human-readable floor label for this stop (the transit destination row keeps its own DisplayName). *(name kept: struct member / JSON-save mapped)* |

### AZP_TransitPanel
_Placeable elevator/tram console actor: player overlaps and presses E to open a floor-selection widget, then travels via UZP_TransitSubsystem or moves a linked in-map AZP_Elevator car._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_Destinations` | `TArray<FZP_TransitDestination>` | `empty array` | Transit | EditAnywhere + BP | Per-placement list of transit destinations (floors/areas) this panel offers in its menu. |
| `AZP_LinkedElevator` | `TObjectPtr<AZP_Elevator>` | `null` | Transit | EditInstanceOnly + BP | The elevator car this console rides and controls; required for InMapElevator destinations, panel auto-attaches to it at BeginPlay. |
| `AZP_TransitMenuWidgetClass` | `TSubclassOf<UUserWidget>` | `UZP_TransitMenuWidget::StaticClass() (set in ctor, ZP_TransitPanel.cpp:39)` | Transit | EditAnywhere + BP | The floor-selection widget class to spawn on interact; must extend UZP_TransitMenuWidget. |
| `AZP_PromptText` | `FText` | `FText::FromString(TEXT("Use Elevator"))` | Transit | EditAnywhere + BP | Player-facing interaction prompt shown by the HUD when standing at the panel (dev-authored text per house rule). |
| `AZP_CurrentFloorTolerance` | `float` | `20.f` | Transit | EditAnywhere + BP | Distance in UU within which the elevator pivot counts as already parked at a destination floor, hiding that floor from the menu. |

### AZP_TransitReturn
_Wall-mounted elevator call button: on E-press it recalls the linked AZP_Elevator car to this floor, resolving the target Z from an explicit or auto-found nearest AZP_TransitLocation marker._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_LinkedElevator` | `TObjectPtr<AZP_Elevator>` | `null` | Transit | EditInstanceOnly + BP | The elevator car this button recalls; required per-instance level reference. |
| `AZP_ReturnLocation` | `TObjectPtr<AZP_TransitLocation>` | `null` | Transit | EditInstanceOnly + BP | Explicit stop marker for this floor (target Z the car is recalled to); unset means auto-find nearest at BeginPlay. |
| `bAZP_AutoFindNearestLocation` | `bool` | `true` | Transit | EditAnywhere + BP | When ReturnLocation is unset, find and cache the nearest AZP_TransitLocation at BeginPlay. |
| `AZP_PromptText` | `FText` | `FText::FromString(TEXT("Call Elevator"))` | Transit | EditAnywhere + BP | Interaction prompt text for the call button (note: OnOverlapBegin intentionally shows no HUD prompt, so this only surfaces via GetInteractionPrompt). |
| `AZP_UnpoweredPromptText` | `FText` | `"No Power" (PLACEHOLDER — author per level)` | Transit | EditAnywhere + BP | Prompt returned while the AZP_RequiredObjective gate is unmet. |
| `AZP_RequiredObjective` | `FName` | `NAME_None` | Transit\|Objective | EditAnywhere + BP | Objective/flag id that powers the button (matches flag OR completed main objective — e.g. a fuse-box BP_ObjectiveContainer's unlock flag). Unmet = button dead + IndicatorLight off. NAME_None = always powered. |
| `AZP_ReadyLightColor` | `FLinearColor` | `White` | Transit\|Light | EditAnywhere + BP | IndicatorLight idle color while the elevator works (dim white). |
| `AZP_ArrivedLightColor` | `FLinearColor` | `(0.1, 0.8, 0.1)` | Transit\|Light | EditAnywhere + BP | IndicatorLight color while the car is parked at this floor (back to ready color on departure). |
| `AZP_ArriveZMargin` | `float` | `100.f` | Transit\|Light | EditAnywhere + BP | Z tolerance (UU) between car pivot and this floor's stop marker that counts as "car is at this floor". |
| `AZP_PressSound` | `TObjectPtr<USoundBase>` | `SFX_Elevator_Beep (ctor FObjectFinder)` | Transit\|Audio | EditAnywhere + BP | Acknowledge beep played at the button on a successful (powered) press. |
| `AZP_PressSoundCarry` | `EZP_SFXCarry` | `Close` | Transit\|Audio | EditAnywhere + BP | Carry profile for the press beep. |
| `AZP_PressSoundVolume` | `float` | `1.f` | Transit\|Audio | EditAnywhere + BP | Volume multiplier for the press beep. |

### AZP_VentDoor
_One-shot, key-gated vent/barricade panel: player needs RequiredItemDA in Moonville inventory to open; mesh tilts around a top-edge hinge, then collision and interaction are disabled permanently._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_RequiredItemDA` | `TSoftObjectPtr<UObject>` | `null` | Vent | EditAnywhere + BP | The Moonville PDA_Item data asset the player must carry to open this vent. |
| `AZP_RequiredItemName` | `FText` | `FText::FromString(TEXT("Screwdriver"))` | Vent | EditAnywhere + BP | Display name of the required item shown in HUD prompts. |
| `AZP_LockedPrompt` | `FText` | `FText::FromString(TEXT("Needs something to unscrew this..."))` | Vent | EditAnywhere + BP | Player-facing prompt shown when the player lacks the required item. |
| `AZP_UnlockedPromptFormat` | `FText` | `FText::FromString(TEXT("Use {0} to open it up"))` | Vent | EditAnywhere + BP | Player-facing prompt format shown when the player has the item; {0} is RequiredItemName. |
| `AZP_HingeOffset` | `FVector` | `FVector(76.5f, 0.f, 0.f)` | Vent | EditAnywhere + BP | Hinge pivot location in actor-local space; the vent mesh is counter-offset so its visual position stays at actor origin when closed. |
| `AZP_OpenRotation` | `FRotator` | `FRotator(-90.f, 0.f, 0.f)` | Vent | EditAnywhere + BP | Relative rotation applied to the hinge pivot when the vent is fully open (how far the panel tilts outward). |
| `AZP_OpenDuration` | `float` | `0.7f` | Vent | EditAnywhere + BP | Seconds the open animation takes to tilt the vent from closed to fully open. |
| `AZP_InteractionVolumeExtent` | `FVector` | `FVector(100.f, 100.f, 100.f)` | Vent | EditAnywhere + BP | Half-extents of the interaction box the player must enter to get the interact prompt. |
| `AZP_InteractionVolumeOffset` | `FVector` | `FVector(0.f, 0.f, 0.f)` | Vent | EditAnywhere + BP | Relative offset of the interaction box from the actor origin. |
| `bAZP_ConsumeItemOnUse` | `bool` | `false` | Vent | EditAnywhere + BP | If true, one unit of the required item is removed from inventory on use (default false: reusable tool like a screwdriver). |

### SM_Surface
_Procedural flat slab (floor/roof/ceiling) actor (`ASM_Surface`, BP child `/Game/Core/Actors/BP_Surface`, added 2026-07-12): flat material slab OR natural-size mesh-tile grid (set TileISM's Static Mesh), cutting real holes (visibility + collision) around assigned actors and auto-rebuilding in-editor. The actor NEVER writes materials — set them on the components' ordinary Materials slots. Full guide: `Docs/BP_Surface_Guide.md`. REMOVED same day (do not look for them): `AZP_Material`, `bAZP_AutoTileFromMesh`, `bAZP_ScaleEdgeTiles`._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_SurfaceSize` | `FVector2D` | `(1000, 1000)` | Surface | EditAnywhere + BP | Total surface footprint in uu (local X/Y), centered on the actor. Mesh-tile mode always covers this whole rectangle — partial tiles overhang, never scale. |
| `AZP_Thickness` | `float` | `10.f` | Surface | EditAnywhere + BP | Flat-slab mode thickness in uu; the TOP face sits at the actor's Z. |
| `AZP_CutActors` | `TArray<TObjectPtr<AActor>>` | empty | Surface\|Cuts | EditAnywhere + BP | Actors whose bounds footprints are cut out of the surface as real holes (visibility + collision). Slab mode: border-accurate hole; mesh mode: whole-tile knockout. |
| `AZP_CutMargin` | `float` | `0.f` | Surface\|Cuts | EditAnywhere + BP | Extra margin (uu) around each cut actor's bounds; negative shrinks the hole (clamped so thin actors never invert). |
| `AZP_TileSize` | `FVector2D` | `(450, 450)` | Surface\|Material | EditAnywhere + BP | Flat-slab mode only: world size (uu) of ONE material pattern repeat (UV tiling). Mesh-tile mode ignores it — the pitch is always the mesh's own footprint. |
| `bAZP_GenerateCollision` | `bool` | `true` | Surface | EditAnywhere + BP | Generate blocking collision for the slab (holes have none). |
| `AZP_FollowInterval` | `float` | `0.5f` | Surface\|Cuts | EditAnywhere + BP | Seconds between editor checks for moved/resized cut actors (live-follow rebuild cadence). |

### UZP_WarmupGateSubsystem
_Loading-screen warm-up gate (added 2026-07-12): world subsystem that holds the EasyGameUI loading screen (`WBP_ESGU_LoadingScreen` via `BP_EasySaveGameOperationsManager`, reflection-only) at every gameplay-map start — game paused — until the ~85-asset lazy-load manifest, the async loader, pending shader compiles and the PSO precache queue (cooked builds only; engine disables PSO precache in PIE) are all drained. **Deliberate exception to the AZP_ UPROPERTY convention:** a subsystem has no editor Details panel, so its knobs are CONSOLE VARIABLES (tunable live in PIE, no rebuild). Log tag: `[WarmupGate]`._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `zp.WarmupGate.Enabled` | `int32` | `1` | WarmupGate | console variable | Master switch: 1 = hold the loading screen at world start until warm; 0 = gate off entirely. |
| `zp.WarmupGate.MinShowSec` | `float` | `1.5` | WarmupGate | console variable | Minimum seconds the screen stays up even if already warm (anti-strobe). |
| `zp.WarmupGate.TimeoutSec` | `float` | `45.0` | WarmupGate | console variable | Hard release cap; on timeout logs which drain condition never finished. |
| `zp.WarmupGate.Pause` | `int32` | `1` | WarmupGate | console variable | 1 = pause the world while holding (enemies frozen, no damage; loads/PSOs still progress on engine threads). |
| `zp.WarmupGate.WaitForShaders` | `int32` | `1` | WarmupGate | console variable | 1 = also wait for GShaderCompilingManager to go idle (editor on-demand compiles); bounded by TimeoutSec. |

### FZP_DialogueChoice
_A single player choice presented during dialogue; can fire a narrative beat and/or jump to another dialogue._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `ChoiceText` | `FText` |  | Dialogue | EditAnywhere + BP | Player-facing text shown on the choice button. *(name kept: struct member / JSON-save mapped)* |
| `NextDialogueID` | `FName` | `NAME_None` | Dialogue | EditAnywhere + BP | Dialogue asset to jump to after this choice is selected; None continues the current sequence. *(name kept: struct member / JSON-save mapped)* |
| `RequiredBeatID` | `FName` | `NAME_None` | Dialogue | EditAnywhere + BP | Narrative beat that must already be triggered for this choice to be visible. *(name kept: struct member / JSON-save mapped)* |
| `TriggerBeatID` | `FName` | `NAME_None` | Dialogue | EditAnywhere + BP | Narrative beat fired when this choice is chosen. *(name kept: struct member / JSON-save mapped)* |

### FZP_DialogueLine
_A single line of dialogue: speaker, subtitle, optional audio, timing, optional choices._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `Speaker` | `FName` | `NAME_None` | Dialogue | EditAnywhere + BP | Speaker identifier displayed with the subtitle line. *(name kept: struct member / JSON-save mapped)* |
| `SubtitleText` | `FText` |  | Dialogue | EditAnywhere + BP | Subtitle text displayed to the player for this line. *(name kept: struct member / JSON-save mapped)* |
| `AudioAsset` | `TObjectPtr<USoundBase>` | `null` | Dialogue | EditAnywhere + BP | Voice line audio for this line; null means text-only. *(name kept: struct member / JSON-save mapped)* |
| `Duration` | `float` | `0.f` | Dialogue | EditAnywhere + BP | Line duration in seconds; 0 auto-derives from the audio length. *(name kept: struct member / JSON-save mapped)* |
| `PostDelay` | `float` | `0.3f` | Dialogue | EditAnywhere + BP | Pause in seconds after this line before advancing to the next. *(name kept: struct member / JSON-save mapped)* |
| `NarrativeBeatID` | `FName` | `NAME_None` | Dialogue | EditAnywhere + BP | Narrative beat fired when this line starts playing. *(name kept: struct member / JSON-save mapped)* |
| `Choices` | `TArray<FZP_DialogueChoice>` | `empty` | Dialogue | EditAnywhere + BP | Player choices for this line; empty means linear auto-advance. *(name kept: struct member / JSON-save mapped)* |

### FZP_FootstepSurface
_One row of the footstep surface table: match rules (SurfaceType or material-name keywords) plus the sound set and volume/pitch character for that surface._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `Name` | `FString` | `""` | Surface | EditAnywhere + BP | Readability-only label for the row in the asset editor. *(name kept: struct member / JSON-save mapped)* |
| `SurfaceType` | `TEnumAsByte<EPhysicalSurface>` | `SurfaceType_Default` | Surface | EditAnywhere + BP | Physical-material surface type this row matches (preferred, exact match path). *(name kept: struct member / JSON-save mapped)* |
| `MaterialNameKeywords` | `TArray<FString>` | `empty` | Surface | EditAnywhere + BP | Case-insensitive substrings of floor material names this row matches (zero-authoring path for purchased packs). *(name kept: struct member / JSON-save mapped)* |
| `Sounds` | `TArray<TObjectPtr<USoundBase>>` | `empty` | Surface | EditAnywhere + BP | Step sound variations for this surface, one picked at random per step; empty makes the row a placeholder. *(name kept: struct member / JSON-save mapped)* |
| `VolumeMul` | `float` | `1.f` | Surface | EditAnywhere + BP | Volume multiplier applied to steps on this surface. *(name kept: struct member / JSON-save mapped)* |
| `PitchMin` | `float` | `0.92f` | Surface | EditAnywhere + BP | Lower bound of the random pitch variation per step on this surface. *(name kept: struct member / JSON-save mapped)* |
| `PitchMax` | `float` | `1.08f` | Surface | EditAnywhere + BP | Upper bound of the random pitch variation per step on this surface. *(name kept: struct member / JSON-save mapped)* |

### FZP_NoteEntry
_Struct describing one collected note/document (ID, title, content, optional safe-combination code) authored on note pickups and stored in the note subsystem and save game._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `NoteID` | `FName` | `NAME_None` | Note | EditAnywhere + BP | Unique note identifier used for deduplication and save/load. *(name kept: struct member / JSON-save mapped)* |
| `Title` | `FText` | `(empty FText)` | Note | EditAnywhere + BP | Display title shown in the Notes tab list (player-facing text, dev-authored). *(name kept: struct member / JSON-save mapped)* |
| `Content` | `FText` | `(empty FText)` | Note | EditAnywhere + BP | Full note body shown in the detail view (player-facing text, dev-authored). *(name kept: struct member / JSON-save mapped)* |
| `bIsCode` | `bool` | `false` | Note | EditAnywhere + BP | Marks this note as containing a combination-lock code (feeds the safe system, TICKET-057). *(name kept: struct member / JSON-save mapped)* |
| `CodeValue` | `FString` | `(empty FString)` | Note | EditAnywhere + BP | The literal combination code string (e.g. 4281); only meaningful when bIsCode is true. *(name kept: struct member / JSON-save mapped)* |

### FZP_ObjectiveDef
_A main campaign objective — one DT_Objectives DataTable row (FTableRowBase) authored in SourceData/Objectives.json: id, title, sequential gating, side/auto-start flags, sub-objectives._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `Id` | `FName` | `NAME_None` | Objectives | EditAnywhere + BP | Canonical stable objective id — the key used everywhere (Requires gates, enemy revival, save data), decoupled from the table row name. *(name kept: struct member / JSON-save mapped)* |
| `Title` | `FText` | `empty` | Objectives | EditAnywhere + BP | Player-facing main objective title shown in the HUD tracker. *(name kept: struct member / JSON-save mapped)* |
| `Requires` | `TArray<FName>` | `empty` | Objectives | EditAnywhere + BP | Prior main-objective ids that must be complete before this one can start (sequential campaign gate). *(name kept: struct member / JSON-save mapped)* |
| `bSideObjective` | `bool` | `false` | Objectives | EditAnywhere + BP | Marks the objective as optional/side; the HUD tracker shows main objectives only. *(name kept: struct member / JSON-save mapped)* |
| `bStartOnLoad` | `bool` | `false` | Objectives | EditAnywhere + BP | Auto-starts this objective at game start. *(name kept: struct member / JSON-save mapped)* |
| `SubObjectives` | `TArray<FZP_SubObjectiveDef>` | `empty` | Objectives | EditAnywhere + BP | The ordered sub-objectives contained by this main objective. *(name kept: struct member / JSON-save mapped)* |

### FZP_ObjectiveRequirement
_One requirement on a sub-objective/stage: a typed condition (flag/item/dialogue/note/trigger) plus a string value._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `Type` | `EZP_ObjReqType` | `EZP_ObjReqType::FlagSet` | Objectives | EditAnywhere + BP | Which kind of condition this requirement checks (flag set, objective complete, has item, heard dialogue, collected note, reached trigger). *(name kept: struct member / JSON-save mapped)* |
| `Value` | `FString` | `empty` | Objectives | EditAnywhere + BP | The id/path the requirement checks against; interpretation depends on Type (flag name, item DataAsset path, dialogue id, note id, trigger id). *(name kept: struct member / JSON-save mapped)* |

### FZP_ObjectiveStage
_One stage of a multi-stage sub-objective: current-stage title, its requirements, and an optional flag set on stage entry._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `Title` | `FText` | `empty` | Objectives | EditAnywhere + BP | Player-facing tracker title while this stage is current. *(name kept: struct member / JSON-save mapped)* |
| `Requirements` | `TArray<FZP_ObjectiveRequirement>` | `empty` | Objectives | EditAnywhere + BP | Conditions that must all be met to advance past this stage. *(name kept: struct member / JSON-save mapped)* |
| `EnterFlag` | `FName` | `NAME_None` | Objectives | EditAnywhere + BP | Progression flag set the moment the objective enters this stage (None = no flag); used to unlock other systems like transit floors, auto-persists. *(name kept: struct member / JSON-save mapped)* |

### FZP_RequiredItem
_USTRUCT pairing a Moonville PDA_Item data asset with the count the player must hold; array element type for AZP_ObjectiveContainer.RequiredItems._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `Item` | `TSoftObjectPtr<UObject>` | `null` | ObjectiveContainer | EditAnywhere + BP | The Moonville PDA_Item data asset (e.g. DA_Fuse) the player must hold for this requirement. *(name kept: struct member / JSON-save mapped)* |
| `Count` | `int32` | `1` | ObjectiveContainer | EditAnywhere + BP | How many of this item the player must hold (e.g. 3 fuses) to satisfy the requirement. *(name kept: struct member / JSON-save mapped)* |

### FZP_SubObjectiveDef
_One sub-objective of a main objective: id, stage-0 title/requirements, optional reveal gating, and extra stages._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `Id` | `FName` | `NAME_None` | Objectives | EditAnywhere + BP | Stable sub-objective id referenced by SubObjectiveComplete requirements and enemy ReviveOnObjective fields. *(name kept: struct member / JSON-save mapped)* |
| `Title` | `FText` | `empty` | Objectives | EditAnywhere + BP | Stage-0 (or only) player-facing tracker title for this sub-objective. *(name kept: struct member / JSON-save mapped)* |
| `Requirements` | `TArray<FZP_ObjectiveRequirement>` | `empty` | Objectives | EditAnywhere + BP | Stage-0 requirements; completing them finishes the sub (or advances to Stages[0] if extra stages exist). *(name kept: struct member / JSON-save mapped)* |
| `RevealRequirements` | `TArray<FZP_ObjectiveRequirement>` | `empty` | Objectives | EditAnywhere + BP | Empty = sub visible as soon as the parent objective is active; non-empty = sub stays hidden until all are met, then pops into the tracker. *(name kept: struct member / JSON-save mapped)* |
| `Stages` | `TArray<FZP_ObjectiveStage>` | `empty` | Objectives | EditAnywhere + BP | Extra stages after stage 0; the sub completes when the last stage's requirements are met and re-titles itself per current stage. *(name kept: struct member / JSON-save mapped)* |

### FZP_TransitDestination
_Authoring struct for one travel destination offered by a transit panel (elevator/tram/building entry), including its gating data._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `DestinationId` | `FName` | `NAME_None` | Transit | EditAnywhere + BP | Stable id for the destination (e.g. Building1.Floor3) used by gating and menu logic. *(name kept: struct member / JSON-save mapped)* |
| `DisplayName` | `FText` |  | Transit | not editor-editable | Player-facing label shown in the transit menu for this destination. *(name kept: struct member / JSON-save mapped)* |
| `DestType` | `EZP_TransitDestType` | `EZP_TransitDestType::LoadLevel` | Transit | EditAnywhere + BP | Whether this destination loads another map or rides an in-map elevator. *(name kept: struct member / JSON-save mapped)* |
| `TargetLevel` | `TSoftObjectPtr<UWorld>` | `null` | Transit | EditAnywhere + BP | Map to open when a LoadLevel destination is chosen. *(name kept: struct member / JSON-save mapped)* |
| `ArrivalPointTag` | `FName` | `NAME_None` | Transit | EditAnywhere + BP | PlayerStartTag the player spawns at after a LoadLevel travel. *(name kept: struct member / JSON-save mapped)* |
| `ElevatorLocation` | `TObjectPtr<AZP_TransitLocation>` | `null` | Transit | EditAnywhere + BP | Preferred in-map elevator stop marker this destination travels to (overrides ElevatorTargetRelativeZ). *(name kept: struct member / JSON-save mapped)* |
| `ElevatorTargetRelativeZ` | `float` | `0.f` | Transit | EditAnywhere + BP | Fallback raw relative Z (UU) from the elevator's start position when no ElevatorLocation marker is set. *(name kept: struct member / JSON-save mapped)* |
| `RequiredObjectiveId` | `FName` | `NAME_None` | Transit|Gating | EditAnywhere + BP | Objective that must be complete before this destination unlocks. *(name kept: struct member / JSON-save mapped)* |
| `RequiredFlag` | `FName` | `NAME_None` | Transit|Gating | EditAnywhere + BP | Progression flag that must be set before this destination unlocks (preferred gate for objective steps). *(name kept: struct member / JSON-save mapped)* |
| `StartObjectiveIfLocked` | `FName` | `NAME_None` | Transit|Gating | EditAnywhere + BP | Objective to start when the player selects this destination while it is still locked. *(name kept: struct member / JSON-save mapped)* |
| `RequiredKeyItem` | `TSoftObjectPtr<UObject>` | `null` | Transit|Gating | EditAnywhere + BP | Moonville item data asset the player must carry to travel to this destination. *(name kept: struct member / JSON-save mapped)* |
| `bConsumeKey` | `bool` | `false` | Transit|Gating | EditAnywhere + BP | Whether the required key item is consumed from inventory on use. *(name kept: struct member / JSON-save mapped)* |
| `LockedReason` | `FText` |  | Transit | not editor-editable | Player-facing text shown when the destination is locked (GreyedWithReason style). *(name kept: struct member / JSON-save mapped)* |
| `LockStyle` | `EZP_TransitLockStyle` | `EZP_TransitLockStyle::GreyedWithReason` | Transit|Gating | EditAnywhere + BP | How a locked destination is displayed: hidden until known, or greyed with a reason. *(name kept: struct member / JSON-save mapped)* |

### FZP_WallMap
_Static (non-UObject) level-wide database of climbable wall surfaces; scanned once at level start via grid line traces, queried by creature AI to find/score walls._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_ScanExtent` | `float (constexpr)` | `10000.f` | EnemyAI|WallMap | internal (C++ only) | Half-size (UU) of the square scan area around the scan center when building the wall map. |
| `AZP_GridStep` | `float (constexpr)` | `500.f` | EnemyAI|WallMap | internal (C++ only) | Distance (UU) between grid scan points; lower = more wall detail but more traces at level start. |
| `AZP_TraceLength` | `float (constexpr)` | `2000.f` | EnemyAI|WallMap | internal (C++ only) | Length (UU) of each horizontal scan ray looking for wall surfaces. |
| `AZP_ScanHeights` | `float[3] (constexpr array; becomes TArray<float>)` | `{ 100.f, 400.f, 800.f }` | EnemyAI|WallMap | internal (C++ only) | World-Z offsets above the scan center at which horizontal scan passes run; multiple heights catch elevated walls. |
| `AZP_WallTopTraceStep` | `float (constexpr)` | `50.f` | EnemyAI|WallMap | internal (C++ only) | Vertical step size (UU) used when tracing up a wall to find its top edge. |
| `AZP_WallTopTraceMax` | `float (constexpr)` | `3000.f` | EnemyAI|WallMap | internal (C++ only) | Maximum height (UU) traced upward when searching for a wall's top. |
| `AZP_MinWallHeight` | `float (constexpr)` | `250.f` | EnemyAI|WallMap | internal (C++ only) | Minimum wall height (UU) worth recording; 250 = 2.5m so furniture (desks/partitions) is not mapped as a wall - a key 'wall vs furniture' balance value. |
| `AZP_WallNormalZThreshold` | `float` | `0.5f` | EnemyAI|WallMap | internal (C++ only) | Max \|ImpactNormal.Z\| for a hit surface to count as a climbable wall (0.5 = surfaces steeper than ~60 degrees); also documented in the header comment ZP_WallMap.h:36. |
| `AZP_MinWallConsiderDistance` | `float` | `100.f (compared squared: 100.f * 100.f)` | EnemyAI|WallMap | internal (C++ only) | Walls closer than this (UU) to the creature are skipped by FindBestToward so it never picks the wall it is already touching. |
| `AZP_WallScoreWeights` | `float x3 (DirWeight / DistWeight / HeightWeight)` | `0.5f / 0.3f / 0.2f (single site, ZP_WallMap.cpp:153)` | EnemyAI|WallMap | internal (C++ only) | FindBestToward scoring blend: direction-toward-target weight (0.5), closeness weight (0.3), wall-height weight (0.2) - directly shapes which wall a creature chooses; promote as three floats (AZP_WallScoreDirWeight, AZP_WallScoreDistWeight, AZP_WallScoreHeightWeight). |
| `AZP_HeightScoreMaxWallHeight` | `float` | `500.f` | EnemyAI|WallMap | internal (C++ only) | Wall height (UU) at which the height component of the wall score saturates at 1.0 - taller walls than this gain no extra preference. |

### UZP_BloodFXComponent
_Per-enemy blood identity component: weapons call static PlayHitBloodFor() on hit actors; spawns a composite of Niagara burst, guaranteed traced wall/floor splatter decals, delayed floor pool, optional bleed dribble, and pre-skinned material body wounds; class defaults make every actor bleed even without the component._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_BloodIntensity` | `int32` | `2` | Blood | EditAnywhere + BP | The per-enemy spurt intensity dial 1-3 selecting which of the pack's three hit-system levels plays (MeleeBloodSystems/RangedBloodSystems[Intensity-1]). |
| `AZP_MeleeBloodSystems` | `TArray<TObjectPtr<UNiagaraSystem>>` | `empty (lazily filled from MeleeBloodPaths: P_SplashWithBurst_Hit_01/02/03)` | Blood | EditAnywhere + BP | Melee-hit Niagara system trio by intensity ([0]=1..[2]=3); default is the pack's Splash-with-Burst+Hit trio. |
| `AZP_RangedBloodSystems` | `TArray<TObjectPtr<UNiagaraSystem>>` | `empty (lazily filled from RangedBloodPaths: P_Splash_HitWithMetal_01/02/03)` | Blood | EditAnywhere + BP | Ranged/bullet-hit Niagara system trio by intensity; default is the pack's Splash+Hit+Metal trio. |
| `AZP_BleedSystem` | `TObjectPtr<UNiagaraSystem>` | `null (deliberately not defaulted - pack bleed systems dev-rejected as a gag)` | Blood | EditAnywhere + BP | Optional Niagara dribble attached to the body after a heavy melee hit; only used if assigned. |
| `AZP_BleedDuration` | `float` | `1.6f` | Blood | EditAnywhere + BP | Seconds the attached bleed system runs before being deactivated (only when BleedSystem is set). |
| `bAZP_DelayedFloorPool` | `bool` | `true` | Blood|Splatter | EditAnywhere + BP | Toggles the quiet-aftermath pool decal that forms under the enemy a beat after a heavy melee hit. |
| `AZP_PoolDelay` | `float` | `1.2f` | Blood|Splatter | EditAnywhere + BP | Seconds after the heavy hit before the floor pool decal appears. |
| `AZP_PoolSizeMin` | `float` | `28.f` | Blood|Splatter | EditAnywhere + BP | Minimum random size (UU) of the delayed floor pool decal. |
| `AZP_PoolSizeMax` | `float` | `44.f` | Blood|Splatter | EditAnywhere + BP | Maximum random size (UU) of the delayed floor pool decal. |
| `bAZP_DisableBodyDecals` | `bool` | `true` | Blood|Splatter | EditAnywhere + BP | Sets ReceivesDecals=false on the owner's skeletal meshes at BeginPlay (world decals smearing on deforming skin is a documented DEAD END; keep ON). |
| `bAZP_BodyWounds` | `bool` | `true` | Blood|Wounds | EditAnywhere + BP | Toggles the clinging pre-skinned sphere-mask body wounds written into the M_ZP_CreatureSkin material. |
| `AZP_WoundColor` | `FLinearColor` | `FLinearColor(0.06f, 0.008f, 0.1f, 1.f)` | Blood|Wounds | EditAnywhere + BP | Wound tint blended into the skin material's BaseColor. |
| `AZP_WoundRadiusMin` | `float` | `14.f` | Blood|Wounds | EditAnywhere + BP | Minimum body-wound radius in world units (converted to mesh-local for the shader). |
| `AZP_WoundRadiusMax` | `float` | `24.f` | Blood|Wounds | EditAnywhere + BP | Maximum body-wound radius in world units. |
| `AZP_MaxBodyWounds` | `int32` | `4` | Blood|Wounds | EditAnywhere + BP | Round-robin wound slot count; MUST match the WoundLoc_N/WoundRadius_N slot count (4) authored in M_ZP_CreatureSkin - raising it needs a material edit too. |
| `bAZP_BodyResidual` | `bool` | `false` | Blood|Splatter | EditAnywhere + BP | Experiment-only toggle for bone-attached residual decals on the body (documented DEAD END, default OFF; also requires bDisableBodyDecals=false). |
| `AZP_NumBodyResiduals` | `int32` | `3` | Blood|Splatter | EditAnywhere + BP | How many residual stains attach to the body per hit when the dead-end bBodyResidual experiment is enabled. |
| `AZP_BodyResidualSizeMin` | `float` | `7.f` | Blood|Splatter | EditAnywhere + BP | Minimum size of a body residual stain decal (dead-end experiment path). |
| `AZP_BodyResidualSizeMax` | `float` | `14.f` | Blood|Splatter | EditAnywhere + BP | Maximum size of a body residual stain decal (dead-end experiment path). |
| `AZP_BloodColor` | `FLinearColor` | `FLinearColor(0.14f, 0.015f, 0.22f, 1.f)` | Blood | EditAnywhere + BP | Airborne particle blood tint (default dark infected purple that still reads in dark rooms; Marcus's own component overrides it to red in the ZP_GraceCharacter constructor). |
| `AZP_SmokeColor` | `FLinearColor` | `FLinearColor(0.06f, 0.02f, 0.1f, 1.f)` | Blood | EditAnywhere + BP | Tint of the mist/smoke layer of the burst systems. |
| `AZP_DecalColor` | `FLinearColor` | `FLinearColor(0.05f, 0.006f, 0.085f, 1.f)` | Blood | EditAnywhere + BP | Splatter decal tint - inkier/darker than the airborne blood for a drying-ink look on surfaces. |
| `AZP_BloodScale` | `float` | `1.6f` | Blood | EditAnywhere + BP | Overall burst scale (applied as component scale and as the pack's Scale user param). |
| `AZP_LifeTimeMult` | `float` | `1.5f` | Blood | EditAnywhere + BP | Particle lifetime multiplier (the pack's LifeTimeMult user param) - longer-lingering droplets. |
| `AZP_DecalMaterial` | `TObjectPtr<UMaterialInterface>` | `null (lazily loaded from /Game/Blood_VFX_Pack/Materials/MI_BloodDecal)` | Blood|Splatter | EditAnywhere + BP | Decal material for all traced splatter/pool/residual stamps; the master must expose a Color vector param. |
| `AZP_NumWallSplats` | `int32` | `2` | Blood|Splatter | EditAnywhere + BP | Number of decal splats traced along the spray onto walls/geometry behind the enemy per hit. |
| `AZP_NumFloorSplats` | `int32` | `2` | Blood|Splatter | EditAnywhere + BP | Number of decal splats traced down onto the floor around/past the enemy per hit. |
| `AZP_DecalSizeMin` | `float` | `26.f` | Blood|Splatter | EditAnywhere + BP | Minimum random size (UU) of wall/floor splatter decals. |
| `AZP_DecalSizeMax` | `float` | `58.f` | Blood|Splatter | EditAnywhere + BP | Maximum random size (UU) of wall/floor splatter decals. |
| `AZP_DecalLifetime` | `float` | `0.f` | Blood|Splatter | EditAnywhere + BP | Seconds before a splatter decal fades; 0 means it never fades and stays permanently. |
| `AZP_WallTraceDistance` | `float` | `420.f` | Blood|Splatter | EditAnywhere + BP | How far behind the wound (UU) the wall-splatter trace reaches. |
| `AZP_ColorParam` | `FName` | `FName(TEXT("Color"))` | Blood | EditAnywhere + BP | Name of the Niagara user parameter that receives BloodColor; the VALUE 'Color' is a third-party Rimaye pack param name and must not change unless the systems are swapped. |
| `AZP_SmokeColorParam` | `FName` | `FName(TEXT("SmokeColor"))` | Blood | EditAnywhere + BP | Name of the Niagara user parameter that receives SmokeColor (third-party pack param name as value). |
| `AZP_ScaleParam` | `FName` | `FName(TEXT("Scale"))` | Blood | EditAnywhere + BP | Name of the Niagara user parameter that receives BloodScale (third-party pack param name as value). |
| `AZP_LifeTimeParam` | `FName` | `FName(TEXT("LifeTimeMult"))` | Blood | EditAnywhere + BP | Name of the Niagara user parameter that receives LifeTimeMult (third-party pack param name as value). |
| `AZP_DecalColorParam` | `FName` | `FName(TEXT("Color"))` | Blood | EditAnywhere + BP | Name of the decal material's tint vector parameter ('Color' on the pack's MI_BloodDecal master). |
| `AZP_MeleeBloodPaths` | `const TCHAR*[3]` | `/Game/Blood_VFX_Pack/Particles/Systems/P_SplashWithBurst_Hit_01/02/03` | Blood | internal (C++ only) | File-scope default asset paths for the melee hit trio - single source of truth for lazy load, CDO fallback, and prewarm; the exposed MeleeBloodSystems array is the designer-facing override. |
| `AZP_RangedBloodPaths` | `const TCHAR*[3]` | `/Game/Blood_VFX_Pack/Particles/Systems/P_Splash_HitWithMetal_01/02/03` | Blood | internal (C++ only) | File-scope default asset paths for the ranged hit trio (lazy load, CDO fallback, prewarm); RangedBloodSystems is the designer-facing override. |
| `AZP_BloodDecalPath` | `const TCHAR*` | `/Game/Blood_VFX_Pack/Materials/MI_BloodDecal.MI_BloodDecal` | Blood|Splatter | internal (C++ only) | File-scope default asset path for the splatter decal material (lazy load, CDO fallback, prewarm); DecalMaterial is the designer-facing override. |
| `AZP_DecalFadeScreenSize` | `float` | `0.0005f` | Blood|Splatter | internal (C++ only) | SetFadeScreenSize applied to every spawned blood decal so splats don't vanish at distance; same value at three sites - ZP_BloodFXComponent.cpp:315 (splats), :385 (floor pool), :440 (body residual). |
| `AZP_SplatterDecalDepth` | `float` | `32.f` | Blood|Splatter | internal (C++ only) | Projection-box depth (X of FVector(32,Size,Size)) for wall/floor splat and pool decals; same tune at ZP_BloodFXComponent.cpp:312 and :383. |
| `AZP_WallSplatJitterLateral` | `float` | `0.18f` | Blood|Splatter | internal (C++ only) | Half-width (+/-0.18) of the random lateral jitter added to each wall-splat trace direction - the spray cone spread. |
| `AZP_WallSplatUpwardBias` | `float` | `FRandRange(-0.05f, 0.28f)` | Blood|Splatter | internal (C++ only) | Vertical jitter range (-0.05..0.28, upward-skewed) added to wall-splat trace directions so the through-spray reads as an arterial arc. |
| `AZP_FloorSplatScatter` | `float` | `forward FRandRange(30,170), lateral +/-40, start +30 up` | Blood|Splatter | internal (C++ only) | Scatter pattern of floor-splat trace starts: 30-170 UU forward along the spray plus +/-40 UU lateral jitter - how far past the wound the drops land (lines 488-490). |
| `AZP_FloorSplatTraceDepth` | `float` | `350.f` | Blood|Splatter | internal (C++ only) | How far down (UU) each floor-splat trace searches for WorldStatic ground below its start point. |
| `AZP_PoolTraceDepth` | `float` | `300.f` | Blood|Splatter | internal (C++ only) | How far down (UU) the delayed floor-pool trace searches under the enemy's feet for ground to stamp the pool on. |

### UZP_CrawlerBehaviorComponent
_Brain for the wall-ambush Crawler enemy: perched-on-wall dormancy, gaze/proximity provocation, silent drop, ballistic launch, ground hunt and slam attacks, lurk audio, and gunshot alerting (EnemyAI subsystem)._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `bAZP_SpawnOnWall` | `bool` | `true` | Crawler|Perch | EditAnywhere + BP | Whether the crawler adheres to the wall it was placed against on spawn (the wall becomes its floor). |
| `AZP_SpawnWallSnapRange` | `float` | `300.f` | Crawler|Perch | EditAnywhere + BP | Max distance (UU) from the spawn point to search for a wall to adhere to. |
| `AZP_PerchDormantRange` | `float` | `4000.f` | Crawler|Perch | EditAnywhere + BP | While perched, the player is ignored entirely beyond this range in UU (4000 = 40m). |
| `AZP_DropProximity` | `float` | `500.f` | Crawler|Perch | EditAnywhere + BP | Player closer than this (UU) to a perched crawler triggers a silent drop into a ground hunt (500 = 5m). |
| `AZP_GazeLaunchRange` | `float` | `2000.f` | Crawler|Perch | EditAnywhere + BP | Max range (UU) at which a sustained player stare provokes the launch pounce (2000 = 20m). |
| `AZP_GazeProvokeTime` | `float` | `2.0f` | Crawler|Perch | EditAnywhere + BP | Seconds the player must stare directly at the perched crawler before it launches. |
| `AZP_GazeAlertCooldown` | `float` | `3.0f` | Crawler|Perch | EditAnywhere + BP | Min seconds between gaze-provoke alert hisses so a held stare does not spam the vocal. |
| `AZP_GazeAlertLowPassHz` | `float` | `5000.f` | Crawler|Perch | EditAnywhere + BP | Low-pass filter frequency (Hz) applied to the gaze-provoke alert vocal for a muffled hiss. |
| `AZP_GazeThreshold` | `float` | `0.85f` | Crawler|Perch | EditAnywhere + BP | dot(view, dirToCrawler) threshold for 'looking right at it' (0.85 is roughly a 30-degree cone). |
| `AZP_HuntSpeed` | `float` | `220.f` | Crawler|Hunt | EditAnywhere + BP | Ground pursuit speed in UU/s (applied to the CMC MaxWalkSpeed/MaxFlySpeed via SetSpeed). |
| `AZP_DetectionRange` | `float` | `3000.f` | Crawler|Hunt | EditAnywhere + BP | Range (UU) within which a grounded crawler (re)acquires the player on a clear sightline. |
| `AZP_AttackRange` | `float` | `200.f` | Crawler|Attack | EditAnywhere + BP | Horizontal distance (UU) at which a grounded crawler starts a slam attack. |
| `AZP_AttackCooldown` | `float` | `1.5f` | Crawler|Attack | EditAnywhere + BP | Seconds between attacks (launch or slam). |
| `AZP_AttackDamage` | `float` | `25.f` | Crawler|Attack | EditAnywhere + BP | Damage dealt per landed attack via UGameplayStatics::ApplyDamage. |
| `AZP_AttackHitRadius` | `float` | `250.f` | Crawler|Attack | EditAnywhere + BP | Proximity (UU) to the player at impact time required for an attack to actually connect and deal damage. |
| `AZP_SlamHoldTime` | `float` | `0.3f` | Crawler|Attack | EditAnywhere + BP | Slam wind-up/strike duration in seconds before the impact check (passed to CMC BeginSlam). |
| `AZP_LurkRange` | `float` | `1500.f` | Crawler|Audio | EditAnywhere + BP | Player distance (UU) inside which an unnoticed perched crawler plays periodic lurk growls. |
| `AZP_LurkIntervalMin` | `float` | `7.f` | Crawler|Audio | EditAnywhere + BP | Minimum seconds between lurk growls (random interval lower bound). |
| `AZP_LurkIntervalMax` | `float` | `15.f` | Crawler|Audio | EditAnywhere + BP | Maximum seconds between lurk growls (random interval upper bound). |
| `AZP_EvalInterval` | `float` | `0.3f` | Crawler | EditAnywhere + BP | Behavior evaluation timer period in seconds; also the increment used by the gaze-stare and lurk timers each eval. |
| `AZP_GunshotAlertRadius` | `float` | `5000.f` | Crawler | EditAnywhere + BP | Max range (UU) a gunshot can wake this crawler (5000 = 50m); also gated on a same-room LOS trace. |
| `bAZP_AutoInitialize` | `bool` | `true` | Crawler | EditAnywhere + BP | Whether InitializeBehavior runs automatically on BeginPlay (feature toggle for manual/scripted initialization). |

### UZP_CrawlerMovementComponent
_Minimal CharacterMovementComponent for the wall-ambush Crawler: four exclusive PhysFlying-driven modes (Cling/Launch/Slam/Ground) that own all velocity and rotation._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_CrawlerGravity` | `float` | `980.f` | Crawler|Movement | EditAnywhere + BP | Downward gravity acceleration applied to the crawler during launch arcs and ground pursuit (used both for arc solving in BeginLaunch and per-frame fall in PhysFlying). |
| `AZP_TerminalVelocity` | `float` | `2000.f` | Crawler|Movement | EditAnywhere + BP | Maximum downward fall speed the crawler can reach while launching or falling. |
| `AZP_LaunchHorizSpeed` | `float` | `1400.f` | Crawler|Launch | EditAnywhere + BP | Horizontal speed used to solve the ballistic pounce arc (local const 'HorizSpeed' in BeginLaunch) — governs the L4D-Hunter fast/low pounce feel. |
| `AZP_LaunchTimeMin` | `float` | `0.30f` | Crawler|Launch | EditAnywhere + BP | Lower clamp on the solved pounce flight time — shorter minimum makes close-range pounces snappier and flatter. |
| `AZP_LaunchTimeMax` | `float` | `0.85f` | Crawler|Launch | EditAnywhere + BP | Upper clamp on the solved pounce flight time — caps how long/floaty a long-range pounce arc can be. |
| `AZP_LaunchMaxDuration` | `float` | `1.5f` | Crawler|Launch | EditAnywhere + BP | Safety timeout in seconds after which an in-flight launch force-ends and flags an impact so the crawler never flies forever. |
| `AZP_GroundPursuitInterpSpeed` | `float` | `8.f` | Crawler|Ground | EditAnywhere + BP | VInterpTo ease rate for horizontal velocity while ground-pursuing the target — higher snaps to full chase speed faster. |
| `AZP_VoidKillZ` | `float` | `-5000.f` | Crawler|Safety | EditAnywhere + BP | World Z below which a runaway crawler destroys itself — may need per-level tuning on maps with deep geometry. |
| `AZP_BodyRotationRate` | `float` | `10.f` | Crawler|Rotation | EditAnywhere + BP | RInterpTo rate for turning the body to face travel direction while on the ground or in the air. |
| `AZP_ClingRotationRate` | `float` | `12.f` | Crawler|Rotation | EditAnywhere + BP | RInterpTo rate for aligning the body to the wall normal while clinging (wall-as-floor alignment speed). |

### UZP_DeathSaveComponent
_Per-enemy save/state + revival glue: persists the enemy's dead flag into the EasyGameUI per-slot JSON, restores corpse state in place on load (IZP_Revivable), and revives a dead enemy when its configured objective/flag completes._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_ReviveOnObjective` | `FName` | `NAME_None` | Enemy|Persistence | EditAnywhere + BP | Objectives.json id (objective, sub-objective, or flag) whose completion revives this dead enemy; None means the enemy stays dead once killed. Set per placed level instance. |

### UZP_DialogueData
_DataAsset for a complete dialogue sequence — the authoring asset per conversation, monologue, or found document._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_DialogueID` | `FName` | `NAME_None` | Dialogue | EditAnywhere + BP | Unique id used for one-shot save tracking and cross-dialogue jumps. |
| `AZP_Lines` | `TArray<FZP_DialogueLine>` | `empty` | Dialogue | EditAnywhere + BP | Ordered lines of the dialogue sequence. |
| `bAZP_OneShot` | `bool` | `true` | Dialogue | EditAnywhere + BP | If true the dialogue plays only once per save. |
| `AZP_Priority` | `int32` | `50` | Dialogue | EditAnywhere + BP | Queue priority; a higher-priority dialogue interrupts a lower one (guide=100, ambient=10). |

### UZP_DialogueManager
_ActorComponent on the PlayerController that orchestrates dialogue playback: ticks through lines, plays 2D voice audio, fires subtitle/choice delegates to the UI, manages a priority queue, and tracks one-shot playback and narrative beats for save persistence._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_DialogueLookupTable` | `TMap<FName, TObjectPtr<UZP_DialogueData>>` | `empty (auto-populated at BeginPlay via AssetRegistry scan and RegisterDialogue)` | Dialogue | EditAnywhere + BP | Maps DialogueID names to UZP_DialogueData assets for ID-based playback and choice jumps; can be pre-populated in editor but is also auto-registered at runtime. |
| `AZP_MinSubtitleDuration` | `float` | `2.f` | Dialogue|Timing | EditAnywhere + BP | Minimum on-screen time in seconds for a text-only dialogue line (no audio, no explicit Duration) before auto-advancing. |
| `AZP_SubtitleSecondsPerChar` | `float` | `0.05f` | Dialogue|Timing | EditAnywhere + BP | Reading-speed fallback for text-only lines: seconds of display time added per subtitle character (~50ms/char) when a line has no audio and no explicit Duration. |

### UZP_DialogueWidget
_C++ base for the dialogue UI widget (WBP_DialogueBox): shows speaker + subtitle, spawns choice buttons, smooth fade in/out, binds to UZP_DialogueManager delegates._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_ChoiceButtonClass` | `TSubclassOf<UZP_DialogueChoiceButton>` | `null (set to WBP_DialogueChoiceButton in WBP_DialogueBox defaults)` | Dialogue|UI | EditDefaultsOnly | Widget class spawned for each dialogue choice button; must be a UZP_DialogueChoiceButton subclass, authored in WBP_DialogueBox class defaults. |
| `AZP_FadeSpeed` | `float` | `8.f` | Dialogue|UI | EditDefaultsOnly | FInterpTo speed for the dialogue box render-opacity fade in/out. |
| `AZP_ChoiceButtonPadding` | `FMargin` | `FMargin(0.f, 4.f, 0.f, 4.f)` | Dialogue|UI | EditAnywhere + BP | Vertical padding applied to each spawned choice button in the choice list - the only layout value of the choice stack not authorable in UMG because the buttons are spawned in C++. |

### UZP_EnemyAudioComponent
_Reusable spatialized enemy voice component: intermittent lurking growls plus alert/hit/attack one-shots, all routed through UZP_SFXStatics with Far carry and occlusion muffle._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_LurkingLoop` | `TObjectPtr<USoundBase>` | `/Game/Audio/Crawler/SFX_Crawler_Lurking (set in ctor via ConstructorHelpers)` | EnemyAudio | EditAnywhere + BP | The lurking growl one-shot played intermittently at low volume while the enemy has not noticed the player. |
| `AZP_AlertSound` | `TObjectPtr<USoundBase>` | `/Game/Audio/Crawler/SFX_Crawler_Alert (set in ctor via ConstructorHelpers)` | EnemyAudio | EditAnywhere + BP | One-shot vocalization played when the enemy notices the player. |
| `AZP_HitSound` | `TObjectPtr<USoundBase>` | `/Game/Audio/Crawler/SFX_Crawler_Hit (set in ctor via ConstructorHelpers)` | EnemyAudio | EditAnywhere + BP | One-shot vocalization played when the enemy takes damage but survives. |
| `AZP_AttackSounds` | `TArray<TObjectPtr<USoundBase>>` | `[/Game/Audio/Crawler/SFX_Crawler_Attack, /Game/Audio/Crawler/SFX_Crawler_Attack2] (set in ctor)` | EnemyAudio | EditAnywhere + BP | Attack vocalizations: index 0 is the normal strike (Attack1), the last element is the lunge/surprise strike (Attack2). |
| `AZP_VolumeMultiplier` | `float` | `1.f` | EnemyAudio | EditAnywhere + BP | Master volume multiplier applied to every voice one-shot this component plays. |
| `AZP_LurkVolume` | `float` | `0.8f` | EnemyAudio | EditAnywhere + BP | Base volume for lurking growls - quieter than alert/attack but meant to stay clearly audible. |
| `AZP_LurkVolumeJitterDb` | `float` | `2.f` | EnemyAudio | EditAnywhere + BP | Per-growl random volume swing in +/- dB that keeps the intermittent lurking sound organic instead of mechanical. |

### UZP_FloorCullingComponent
_Player-attached perf component that buckets level actors by floor Z-band and hides non-adjacent floors; currently neutralized (AdjacentFloorsToShow=99) after the session-65 'blackbox' incident._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_FloorHeight` | `float` | `500.0f` | Floor Culling | EditAnywhere + BP | Height of each building floor in UU, used to bucket actors and locate the player's floor. |
| `AZP_FloorBaseZ` | `float` | `0.0f` | Floor Culling | EditAnywhere + BP | World Z of the bottom of floor 1; the origin of the floor-band math. |
| `AZP_NumFloors` | `int32` | `5` | Floor Culling | EditAnywhere + BP | Total number of floors in the building; floor indices are clamped to this range. |
| `AZP_AdjacentFloorsToShow` | `int32` | `99` | Floor Culling | EditAnywhere + BP | How many floors above and below the player's floor stay visible; 99 deliberately neutralizes culling (session-65 blackbox fix) until CollectActors skips sky/env actors. |
| `AZP_CheckInterval` | `float` | `0.3f` | Floor Culling | EditAnywhere + BP | Seconds between polls of the player's Z position to detect a floor change. |
| `AZP_AlwaysVisibleZones` | `TArray<FBox>` | `empty array` | Floor Culling | EditAnywhere + BP | Boxes (stairwells, atriums, vertical shafts) whose actors are never culled or ISM-batched; currently hardcoded-populated by ZP_GraceCharacter.cpp:681 and mirrored onto UZP_RuntimeISMBatcher (ZP_GraceCharacter.cpp:687). |
| `AZP_SkipActorClassNames` | `TSet<FString> (propose TArray<FString> knob)` | `{"SkyAtmosphere","SkyLight","DirectionalLight","ExponentialHeightFog","VolumetricCloud","PostProcessVolume","LightmassImportanceVolume","PlayerStart","WorldSettings","GameModeBase","NavigationData","AbstractNavData","LevelBounds"}` | Floor Culling | EditAnywhere + BP | Class names (BP _C suffix stripped) that are never floor-culled; currently a function-local static const set - exposing it is the prerequisite the h:54 comment names for ever re-enabling culling (add sky/env classes like Ultra_Dynamic_Sky per level). |

### UZP_FootstepData
_THE footstep surface table DataAsset (DA_Footsteps): rows matched top-down plus fallback DefaultSounds; Resolve() picks the best row per floor._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_Surfaces` | `TArray<FZP_FootstepSurface>` | `empty` | Footsteps | EditAnywhere + BP | Surface rows matched top-down (SurfaceType first, then material-name keywords). |
| `AZP_DefaultSounds` | `TArray<TObjectPtr<USoundBase>>` | `empty` | Footsteps | EditAnywhere + BP | Fallback footstep sound set for unmatched floors and placeholder rows. |

### UZP_GraceAnimInstance
_C++ AnimInstance base for the player's locomotion AnimBP (ABP_GraceLocomotion): mirrors character movement state (speed, direction, air/crouch/sprint, gait) into AnimGraph-readable properties every frame._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_MovingSpeedThreshold` | `float` | `3.0f` | Locomotion|Tuning | EditAnywhere + BP | Ground-speed threshold (cm/s) above which the character counts as moving (drives bShouldMove and the idle/locomotion switch feel). |

### UZP_GraceGameplayComponent
_Reusable ActorComponent encapsulating the player's gameplay systems: stamina/sprint, wall-peek + cover-aim, head bob, camera offsets, interaction trace, and applying the UZP_GraceMovementConfig DataAsset to the CharacterMovementComponent._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_MovementConfig` | `TObjectPtr<UZP_GraceMovementConfig>` | `null (transient default created at BeginPlay from C++ UPROPERTY defaults if unassigned)` | Config | EditDefaultsOnly + BP | DataAsset that holds every movement/stamina/peek/head-bob tuning value this component consumes. |
| `AZP_CameraComponent` | `TObjectPtr<UCameraComponent>` | `null (auto-discovered at BeginPlay by component name 'FirstPersonCamera')` | References | EditAnywhere + BP | Which camera the component drives for peek, head bob, camera offsets and the interaction trace; auto-discovered by name when left unset. |
| `bAZP_UseBuiltInHeadBob` | `bool` | `true` | Movement|HeadBob | EditAnywhere + BP | Feature toggle for the component's procedural positional head bob; disable when Kinemation owns camera motion. |
| `AZP_BlockDodgeForwardClearance` | `float` | `14.0f` | Movement|Camera | EditAnywhere + BP | Transient forward camera nudge (cm) snapped in during the block/dodge lean-in so the leaning body cannot clip the lens. |
| `AZP_ForwardClearanceInterpSpeed` | `float` | `8.0f` | Movement|Camera | EditAnywhere + BP | Interp speed for easing the block/dodge forward camera clearance back to zero after the lean-in window ends. |
| `AZP_CameraExtraForward` | `float` | `-70.0f` | Movement|Camera | EditAnywhere + BP | Forward (capsule +X) camera offset in cm pulling the camera off the Operator FPCamera socket back onto Marcus's head for the unarmed/melee eye line. |
| `AZP_CameraExtraHeight` | `float` | `12.0f` | Movement|Camera | EditAnywhere + BP | World-up camera offset in cm setting Marcus's eye line so the body extends correctly below the down-view. |
| `AZP_CameraRangedForward` | `float` | `0.0f` | Movement|Camera | EditAnywhere + BP | Forward camera offset used only while a ranged weapon is up (Kinemation Operator arms), framing the armed POV independently of the unarmed body view; 0 = camera exactly at the FPCamera socket. |
| `AZP_CameraRangedHeight` | `float` | `0.0f` | Movement|Camera | EditAnywhere + BP | Up camera offset used only while a ranged weapon is up, paired with CameraRangedForward for the armed POV framing. |
| `AZP_MinSpeedForBob` | `float` | `10.0f` | Movement|HeadBob | EditAnywhere + BP | Minimum planar speed (cm/s) before head bob starts; below it the bob eases back to rest. |
| `AZP_SprintDrainForwardSpeedThreshold` | `float` | `50.0f` | Movement|Stamina | EditAnywhere + BP | Forward speed (cm/s) the player must exceed while sprinting for stamina to actually drain — holding sprint while standing still regens instead. |
| `AZP_WallStuckSpeedThreshold` | `float` | `30.0f` | Movement|Stamina | EditAnywhere + BP | Planar speed (cm/s) below which pressing forward while sprinting on ground counts as being stalled against a wall. |
| `AZP_WallStuckCancelTime` | `float` | `0.25f` | Movement|Stamina | EditAnywhere + BP | Seconds of wall-stall (sprinting into a wall without moving) before sprint auto-cancels so stamina stops draining pointlessly. |
| `AZP_GaitRunSpeedRatio` | `float` | `0.5f` | Movement|GASP | EditAnywhere + BP | Fraction of WalkSpeed above which the GASP gait classifier reports Run (1) instead of Walk (0) — feeds the AnimBP gait state. |
| `AZP_CameraAutoDiscoverName` | `FName` | `"FirstPersonCamera"` | References | EditAnywhere + BP | Component name searched on the owner at BeginPlay to auto-discover the camera when CameraComponent is left unset. |

### UZP_GraceMovementConfig
_UDataAsset holding ALL player movement/camera/stamina/peek tuning for the first-person character (legacy 'Grace' name; character is Marcus). Read at BeginPlay by ZP_GraceCharacter and ZP_GraceGameplayComponent._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_CapsuleRadius` | `float` | `55.0f` | Capsule | EditAnywhere + BP | Player capsule collision radius in cm. |
| `AZP_CapsuleHalfHeight` | `float` | `88.0f` | Capsule | EditAnywhere + BP | Standing capsule half-height in cm. |
| `AZP_CrouchedHalfHeight` | `float` | `44.0f` | Capsule | EditAnywhere + BP | Capsule half-height while crouched, in cm. |
| `AZP_PlayerMeshOffsetZ` | `float` | `-90.0f` | Capsule | EditAnywhere + BP | PlayerMesh Z offset from capsule origin in cm, aligning mesh feet with capsule bottom. |
| `AZP_WalkSpeed` | `float` | `260.0f` | Movement|Walk | EditAnywhere + BP | Base walk speed in cm/s (feeds CMC MaxWalkSpeed). |
| `AZP_SprintSpeed` | `float` | `390.0f` | Movement|Sprint | EditAnywhere + BP | Sprint speed in cm/s. |
| `AZP_BrakingDeceleration` | `float` | `1400.0f` | Movement|Walk | EditAnywhere + BP | Braking deceleration while walking; higher stops the player faster. |
| `AZP_MaxAcceleration` | `float` | `1200.0f` | Movement|Walk | EditAnywhere + BP | Max movement acceleration; lower gives sluggish starts. |
| `AZP_GroundFriction` | `float` | `6.0f` | Movement|Walk | EditAnywhere + BP | Ground friction; higher means more grip. |
| `AZP_MaxStamina` | `float` | `100.0f` | Movement|Stamina | EditAnywhere + BP | Maximum sprint stamina pool. |
| `AZP_StaminaDrainRate` | `float` | `10.0f` | Movement|Stamina | EditAnywhere + BP | Stamina drained per second while sprinting. |
| `AZP_StaminaRegenRate` | `float` | `15.0f` | Movement|Stamina | EditAnywhere + BP | Stamina regenerated per second while not sprinting. |
| `AZP_StaminaRegenDelay` | `float` | `2.0f` | Movement|Stamina | EditAnywhere + BP | Seconds after sprint stops before stamina regen begins. |
| `AZP_HeadBobFrequency` | `float` | `1.6f` | Movement|HeadBob | EditAnywhere + BP | Head bob cycles per second while walking. |
| `AZP_HeadBobVerticalAmplitude` | `float` | `0.8f` | Movement|HeadBob | EditAnywhere + BP | Vertical head bob amplitude in cm per step. |
| `AZP_HeadBobHorizontalAmplitude` | `float` | `0.4f` | Movement|HeadBob | EditAnywhere + BP | Side-to-side head bob sway amplitude in cm. |
| `AZP_SprintBobFrequencyMultiplier` | `float` | `1.4f` | Movement|HeadBob | EditAnywhere + BP | Head bob frequency multiplier while sprinting. |
| `AZP_SprintBobAmplitudeMultiplier` | `float` | `1.5f` | Movement|HeadBob | EditAnywhere + BP | Head bob amplitude multiplier while sprinting (panic feel). |
| `AZP_HeadBobReturnSpeed` | `float` | `6.0f` | Movement|HeadBob | EditAnywhere + BP | Interpolation speed returning the camera to rest rotation after bobbing. |
| `AZP_IdleSwayFrequency` | `float` | `0.4f` | Movement|CameraSway | EditAnywhere + BP | Idle anxious-breathing camera sway frequency. |
| `AZP_IdleSwayAmplitude` | `float` | `0.3f` | Movement|CameraSway | EditAnywhere + BP | Idle camera sway amplitude in degrees. |
| `AZP_CrouchWalkSpeed` | `float` | `150.0f` | Movement|Crouch | EditAnywhere + BP | Walk speed in cm/s while crouched. |
| `AZP_CrouchCameraInterpSpeed` | `float` | `8.0f` | Movement|Crouch | EditAnywhere + BP | Interpolation speed for camera height when entering/exiting crouch. |
| `AZP_JumpZVelocity` | `float` | `300.0f` | Movement|Jump | EditAnywhere + BP | Jump launch velocity; 0 disables jumping. |
| `AZP_AirControl` | `float` | `0.15f` | Movement|Jump | EditAnywhere + BP | Mid-air steering amount (0 = none, 1 = full). |
| `AZP_InteractionTraceRange` | `float` | `250.0f` | Interaction | EditAnywhere + BP | Max range in cm of the interaction line trace. |
| `AZP_CameraHeightOffset` | `float` | `64.0f` | Camera | EditAnywhere + BP | Camera height offset from capsule center in cm. |
| `AZP_DefaultFOV` | `float` | `90.0f` | Camera | EditAnywhere + BP | Default first-person field of view in degrees (propagated to the Kinemation component). |
| `AZP_AdsFOV` | `float` | `65.0f` | Camera|ADS | EditAnywhere + BP | Field of view while aiming down sights; lower = more zoom. |
| `AZP_AdsFOVInterpSpeed` | `float` | `10.0f` | Camera|ADS | EditAnywhere + BP | Interpolation speed of FOV transitions in/out of ADS. |
| `AZP_PeekLateralOffset` | `float` | `25.0f` | Movement|Peek | EditAnywhere + BP | Lateral camera offset in cm toward the open side while peeking. |
| `AZP_PeekForwardOffset` | `float` | `8.0f` | Movement|Peek | EditAnywhere + BP | Forward camera offset in cm pushing the head past the corner while peeking. |
| `AZP_PeekRollAngle` | `float` | `3.0f` | Movement|Peek | EditAnywhere + BP | Camera roll in degrees toward the peek direction (lean feel). |
| `AZP_PeekInterpSpeed` | `float` | `8.0f` | Movement|Peek | EditAnywhere + BP | Interpolation speed when entering peek. |
| `AZP_PeekReturnInterpSpeed` | `float` | `6.0f` | Movement|Peek | EditAnywhere + BP | Interpolation speed when exiting peek. |
| `AZP_PeekWallDetectionRange` | `float` | `180.0f` | Movement|Peek | EditAnywhere + BP | Sphere-trace range in cm for peek wall detection. |
| `AZP_PeekTraceRadius` | `float` | `12.0f` | Movement|Peek | EditAnywhere + BP | Sphere-trace radius in cm for peek wall detection. |
| `AZP_PeekWallHitThreshold` | `int32` | `2` | Movement|Peek | EditAnywhere + BP | Minimum trace hits per side required to count a surface as a wall for peeking. |
| `AZP_PeekTraceFanHalfAngle` | `float` | `75.0f` | Movement|Peek | EditAnywhere + BP | Half-angle in degrees of the 3-ray peek detection fan relative to perpendicular. |
| `AZP_HeadBobPeekDamping` | `float` | `0.15f` | Movement|Peek | EditAnywhere + BP | Head bob amplitude multiplier during peek (0 = no bob, 1 = full). |
| `AZP_HeadBobPeekVerticalDamping` | `float` | `0.5f` | Movement|Peek | EditAnywhere + BP | Vertical head bob amplitude multiplier during peek. |
| `AZP_PeekMaxWallAngleFromVertical` | `float` | `20.0f` | Movement|Peek | EditAnywhere + BP | Max angle in degrees from vertical a surface can be and still count as a peekable wall. |

### UZP_GracePlayerAnimInstance
_C++ AnimInstance for the player's visible PlayerMesh: copies lower-body locomotion bones by name from the hidden Mesh in NativePostEvaluateAnimation, and layers procedural weapon-action overlays (melee swing, grenade throw, weapon-switch arm drop, throwable grip spread) on top of the Kinemation pose._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_GripSpreadPerJoint` | `FRotator` | `FRotator(-20.f, 0.f, 0.f)` | Overlays | EditAnywhere + BP | Additive local-space rotation applied at every right-hand finger joint while the throwable grip spread is active; negative pitch opens the pistol curl into a wide grenade grip. |
| `AZP_GripSpreadIndexScale` | `float` | `0.4f` | Overlays | EditAnywhere + BP | Fraction of GripSpreadPerJoint applied to the index finger chain (full spread splays it too wide — dev verdict session 63). |
| `AZP_GripSpreadThumbScale` | `float` | `0.3f` | Overlays | EditAnywhere + BP | Fraction of GripSpreadPerJoint applied to the thumb chain during throwable grip spread. |
| `AZP_MeleeSwingDuration` | `float` | `0.35f` | Overlays|Melee | EditAnywhere + BP | Total duration of the procedural melee-swing arm overlay in seconds; duplicated as the default parameter of StartMeleeSwing (ZP_GracePlayerAnimInstance.h:68) and overwritten by whatever the caller passes — refactor should make the UPROPERTY the single source and drop the param default. |
| `AZP_GrenadeThrowDuration` | `float` | `0.4f` | Overlays|Grenade | EditAnywhere + BP | Total duration of the procedural grenade-throw arm overlay in seconds; duplicated as the default parameter of StartGrenadeThrow (ZP_GracePlayerAnimInstance.h:71) and overwritten by the caller's argument. |
| `AZP_WeaponSwitchDuration` | `float` | `0.5f` | Overlays|WeaponSwitch | EditAnywhere + BP | Total duration of the weapon-switch arms-drop-then-raise overlay in seconds; duplicated as the default parameter of StartWeaponSwitch (ZP_GracePlayerAnimInstance.h:74) and overwritten by the caller's argument. |
| `AZP_MeleeSwingWindupAngle` | `float` | `-35.f` | Overlays|Melee | EditAnywhere + BP | Peak pull-back/right angle (degrees) of the melee swing wind-up phase; the same -35 appears again as the lerp start of the strike phase (ZP_GracePlayerAnimInstance.cpp:187) and both sites must use the one knob. |
| `AZP_MeleeSwingStrikeAngle` | `float` | `60.f` | Overlays|Melee | EditAnywhere + BP | Peak sweep-left angle (degrees) of the melee strike; also the base of the 65-degree follow-through overshoot at ZP_GracePlayerAnimInstance.cpp:194 and the return-phase start at :202 — promote as one knob (overshoot could be derived as StrikeAngle + 5). |
| `AZP_MeleeSwingDropAmount` | `float` | `8.f` | Overlays|Melee | EditAnywhere + BP | Maximum downward arm dip in cm during the melee strike phase for weight feel; the 8 also appears as the follow-through lerp start at ZP_GracePlayerAnimInstance.cpp:195. |
| `AZP_GrenadeThrowWindupAngle` | `float` | `-30.f` | Overlays|Grenade | EditAnywhere + BP | Peak raise-arm-back angle (degrees) of the grenade throw wind-up; the same -30 is the lerp start of the forward sweep at ZP_GracePlayerAnimInstance.cpp:258. |
| `AZP_GrenadeThrowReleaseAngle` | `float` | `25.f` | Overlays|Grenade | EditAnywhere + BP | Peak forward sweep angle (degrees) of the grenade throw; the same 25 is the follow-through lerp start at ZP_GracePlayerAnimInstance.cpp:264. |
| `AZP_WeaponSwitchDropDistance` | `float` | `20.f` | Overlays|WeaponSwitch | EditAnywhere + BP | How far (cm) both clavicles translate down at the bottom of the weapon-switch arm-lower overlay. |
| `AZP_WeaponSwitchDropRotation` | `float` | `25.f` | Overlays|WeaponSwitch | EditAnywhere + BP | How far (degrees) both upper arms tilt downward at the bottom of the weapon-switch arm-lower overlay. |
| `AZP_UpperBodyBoundaryBoneName` | `FName` | `TEXT("spine_01")` | Locomotion | EditAnywhere + BP | Skeleton bone at and above which bones are treated as Kinemation-owned upper body and skipped by the lower-body locomotion copy (the spine_01 split of the dual-mesh architecture). |

### UZP_HUDWidget
_C++ base for the main gameplay HUD (WBP_HUD): health/stamina arcs, ammo line, weapon icons, interaction/grab prompts, damage/heal/effect vignettes, SignalSense waveform._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_PistolIconTexture` | `TObjectPtr<UTexture2D>` | `null` | HUD|Weapon Icons | EditDefaultsOnly | Texture applied to the pistol weapon-slot HUD icon at construct. |
| `AZP_RifleIconTexture` | `TObjectPtr<UTexture2D>` | `null` | HUD|Weapon Icons | EditDefaultsOnly | Texture applied to the rifle weapon-slot HUD icon at construct. |
| `AZP_ShotgunIconTexture` | `TObjectPtr<UTexture2D>` | `null` | HUD|Weapon Icons | EditDefaultsOnly | Texture applied to the shotgun weapon-slot HUD icon at construct. |
| `AZP_PipeIconTexture` | `TObjectPtr<UTexture2D>` | `null` | HUD|Weapon Icons | EditDefaultsOnly | Texture applied to the pipe (melee) weapon-slot HUD icon at construct. |
| `AZP_GrenadeIconTexture` | `TObjectPtr<UTexture2D>` | `null` | HUD|Weapon Icons | EditDefaultsOnly | Texture applied to the grenade weapon-slot HUD icon at construct. |
| `AZP_HealthArcMaterial` | `TObjectPtr<UMaterialInterface>` | `null (set to M_HealthArc in WBP_HUD class defaults)` | HUD|Health | EditDefaultsOnly | Base UI material for the radial health arc (also reused as the stamina arc base). |
| `AZP_SignalWaveMaterial` | `TObjectPtr<UMaterialInterface>` | `null (set to M_SignalWaveform in WBP_HUD class defaults)` | HUD|Signal | EditDefaultsOnly | Base UI material for the SignalSense phone waveform readout. |
| `AZP_FullHealthColor` | `FLinearColor` | `FLinearColor(0.9f, 0.95f, 1.0f, 1.0f)` | HUD|Health | EditDefaultsOnly | Health arc color at full health. |
| `AZP_LowHealthColor` | `FLinearColor` | `FLinearColor(0.8f, 0.1f, 0.1f, 1.0f)` | HUD|Health | EditDefaultsOnly | Health arc color the arc lerps toward at low health. |
| `AZP_LowHealthThreshold` | `float` | `0.35f` | HUD|Health | EditDefaultsOnly | Health fraction below which the arc color shifts toward LowHealthColor. |
| `AZP_DamageVignetteFadeSpeed` | `float` | `3.0f` | HUD|Damage | EditDefaultsOnly | How fast the damage vignette fades out after a hit (higher = faster). |
| `AZP_DamageVignetteMaxOpacity` | `float` | `0.8f` | HUD|Damage | EditDefaultsOnly | Peak opacity the damage vignette flashes to when the player takes a hit. |
| `AZP_HealVignetteFadeSpeed` | `float` | `5.0f` | HUD|Effects | EditDefaultsOnly | How fast the heal vignette fades out after a heal flash (higher = faster). |
| `AZP_HealVignetteMaxOpacity` | `float` | `0.6f` | HUD|Effects | EditDefaultsOnly | Peak opacity the heal vignette flashes to on healing. |
| `AZP_EffectVignetteFadeSpeed` | `float` | `3.0f` | HUD|Effects | EditDefaultsOnly | Fade in/out speed shared by the damage-reduction and invincibility status vignettes. |
| `AZP_DamageReductionVignetteMaxOpacity` | `float` | `0.4f` | HUD|Effects | EditDefaultsOnly | Held opacity of the damage-reduction status vignette while the buff is active. |
| `AZP_InvincibilityVignetteMaxOpacity` | `float` | `0.4f` | HUD|Effects | EditDefaultsOnly | Held opacity of the invincibility status vignette while the buff is active. |
| `AZP_GrabPromptGlyphTexture` | `TSoftObjectPtr<UTexture2D>` | `/Game/InventorySystemPro/ExampleContent/Common/Art/Textures/UI/Input/KeyboardMouse/T_IconMouse1.T_IconMouse1` | HUD|Grab | EditDefaultsOnly | Keyboard/mouse attack-button glyph shown above the grab-struggle mash prompt. |
| `AZP_GrabPromptGlyphGamepadTexture` | `TSoftObjectPtr<UTexture2D>` | `/Game/InventorySystemPro/ExampleContent/Common/Art/Textures/UI/Input/GamepadXboxOne/T_XB1_RT.T_XB1_RT` | HUD|Grab | EditDefaultsOnly | Gamepad (Right Trigger) attack-button glyph for the grab-struggle prompt, auto-selected on controller input. |
| `AZP_StaminaArcColor` | `FLinearColor` | `FLinearColor(0.2f, 0.9f, 0.3f, 1.0f)` | HUD|Stamina | EditAnywhere + BP | Color of the stamina arc (green), set on the stamina arc material's ArcColor parameter at construct. |
| `AZP_StaminaArcScale` | `float` | `0.65f` | HUD|Stamina | EditAnywhere + BP | Render scale of the stamina arc so it nests inside the health arc ring (0.65 = 65%). |
| `AZP_GrabPromptIconSize` | `FVector2D` | `FVector2D(52.f, 52.f)` | HUD|Grab | EditAnywhere + BP | On-screen size of the grab-prompt glyph, laid out at runtime because WBP_HUD has no authored slot for it. |
| `AZP_GrabPromptIconOffsetY` | `float` | `-8.f` | HUD|Grab | EditAnywhere + BP | Vertical gap between the grab-prompt glyph and the interaction prompt text it sits above. |
| `AZP_DamageVignetteMaterialAsset` | `TSoftObjectPtr<UMaterialInterface>` | `/Game/Materials/UI/M_DamageVignette` | HUD|Damage | EditAnywhere + BP | Hardcoded LoadObject path for the damage vignette brush material; should become an asset-reference UPROPERTY. |
| `AZP_HealVignetteMaterialAsset` | `TSoftObjectPtr<UMaterialInterface>` | `/Game/Materials/UI/M_HealVignette` | HUD|Effects | EditAnywhere + BP | Hardcoded LoadObject path for the heal vignette brush material; should become an asset-reference UPROPERTY. |
| `AZP_DamageReductionVignetteMaterialAsset` | `TSoftObjectPtr<UMaterialInterface>` | `/Game/Materials/UI/M_DamageReductionVignette` | HUD|Effects | EditAnywhere + BP | Hardcoded LoadObject path for the damage-reduction vignette brush material; should become an asset-reference UPROPERTY. |
| `AZP_InvincibilityVignetteMaterialAsset` | `TSoftObjectPtr<UMaterialInterface>` | `/Game/Materials/UI/M_InvincibilityVignette` | HUD|Effects | EditAnywhere + BP | Hardcoded LoadObject path for the invincibility vignette brush material; should become an asset-reference UPROPERTY. |

### UZP_HealthComponent
_Reusable standalone health-tracking ActorComponent (damage, heal, death, temporary invincibility and damage reduction) attached to creatures and other damageable actors._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_MaxHealth` | `float` | `100.f` | Health | EditAnywhere + BP | Maximum (and starting) health of the owning actor; per-instance editable. |

### UZP_InventoryTabWidget
_Tab controller for the unified inventory menu (Map / Inventory / Notes) — finds Moonville's WBP_InventoryMenu_Horror in the viewport, injects tab buttons into its user-placed TabHeader, and manages tab switching, world-pause, map marker drawing, and notes display._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_TabMarkerSize` | `FVector2D` | `FVector2D(20.0f, 20.0f)` | InventoryTab|Map | EditDefaultsOnly | Pixel size of the rotating player-position chevron marker drawn on the map tab. |
| `AZP_ActiveTabColor` | `FLinearColor` | `FLinearColor(0.85f, 0.85f, 0.85f, 1.0f)` | InventoryTab|Style | EditDefaultsOnly | Text color of the currently selected tab button (MAP / INVENTORY / NOTES). |
| `AZP_InactiveTabColor` | `FLinearColor` | `FLinearColor(0.3f, 0.3f, 0.3f, 0.6f)` | InventoryTab|Style | EditDefaultsOnly | Text color of the non-selected tab buttons. |
| `AZP_InventoryWidgetClass` | `TSubclassOf<UUserWidget>` | `null (auto-loaded at NativeConstruct from /Game/InventorySystemPro/.../WBP_InventoryMenu_Horror if unset)` | InventoryTab | EditDefaultsOnly | Widget class of the Moonville inventory menu, used to find the live instance in the viewport (never created by this widget). |
| `AZP_NotesWidgetClass` | `TSubclassOf<UZP_NotesWidget>` | `null (auto-loaded at NativeConstruct from /Game/EasyGameUI/.../WBP_Notes if unset)` | InventoryTab | EditDefaultsOnly | Widget class instantiated for the Notes tab panel (WBP_Notes, reparented to UZP_NotesWidget). |
| `AZP_DefaultInventoryWidgetClassPath` | `FSoftClassPath` | `/Game/InventorySystemPro/ExampleContent/Horror/UI/Menus/WBP_InventoryMenu_Horror.WBP_InventoryMenu_Horror_C` | InventoryTab | EditAnywhere + BP | Fallback asset path auto-loaded into InventoryWidgetClass when no class is set — swapping the Moonville menu asset currently requires a code edit. |
| `AZP_DefaultNotesWidgetClassPath` | `FSoftClassPath` | `/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes.WBP_Notes_C` | InventoryTab | EditAnywhere + BP | Fallback asset path auto-loaded into NotesWidgetClass when no class is set. |
| `AZP_TabCycleLeftKey` | `FKey` | `EKeys::Q` | InventoryTab|Input | EditAnywhere + BP | Key that cycles to the previous tab while the menu is open (hardcoded Q; used both in NativeOnKeyDown cpp:78 and raw polling cpp:262 — Enhanced Input is blocked in UI mode). |
| `AZP_TabCycleRightKey` | `FKey` | `EKeys::E` | InventoryTab|Input | EditAnywhere + BP | Key that cycles to the next tab while the menu is open (hardcoded E; used in NativeOnKeyDown cpp:84 and raw polling cpp:266). |
| `AZP_MapTabLabel` | `FText` | `"MAP"` | InventoryTab|Text | EditAnywhere + BP | Player-facing label of the Map tab button — house rule says player-facing text must be an exposed FText, never baked in code. |
| `AZP_InventoryTabLabel` | `FText` | `"INVENTORY"` | InventoryTab|Text | EditAnywhere + BP | Player-facing label of the Inventory tab button. |
| `AZP_NotesTabLabel` | `FText` | `"NOTES"` | InventoryTab|Text | EditAnywhere + BP | Player-facing label of the Notes tab button. |
| `AZP_NoMapAvailableText` | `FText` | `"No map available"` | InventoryTab|Text | EditAnywhere + BP | Player-facing message shown on the map tab when the player is outside any MapVolume (repeated at cpp:616, cpp:856, cpp:873 — one knob, three sites). |
| `AZP_MapNotFoundText` | `FText` | `"Map not found yet"` | InventoryTab|Text | EditAnywhere + BP | Player-facing message shown when the player is in a mapped area but has not picked up that area's map item yet. |
| `AZP_UnknownAreaText` | `FText` | `"Unknown Area"` | InventoryTab|Text | EditAnywhere + BP | Player-facing area title shown when no map area can be resolved (sites cpp:853 and cpp:870 — one knob, two sites). |
| `AZP_TabButtonFontSize` | `int32` | `14` | InventoryTab|Style | EditAnywhere + BP | Font size of the injected MAP/INVENTORY/NOTES tab button labels. |
| `AZP_AreaNameFontSize` | `int32` | `20` | InventoryTab|Style | EditAnywhere + BP | Font size of the area display-name header on the map tab. |
| `AZP_NoMapFontSize` | `int32` | `18` | InventoryTab|Style | EditAnywhere + BP | Font size of the 'No map available' / 'Map not found yet' message text. |
| `AZP_PlayerMarkerColor` | `FLinearColor` | `FLinearColor(0.0f, 1.0f, 0.3f, 1.0f)` | InventoryTab|Map | EditAnywhere + BP | Tint color of the player chevron marker on the map (currently green). |

### UZP_KinemationComponent
_ActorComponent encapsulating Kinemation Tactical Shooter Pack integration on the player: weapon spawn/equip/swap, hitscan + shotgun spray, melee view-model (Kubold pipe) and grip blending, throwable grenade, ammo/reserve mirroring, ADS FOV, and camera wiring._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_WeaponClass` | `TSubclassOf<AActor>` | `null` | Kinemation|Weapon | EditDefaultsOnly + BP | Blueprint class of the default weapon spawned at BeginPlay when bAutoSpawnWeapon is true. |
| `bAZP_AutoSpawnWeapon` | `bool` | `true` | Kinemation|Weapon | EditDefaultsOnly + BP | Whether the component spawns WeaponClass automatically during InitializeKinemation (false when inventory manages weapons). |
| `AZP_BulletDecalMaterials` | `TArray<TObjectPtr<UMaterialInterface>>` | `empty array` | Kinemation|Hitscan | EditDefaultsOnly + BP | Bullet-hole decal materials, one picked at random per shot; hitscan early-outs entirely if this array is empty. |
| `AZP_HitscanRange` | `float` | `10000.0f` | Kinemation|Hitscan | EditDefaultsOnly + BP | Maximum range in cm of the gun hitscan line trace (10000 = 100 m). |
| `AZP_DecalSize` | `FVector` | `FVector(2.0f, 3.0f, 3.0f)` | Kinemation|Hitscan | EditDefaultsOnly + BP | Size in cm of spawned bullet-hole decals. |
| `AZP_DecalLifetime` | `float` | `30.0f` | Kinemation|Hitscan | EditDefaultsOnly + BP | Seconds a bullet-hole decal persists before fading out. |
| `AZP_HitscanBodyDamage` | `float` | `10.f` | Kinemation|Hitscan | EditDefaultsOnly + BP | Total per-shot body damage of the current ranged weapon (split across pellets for shotguns); overwritten per weapon by ApplyWeaponConfig. |
| `AZP_HitscanWeakPointDamage` | `float` | `50.f` | Kinemation|Hitscan | EditDefaultsOnly + BP | Total per-shot weak-point (center-mass) damage; overwritten per weapon by ApplyWeaponConfig. |
| `AZP_WeakPointRadius` | `float` | `50.f` | Kinemation|Hitscan | EditDefaultsOnly + BP | Distance in UU from the hit actor's center within which a hit counts as a weak-point hit. |
| `AZP_PistolImpactSound` | `TObjectPtr<USoundBase>` | `/Game/Audio/Weapons/SFX_RANGE_IMPACT_PISTOL (ConstructorHelpers, .cpp:41)` | Kinemation|Hitscan | EditDefaultsOnly + BP | Bullet-impact sound played at the impact point when the equipped weapon icon is Pistol. |
| `AZP_RifleImpactSound` | `TObjectPtr<USoundBase>` | `/Game/Audio/Weapons/SFX_RANGE_IMPACT_RIFLE (ConstructorHelpers, .cpp:45)` | Kinemation|Hitscan | EditDefaultsOnly + BP | Bullet-impact sound played at the impact point when the equipped weapon icon is Rifle. |
| `AZP_ShotgunImpactSound` | `TObjectPtr<USoundBase>` | `/Game/Audio/Weapons/SFX_RANGE_IMPACT_SHOTGUN (ConstructorHelpers, .cpp:49)` | Kinemation|Hitscan | EditDefaultsOnly + BP | Bullet-impact sound played once per trigger pull at the first pellet impact when the icon is Shotgun. |
| `AZP_ShotgunPelletMin` | `int32` | `15` | Kinemation|Hitscan | EditDefaultsOnly + BP | Minimum pellets per shotgun shot (pellet count randomized in [Min,Max] each trigger pull). |
| `AZP_ShotgunPelletMax` | `int32` | `20` | Kinemation|Hitscan | EditDefaultsOnly + BP | Maximum pellets per shotgun shot. |
| `AZP_ShotgunSpreadDegrees` | `float` | `4.0f` | Kinemation|Hitscan | EditDefaultsOnly + BP | Half-angle in degrees of the cone shotgun pellets scatter within. |
| `AZP_MeleeDamage` | `float` | `25.f` | Kinemation|Melee | EditDefaultsOnly + BP | Damage per melee pipe swing; re-set to 25 by ApplyWeaponConfig when the Pipe is equipped. |
| `AZP_MeleeRange` | `float` | `200.f` | Kinemation|Melee | EditDefaultsOnly + BP | Maximum reach in cm of the melee damage sphere sweep. |
| `AZP_MeleeSweepRadius` | `float` | `40.f` | Kinemation|Melee | EditDefaultsOnly + BP | Radius in cm of the melee hit-detection sphere sweep (aim forgiveness). |
| `AZP_MeleeCooldown` | `float` | `0.6f` | Kinemation|Melee | EditDefaultsOnly + BP | Minimum seconds between melee swings (actual cooldown is max of this and the swing duration); re-set to 0.7 by ApplyWeaponConfig on Pipe equip. |
| `AZP_PipeMetalSound` | `TObjectPtr<USoundBase>` | `/Game/Audio/Weapons/SFX_PIPE_HIT (ConstructorHelpers, .cpp:53)` | Kinemation|Melee | EditDefaultsOnly + BP | Metal clang of the pipe itself, always played on any enemy connect. |
| `AZP_MeleeFleshImpactSounds` | `TArray<TObjectPtr<USoundBase>>` | `[SFX_MELEE_IMPACT1, SFX_MELEE_IMPACT2] (ConstructorHelpers, .cpp:61-67)` | Kinemation|Melee | EditDefaultsOnly + BP | Randomized flesh-impact sounds layered over the pipe metal sound on enemy hits. |
| `AZP_PipeWallImpactSound` | `TObjectPtr<USoundBase>` | `/Game/Audio/Weapons/SFX_PIPE_SURFACE_WALL_IMPACT (ConstructorHelpers, .cpp:57)` | Kinemation|Melee | EditDefaultsOnly + BP | Sound of the pipe striking a wall or hard non-enemy surface. |
| `AZP_MeleeSwingRate` | `float` | `1.4f` | Kinemation|Melee | EditDefaultsOnly + BP | Play-rate of the melee swing animation (1.42s source clip reads ~1.0s at 1.4). |
| `AZP_MeleeBlockCancelFraction` | `float` | `0.75f` | Kinemation|Melee | EditDefaultsOnly + BP | Fraction of the swing after which a held block may cancel the return-to-idle tail (1.0 = block always waits for full clip). |
| `AZP_MeleeSwingSound` | `TObjectPtr<USoundBase>` | `/Game/Audio/Weapons/SFX_MELEE_SWING (ConstructorHelpers, .cpp:69)` | Kinemation|Melee | EditDefaultsOnly + BP | Whoosh played 2D at every swing start (own-body foley). |
| `AZP_MeleeSwingVolume` | `float` | `1.f` | Kinemation|Melee | EditAnywhere + BP | Volume multiplier of the swing whoosh (dev tunes this in BP_GraceCharacter KinemationComp Details). |
| `AZP_MeleeEquipRate` | `float` | `2.0f` | Kinemation|Melee | EditDefaultsOnly + BP | Play-rate of the melee equip/raise animation (also used for the throwable rise-from-below). |
| `AZP_MeleeUnequipRate` | `float` | `2.5f` | Kinemation|Melee | EditDefaultsOnly + BP | Play-rate of the melee unequip/lower animation (truncated by the swap window). |
| `AZP_MeleeDamageDelay` | `float` | `0.35f` | Kinemation|Melee | EditDefaultsOnly + BP | Seconds after swing start (post play-rate) when the damage sweep fires — the impact frame. |
| `AZP_MeleeGripOffset` | `FVector` | `FVector(-23.78f, -1.43f, 22.85f)` | Kinemation|Melee | EditAnywhere + BP | Pipe location offset in hand_r bone space (channel-fitted offline by Scripts/Python/fit_pipe_grip_channel_offline.py, which also writes the BP CDO by this name). |
| `AZP_MeleeGripRotation` | `FRotator` | `FRotator(3.15f, 95.41f, 40.23f)` | Kinemation|Melee | EditAnywhere + BP | Pipe rotation in hand_r bone space, applied every frame so Details edits show live in PIE; written by the fit_pipe_grip scripts via set_editor_property. |
| `AZP_BlockGripDeltaLocation` | `FVector` | `FVector::ZeroVector` | Kinemation|Melee | EditAnywhere + BP | Additive hand-local location delta applied to the pipe grip while blocking (zero = block grip equals idle grip). |
| `AZP_BlockGripDeltaRotation` | `FRotator` | `FRotator::ZeroRotator` | Kinemation|Melee | EditAnywhere + BP | Additive hand-local rotation delta applied to the pipe grip while blocking. |
| `AZP_BlockGripBlendSpeed` | `float` | `8.0f` | Kinemation|Melee | EditAnywhere + BP | FInterpTo speed the pipe grip eases between idle and block grips (higher = snappier). |
| `AZP_MeleeHandMaterial` | `TObjectPtr<UMaterialInterface>` | `/Game/Core/Materials/MI_HandSkin (ConstructorHelpers, .cpp:33)` | Kinemation|Melee | EditAnywhere + BP | Material painted on the melee view-model's bare-skin slots (forearm + hand) so FP melee arms read as Marcus skin. |
| `AZP_ThrowableGripOffset` | `FVector` | `FVector(-7.57f, 1.36f, -0.04f)` | Kinemation|Throwable | EditAnywhere + BP | Held grenade location offset in the Kubold view mesh's hand_r bone space (fitted offline by Scripts/Python/fit_grenade_kubold_offline.py; fit_grenade_grip_live.py writes it via set_editor_property). |
| `AZP_ThrowableGripRotation` | `FRotator` | `FRotator(3.15f, 95.41f, 40.23f)` | Kinemation|Throwable | EditAnywhere + BP | Held grenade rotation in hand_r bone space; written by fit_grenade scripts via set_editor_property. |
| `AZP_ThrowableGripScale` | `FVector` | `FVector(0.6f, 0.6f, 1.01f)` | Kinemation|Throwable | EditAnywhere + BP | Held grenade scale (dev-tuned: slimmer, stretched along its long Z axis). |
| `AZP_ThrowableEquipStartFraction` | `float` | `0.5f` | Kinemation|Throwable | EditDefaultsOnly + BP | Fraction into the Kubold Equip anim where grenade equip starts (skips the pipe-draw first half). |
| `AZP_GrenadeProjectileClass` | `TSubclassOf<AActor>` | `null (defaulted to AZP_GrenadeProjectile::StaticClass() in InitializeKinemation, .cpp:239-242)` | Kinemation|Throwable | EditDefaultsOnly + BP | Actor class spawned when a grenade is thrown. |
| `AZP_ThrowSpeed` | `float` | `800.f` | Kinemation|Throwable | EditDefaultsOnly + BP | Intended initial speed of the thrown projectile in cm/s — currently UNUSED: ThrowProjectile never reads it (the projectile's own ProjectileMovement sets speed). |
| `AZP_MeleeLightAnims` | `TArray<TObjectPtr<UAnimSequenceBase>>` | `empty (hard-loaded at init: /Game/TheSignal/Animations/Melee/A_MeleePipe_Attack_R + _L, .cpp:161-170)` | Kinemation|Animation | EditAnywhere + BP | Melee swing animations cycled per swing (R then L); asset config, but InitializeKinemation Reset()s and re-loads them by hardcoded path every init — the load must be made conditional before an editor-set value can stick. |
| `AZP_MeleeIdleAnim` | `TObjectPtr<UAnimSequenceBase>` | `null (hard-loaded: /Game/TheSignal/Animations/Melee/A_MeleePipe_Idle, .cpp:171-172)` | Kinemation|Animation | EditAnywhere + BP | Melee view-model idle loop; overwritten by hardcoded-path load in InitializeKinemation. |
| `AZP_MeleeEquipAnim` | `TObjectPtr<UAnimSequenceBase>` | `null (hard-loaded: /Game/TheSignal/Animations/Melee/A_MeleePipe_Equip, .cpp:173-174)` | Kinemation|Animation | EditAnywhere + BP | Melee view-model equip/raise animation; overwritten by hardcoded-path load in InitializeKinemation. |
| `AZP_MeleeUnequipAnim` | `TObjectPtr<UAnimSequenceBase>` | `null (hard-loaded: /Game/TheSignal/Animations/Melee/A_MeleePipe_Unequip, .cpp:175-176)` | Kinemation|Animation | EditAnywhere + BP | Melee view-model unequip/lower animation; overwritten by hardcoded-path load in InitializeKinemation. |
| `AZP_GrenadeThrowAnim` | `TObjectPtr<UAnimSequenceBase>` | `null (hard-loaded: /Game/Animations/FPS/AM_FP_GrenadeThrow, .cpp:177-178)` | Kinemation|Animation | EditAnywhere + BP | Grenade throw animation loaded by path at init (currently unused in play — no throw gesture by dev call, .cpp:1617); overwritten every init. |
| `AZP_MagSize` | `int32` | `12` | Kinemation|Ammo | EditDefaultsOnly + BP | Rounds per magazine of the current weapon; overwritten per weapon by ApplyWeaponConfig. |
| `AZP_DefaultFOV` | `float` | `90.0f` | Kinemation|ADS | EditAnywhere + BP | Hip-fire field of view — but overwritten from UZP_GraceMovementConfig.DefaultFOV in AZP_GraceCharacter (ZP_GraceCharacter.cpp:525); the DataAsset is the real knob, this is the mirror. |
| `AZP_AdsFOV` | `float` | `65.0f` | Kinemation|ADS | EditAnywhere + BP | Generic ADS field of view — overwritten from MovementConfig (ZP_GraceCharacter.cpp:526) and no longer read by SetAiming (superseded by the per-weapon AdsFOVPistol/Shotgun/Rifle). |
| `AZP_AdsFOVPistol` | `float` | `90.0f` | Kinemation|ADS | EditAnywhere + BP | ADS field of view while aiming the pistol (90 = no FOV change; wider counteracts the aim pose's zoom). |
| `AZP_AdsFOVShotgun` | `float` | `98.0f` | Kinemation|ADS | EditAnywhere + BP | ADS field of view while aiming shotguns. |
| `AZP_AdsFOVRifle` | `float` | `98.0f` | Kinemation|ADS | EditAnywhere + BP | ADS field of view while aiming rifles. |
| `AZP_AdsFOVInterpSpeed` | `float` | `10.0f` | Kinemation|ADS | EditAnywhere + BP | Interpolation speed of ADS FOV transitions; overwritten from MovementConfig (ZP_GraceCharacter.cpp:527). |
| `AZP_FireCooldownTime` | `float` | `0.25f` | Kinemation|Weapon | EditDefaultsOnly + BP | Minimum seconds between shots (semi-auto lockout); overwritten per weapon by ApplyWeaponConfig. |
| `AZP_WeaponDrawLockTime` | `float` | `0.6f` | Kinemation|Weapon | EditDefaultsOnly + BP | Fire-input lock duration while the weapon Draw montage plays after a swap. |
| `AZP_SwapHeadHideTime` | `float` | `2.5f` | Kinemation|Weapon | EditDefaultsOnly + BP | Seconds the head region stays render-hidden after a weapon swap; must outlast the longest draw animation (1.8 flashed the head, dev-caught). |
| `AZP_ReloadTime` | `float` | `3.0f` | Kinemation|Weapon | EditDefaultsOnly + BP | Mag-swap reload duration for pistols/rifles. |
| `AZP_ShellReloadEmptyStartTime` | `float` | `2.7f` | Kinemation|Weapon | EditDefaultsOnly + BP | Shell-loader reload start phase duration when the mag is empty (measured from the Herrington reload anims). |
| `AZP_ShellReloadTacStartTime` | `float` | `0.7f` | Kinemation|Weapon | EditDefaultsOnly + BP | Shell-loader reload start phase duration on a tactical (non-empty) reload. |
| `AZP_ShellReloadLoopTime` | `float` | `0.92f` | Kinemation|Weapon | EditDefaultsOnly + BP | Seconds per shell inserted during a shell-by-shell shotgun reload. |
| `AZP_ShellReloadEndTime` | `float` | `0.85f` | Kinemation|Weapon | EditDefaultsOnly + BP | Shell-loader reload end phase duration (closing the action). |
| `AZP_WeaponStatsTable` | `per-weapon literals (int32/float)` | `Viper: mag 12, cooldown 0.25, dmg 10/50; Herrington: 6, 0.8, 30/150, shell; AK105: 30, 0.15, 15/60; TR15: 20, 0.15, 18/70; SRM: 8, 0.7, 30/150, shell; Pipe: dmg 25, cd 0.7; Grenade: mag 1` | Kinemation|Weapon | internal (C++ only) | THE per-weapon balance table (MagSize, FireCooldownTime, HitscanBodyDamage, HitscanWeakPointDamage, bShellReload) hardcoded as name-Contains branches in ApplyWeaponConfig (.cpp:615-689); prime candidate for a struct/DataTable knob. The per-weapon ReserveAmmo seeds there are dead code (comment .cpp:721-724). |
| `AZP_ReserveAmmoCaps` | `int32 x3` | `Pistol 48 / Shotgun 24 / Rifle 90` | Kinemation|Ammo | internal (C++ only) | Per-icon maximum reserve (inventory ammo) the player may carry, hardcoded in the static GetReserveCapForIcon (.cpp:756-765); note it is a STATIC function today, so exposing requires moving the caps to instance properties. |
| `AZP_GunshotAlertRadius` | `float` | `8000.f` | Kinemation|Hitscan | EditAnywhere + BP | Radius in cm within which a gunshot alerts all nearby creatures via BroadcastGunshot (creatures can override per-instance with their own GunshotAlertRadius). |
| `AZP_MeleeNoiseAlertRadius` | `float` | `2000.f` | Kinemation|Melee | EditAnywhere + BP | Radius in cm within which a melee impact alerts nearby creatures (quieter than gunshots). |
| `AZP_ThrowCooldownTime` | `float` | `0.8f` | Kinemation|Throwable | EditAnywhere + BP | Cooldown in seconds after throwing a grenade before the next throw is allowed. |
| `AZP_ThrowSpawnForwardOffset` | `float` | `100.f` | Kinemation|Throwable | EditAnywhere + BP | Distance in cm in front of the camera where the thrown grenade spawns. |
| `AZP_WeaponAttachSocketName` | `FName` | `"VB ik_hand_gun"` | Kinemation|Weapon | EditAnywhere + BP | PlayerMesh socket the spawned ranged weapon attaches to (Kinemation virtual-bone gun socket). |
| `AZP_DecalFadeDuration` | `float` | `2.0f` | Kinemation|Hitscan | EditAnywhere + BP | Seconds a bullet decal takes to fade out at the end of DecalLifetime (SetFadeOut(DecalLifetime - 2.0, 2.0)). |
| `AZP_MeleeUnequipHideDelay` | `float` | `0.5f` | Kinemation|Melee | EditAnywhere + BP | Seconds after the melee unequip/lower anim starts before the view model is hidden (the swap drop window; matches EquipWeaponClass phase 2). |
| `AZP_MeleeViewModelMeshAsset` | `USkeletalMesh* (path literal)` | `/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono` | Kinemation|Melee | EditAnywhere + BP | Skeletal mesh assigned to the melee/throwable view model (Operator body, proven pose; CCMH swap reverted 2026-06-20 — keep the default). |
| `AZP_MeleeWeaponMeshAsset` | `UStaticMesh* (path literal)` | `/Game/InventorySystemPro/ExampleContent/Common/Art/Pipe/SM_Pipe` | Kinemation|Melee | EditAnywhere + BP | Static mesh of the pipe held by the melee view model (Moonville example-content pipe). |

### UZP_LipSyncComponent
_OVRLipSync-driven NPC lip sync: listens on the main audio submix for dialogue PCM, runs viseme analysis, swaps the CC head SkeletalMeshComponent for a PoseableMeshComponent and drives jaw/lip/mouth-corner bones directly._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_VisemeToJaw` | `static const float[15]` | `{0.00,0.15,0.35,0.45,0.60,0.55,0.50,0.30,0.20,0.50,1.00,0.70,0.55,0.80,0.60}` | LipSync|Visemes | internal (C++ only) | Per-viseme jaw-open weight mapping (index order sil,PP,FF,TH,DD,kk,CH,SS,nn,RR,aa,E,ih,oh,ou) that shapes how far the jaw opens for each detected phoneme; expose as a TArray<float> or 15-element fixed array UPROPERTY. |
| `AZP_VisemeToLipUp` | `static const float[15]` | `{0.00,0.70,0.50,0.10,0.00,0.00,0.00,0.00,0.10,0.00,0.00,0.00,0.00,0.00,0.30}` | LipSync|Visemes | internal (C++ only) | Per-viseme lower-lip raise weight mapping (bilabials/labiodentals PP, FF, ou dominate) controlling how much the lower lips curl up for each phoneme. |
| `AZP_VisemeToWidth` | `static const float[15]` | `{0.00,-0.40,-0.15,0.00,0.00,0.00,0.15,0.25,0.00,0.00,0.50,0.60,0.50,-0.50,-0.60}` | LipSync|Visemes | internal (C++ only) | Per-viseme mouth-width mapping (positive = spread as in E/ih/aa, negative = purse as in oh/ou/PP) controlling corner spread vs purse per phoneme. |
| `AZP_JawOpenInterpSpeed` | `float` | `25.f` | LipSync|Smoothing | EditAnywhere + BP | FInterpTo speed used when the jaw is opening (target above current) - higher snaps the mouth open faster on syllable onsets. |
| `AZP_JawCloseInterpSpeed` | `float` | `8.f` | LipSync|Smoothing | EditAnywhere + BP | FInterpTo speed used when the jaw is closing (target below current) - lower gives a softer, lazier mouth close between syllables. |
| `AZP_LipUpInterpSpeed` | `float` | `18.f` | LipSync|Smoothing | EditAnywhere + BP | FInterpTo smoothing speed for the lower-lip raise channel (bilabial lip press response speed). |
| `AZP_MouthWidthInterpSpeed` | `float` | `15.f` | LipSync|Smoothing | EditAnywhere + BP | FInterpTo smoothing speed for the mouth-width (spread/purse) channel. |
| `AZP_MaxJawOpenAngleDeg` | `float` | `25.f` | LipSync|BonePose | EditAnywhere + BP | Maximum jaw bone rotation in degrees at full jaw-open weight - the master 'how wide does the mouth open' knob. |
| `AZP_MaxLipRaiseAngleDeg` | `float` | `-12.f` | LipSync|BonePose | EditAnywhere + BP | Rotation in degrees applied to lip_lower_l/r bones at full lip-raise weight (negative = curl upward) for PP/FF bilabial shapes. |
| `AZP_MaxMouthWidthAngleDeg` | `float` | `8.f` | LipSync|BonePose | EditAnywhere + BP | Rotation in degrees applied to mouth_l/r corner bones at full width weight (mirrored +/- between sides) for spread vs purse shapes. |
| `AZP_JawBoneName` | `FName` | `"jaw"` | LipSync|Bones | EditAnywhere + BP | Skeleton bone name driven for jaw open; also the bone whose presence auto-identifies the CC head mesh (used at ZP_LipSyncComponent.cpp:169 in FindMeshWithJawBone and ZP_LipSyncComponent.cpp:448 in ApplyVisemes). |
| `AZP_LipLowerRightBoneName` | `FName` | `"lip_lower_r"` | LipSync|Bones | EditAnywhere + BP | Skeleton bone name for the right lower-lip bone driven by the lip-raise channel. |
| `AZP_LipLowerLeftBoneName` | `FName` | `"lip_lower_l"` | LipSync|Bones | EditAnywhere + BP | Skeleton bone name for the left lower-lip bone driven by the lip-raise channel. |
| `AZP_MouthRightBoneName` | `FName` | `"mouth_r"` | LipSync|Bones | EditAnywhere + BP | Skeleton bone name for the right mouth-corner bone driven by the width channel. |
| `AZP_MouthLeftBoneName` | `FName` | `"mouth_l"` | LipSync|Bones | EditAnywhere + BP | Skeleton bone name for the left mouth-corner bone driven by the width channel (rotation applied mirrored/negated vs the right corner). |

### UZP_MapWidget
_Full-screen RE/Silent-Hill style map overlay: shows the current area's floor-plan texture, a rotating player marker, and color-coded door markers; toggled with the M key._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_MarkerSize` | `FVector2D` | `FVector2D(12.0f, 12.0f)` | Map|Marker | EditDefaultsOnly | Pixel size of the player position marker on the map. |
| `AZP_DoorMarkerSize` | `FVector2D` | `FVector2D(8.0f, 8.0f)` | Map|Doors | EditDefaultsOnly | Pixel size of the door marker icons on the map. |
| `AZP_UnlockedDoorColor` | `FLinearColor` | `FLinearColor(0.2f, 0.4f, 1.0f, 1.0f)` | Map|Doors | EditDefaultsOnly | Marker color for unlocked/interactable doors (RE-style blue). |
| `AZP_LockedDoorColor` | `FLinearColor` | `FLinearColor(1.0f, 0.15f, 0.15f, 1.0f)` | Map|Doors | EditDefaultsOnly | Marker color for locked doors (RE-style red). |
| `AZP_PlayerMarkerAngleOffset` | `float` | `90.0f` | Map|Marker | EditAnywhere + BP | Degrees added to the negated control yaw so the player-marker art points the correct way; must change if the marker texture's authored orientation changes. |
| `AZP_UnknownAreaText` | `FText` | `TEXT("Unknown Area")` | Map|Text | EditAnywhere + BP | Player-facing area title shown when no MapVolume covers the player (hardcoded UI string; violates the expose-player-facing-text rule). |
| `AZP_NoMapAvailableText` | `FText` | `TEXT("No map available")` | Map|Text | EditAnywhere + BP | Player-facing message shown when no map exists for the current location (hardcoded UI string; violates the expose-player-facing-text rule). |
| `AZP_MapNotFoundText` | `FText` | `TEXT("Map not found yet")` | Map|Text | EditAnywhere + BP | Player-facing message shown when the area has a map but the player has not picked it up yet (hardcoded UI string; violates the expose-player-facing-text rule). |

### UZP_MarcusBodySpineBendAnimInstance
_Post-process AnimInstance for the visible CCMH first-person body: bends the spine forward when the camera pitches down (so the camera never sees inside the chest) and applies grab arm-pose fix rotations during Shambler grapple phases._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_SpineBendWeights` | `float[5] (per-vertebra weights inside a local struct array)` | `spine_01=0.10, spine_02=0.15, spine_03=0.20, spine_04=0.25, spine_05=0.30` | Appearance|SpineBend | internal (C++ only) | Per-vertebra distribution of the total look-down spine bend across CCMH spine_01..spine_05, weighted toward the head so the chest closes to the camera faster than the waist. |

### UZP_NPCInteractionComponent
_Drop-in ActorComponent making any actor dialogue-interactable: creates an overlap volume, shows HUD prompt, routes to CodeSpartan DialoguePlugin (reflection) or the custom ZP_DialogueManager fallback, and drives face-player rotation + gesture montages during dialogue._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_PluginDialogue` | `TObjectPtr<UDataAsset>` | `null` | NPC|Dialogue Plugin | EditAnywhere + BP | The CodeSpartan Dialogue Plugin dialogue asset this NPC plays when interacted with (preferred path). |
| `AZP_DialogueWidgetClass` | `TSubclassOf<UUserWidget>` | `/DialoguePlugin/UI/DemoDialogueWidget (set via ConstructorHelpers::FClassFinder in ctor, ZP_NPCInteractionComponent.cpp:30-35)` | NPC|Dialogue Plugin | EditAnywhere + BP | Widget class (extending the plugin's DialogueUserWidget) created on interact to display the dialogue UI. |
| `AZP_DialogueData` | `TObjectPtr<UZP_DialogueData>` | `null` | NPC|Custom Dialogue | EditAnywhere + BP | Custom-system dialogue DataAsset used as fallback when PluginDialogue is not set (played via ZP_DialogueManager). |
| `AZP_InteractionPrompt` | `FText` | `FText::FromString(TEXT("Talk"))` | NPC | EditAnywhere + BP | HUD prompt text shown when the player is inside this NPC's interaction volume (e.g. Talk, Examine). |
| `bAZP_InteractOnce` | `bool` | `false` | NPC | EditAnywhere + BP | If true, the NPC can only be interacted with once — interaction disables after first use. |
| `AZP_CharacterSaveName` | `FName` | `NAME_None` | Character Designer | EditAnywhere + BP | Name of the saved Character Customizer design to load onto this NPC (dropdown fed by GetSavedCharacterNames). |
| `AZP_GestureAnimations` | `TArray<TObjectPtr<UAnimSequenceBase>>` | `empty array` | NPC|Gestures | EditAnywhere + BP | Pool of gesture AnimSequences randomly played as dynamic montages while dialogue audio is playing. |
| `AZP_FacePlayerSpeed` | `float` | `5.f` | NPC|Behavior | EditAnywhere + BP | Interp speed at which the NPC rotates (yaw only) to face the player during dialogue. |
| `AZP_InteractionVolumeExtent` | `FVector` | `FVector(300.f, 300.f, 150.f)` | NPC|Interaction | EditAnywhere + BP | Half-extents of the runtime-created box overlap volume that defines the NPC's interaction range. |
| `AZP_GestureInitialDelayMin` | `float` | `2.f` | NPC|Gestures | EditAnywhere + BP | Minimum seconds after dialogue starts before the first random gesture can play (GestureTimer = RandRange(2,5)). |
| `AZP_GestureInitialDelayMax` | `float` | `5.f` | NPC|Gestures | EditAnywhere + BP | Maximum seconds after dialogue starts before the first random gesture can play (GestureTimer = RandRange(2,5)). |
| `AZP_GestureIntervalMin` | `float` | `3.f` | NPC|Gestures | EditAnywhere + BP | Minimum seconds between consecutive random gesture montages during dialogue (GestureTimer = RandRange(3,7)). |
| `AZP_GestureIntervalMax` | `float` | `7.f` | NPC|Gestures | EditAnywhere + BP | Maximum seconds between consecutive random gesture montages during dialogue (GestureTimer = RandRange(3,7)). |
| `AZP_GestureBlendTime` | `float` | `0.25f` | NPC|Gestures | EditAnywhere + BP | Blend-in/blend-out time (both 0.25f in PlaySlotAnimationAsDynamicMontage) for gesture montages; the same 0.25f is also used for StopAllMontages blend-out at ZP_NPCInteractionComponent.cpp:335 and :379. |
| `AZP_GestureSlotName` | `FName` | `FName("DefaultSlot")` | NPC|Gestures | EditAnywhere + BP | Montage slot name gesture animations are played into; must match a slot in the NPC's AnimBP. |

### UZP_NoteEntryWidget
_Single clickable note entry in the Notes tab list; broadcasts its note index on click and swaps title/background colors and a folder-tab indent for selected/unselected state._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_SelectedColor` | `FLinearColor` | `FLinearColor(0.92f, 0.92f, 0.92f, 1.0f)` | Notes|Style | EditDefaultsOnly | Title text color for the currently selected note entry. |
| `AZP_UnselectedColor` | `FLinearColor` | `FLinearColor(0.45f, 0.45f, 0.45f, 0.9f)` | Notes|Style | EditDefaultsOnly | Title text color for unselected note entries. |
| `AZP_SelectedBgColor` | `FLinearColor` | `FLinearColor(0.05f, 0.16f, 0.12f, 1.0f)` | Notes|Style | EditDefaultsOnly | Button background color (teal accent) when the entry is selected. |
| `AZP_UnselectedBgColor` | `FLinearColor` | `FLinearColor(0.04f, 0.04f, 0.04f, 0.6f)` | Notes|Style | EditDefaultsOnly | Button background color when the entry is unselected (also applied in NativeConstruct as the initial state). |
| `AZP_SelectedLeftMargin` | `float` | `14.0f` | Notes|Style | EditDefaultsOnly | Left slot padding (folder-tab indent) applied when the entry is selected. |
| `AZP_UnselectedLeftMargin` | `float` | `0.0f` | Notes|Style | EditDefaultsOnly | Left slot padding applied when the entry is unselected. |
| `AZP_UnselectedRightMargin` | `float` | `6.0f` | Notes|Style | EditAnywhere + BP | Right slot padding applied when the entry is unselected (selected uses 0) - the mirror half of the folder-tab indent that is currently hardcoded while its left-margin siblings are knobs. |

### UZP_NotesWidget
_Two-panel notes/log viewer widget (Notes tab): scrollable note list on the left, note content on the right, fed by UZP_NoteComponent; WBP_Notes extends it._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_NoteEntryWidgetClass` | `TSubclassOf<UZP_NoteEntryWidget>` | `null (auto-loads /Game/EasyGameUI/EasyOptionsMenu/Core/WBP_NoteEntry.WBP_NoteEntry_C at NativeConstruct if unset)` | Notes | EditDefaultsOnly | Widget class instantiated for each row in the note list (expected WBP_NoteEntry). |
| `AZP_NotesEmptyFallbackText` | `FText` | `"No notes collected" (same literal repeated at ZP_NotesWidget.cpp:55 and ZP_NotesWidget.cpp:69)` | Notes|Text | EditAnywhere + BP | Player-facing empty-state message shown when no notes are collected; per the project's expose-player-facing-text rule this must become an editable FText knob, not authored in C++. |

### UZP_ObjectiveDepositLibrary
_Static BlueprintFunctionLibrary implementing objective-deposit logic (grid sizing, validation, completion flag, status light, interaction lock, menu text) for the Moonville BP_ObjectiveContainer via reflection._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_GDefaultLightIntensity` | `float` | `300.f` | TheSignal|ObjectiveDeposit | internal (C++ only) | Fallback brightness (candelas) of the objective-container status light when the container BP has no StatusLightIntensity variable; the per-instance knob already exists as a BP var read by reflection. |
| `AZP_GStatusLightRadius` | `float` | `300.f` | TheSignal|ObjectiveDeposit | internal (C++ only) | Attenuation radius (UU) of the runtime-spawned objective-container status point light. |
| `AZP_GStatusLightHeight` | `float` | `80.f` | TheSignal|ObjectiveDeposit | internal (C++ only) | Z offset (UU) above the container root at which the status light is attached. |

### UZP_ObjectiveHudBridge
_ActorComponent glue between UZP_ObjectiveSubsystem and the EasyGameUI quest HUD widget: creates/holds the quest widget in C++ and drives BP-implementable Begin/AddRow/End/Hide events; auto-fades the tracker after a show window._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_QuestWidgetClass` | `TSubclassOf<UUserWidget>` | `null (set to WBP_EHB_QuestStatusDisplayer in BPC_ObjectiveHudGlue defaults)` | Objectives | EditAnywhere + BP | The EasyGameUI quest module widget class to instantiate for the objective tracker HUD. |
| `AZP_ShowDuration` | `float` | `8.f` | Objectives | EditAnywhere + BP | Seconds the objective tracker stays on screen after a show event (menu close / level load / objective update) before fading out. |

### UZP_ObjectiveSubsystem
_GameInstance subsystem: campaign progression backbone — loads objective definitions from DT_Objectives, tracks active/completed objectives, sub-objectives, stages and flags, auto-advances via requirement evaluation, and persists via UZP_SaveGame._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_ObjectiveStateSlot` | `FString` | `TEXT("TheSignal_Objectives")` | Objectives|Save | EditAnywhere + BP | Name of the dedicated SaveGame slot used by SaveObjectiveState/LoadObjectiveState for self-contained objective persistence (separate from the player's manual save slots). |
| `bAZP_AutoPersist` | `bool` | `false` | Objectives|Save | EditAnywhere + BP | Feature toggle: when true, objective state auto-saves to ObjectiveStateSlot on every change and auto-restores at game start; when false, persistence is driven only by the EasyGameUI save hook (save-file-tied, fresh PIE starts clean). |
| `AZP_DefinitionsTablePath` | `FString (proposed FSoftObjectPath/UDataTable* knob)` | `TEXT("/Game/Data/DT_Objectives.DT_Objectives")` | Objectives|Data | EditAnywhere + BP | Asset path of the objective-definitions DataTable loaded by LoadDefinitions (also hardcoded in the warning log at ZP_ObjectiveSubsystem.cpp:37); the same path is baked/imported by Scripts/Python/bake_objectives.py, import_objectives_dt.py, _verify_dt.py, _verify_obj_stages.py — keep the DEFAULT VALUE identical if promoted. |

### UZP_PatrolComponent
_EnemyAI component that cycles a spawned invisible waypoint actor through patrol points and switches to chase on PawnSensing sight, driving Procedural Monsters v2's BPC_3D_Pathfinding via reflection._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_PatrolPoints` | `TArray<FVector>` | `empty array` | Patrol | EditAnywhere + BP | World-space waypoints the creature cycles through in order while patrolling; set per-instance in the level. |
| `AZP_ArrivalThreshold` | `float` | `300.0f` | Patrol | EditAnywhere + BP | Distance (UU) at which the creature counts as arrived at a waypoint and advances to the next one. |
| `AZP_PatrolCheckInterval` | `float` | `0.5f` | Patrol | EditAnywhere + BP | Seconds between checks of whether the creature has arrived at the current waypoint. |
| `AZP_PatrolSeed` | `int32` | `-1` | Patrol | EditAnywhere + BP | If >= 0, overrides the creature's Monster Randomizer seed for deterministic appearance; never read by this component's C++, so a Blueprint graph likely reads it by name. |
| `bAZP_AutoInitialize` | `bool` | `true` | Patrol | EditAnywhere + BP | If true, InitializePatrol() is called automatically from BeginPlay; set false to defer initialization to a custom trigger. |

### UZP_RuntimeISMBatcher
_Runtime draw-call reducer: scans StaticMeshActors, groups them by (floor, mesh, material set), builds per-floor InstancedStaticMeshComponents, hides originals (keeping collision), and exposes per-floor visibility for the floor culling system. PIE-only benefit._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_MinInstanceCount` | `int32` | `10` | ISM Batcher | EditAnywhere + BP | Minimum number of identical mesh+material instances on a floor before they get batched into one ISM; groups below this stay as individual actors. |
| `AZP_FloorHeight` | `float` | `500.0f` | ISM Batcher|Floors | internal (C++ only) | Height in UU of one building floor, used to bucket actors into floor indices by Z; NOT the authoring point - it is overwritten at runtime from UZP_FloorCullingComponent.FloorHeight by ZP_GraceCharacter.cpp:684, so it stays a plain synced member. |
| `AZP_FloorBaseZ` | `float` | `0.0f` | ISM Batcher|Floors | internal (C++ only) | World Z of the bottom of floor 0 for the floor-index calculation; synced at runtime from UZP_FloorCullingComponent (ZP_GraceCharacter.cpp:685) - canonical knob lives on the culling component, this is the mirror. |
| `AZP_NumFloors` | `int32` | `5` | ISM Batcher|Floors | internal (C++ only) | Number of floor buckets for per-floor ISM arrays and Z clamping; synced at runtime from UZP_FloorCullingComponent (ZP_GraceCharacter.cpp:686) - mirror of the culling component's knob. |
| `AZP_AlwaysVisibleZones` | `TArray<FBox>` | `empty` | ISM Batcher|Floors | internal (C++ only) | World-space boxes (stairwells etc.) whose actors are never batched so floor culling cannot hide them; synced at runtime from UZP_FloorCullingComponent (ZP_GraceCharacter.cpp:687). |

### UZP_SFXPropagationSubsystem
_Tickable world subsystem that re-evaluates propagation (occlusion volume/low-pass) for every live world SFX several times a second and smoothly interpolates each audio component toward the new targets._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_RetargetInterval` | `float (static constexpr)` | `0.15f` | Audio|Propagation | EditAnywhere + BP | Seconds between full propagation re-evaluations (one trace + nav query per live sound) - lower = more responsive muffling, higher = cheaper. |
| `AZP_InterpSpeed` | `float (static constexpr)` | `9.f` | Audio|Propagation | EditAnywhere + BP | Per-second interpolation speed of volume and low-pass toward their propagation targets - controls how fast a sound muffles/opens as the listener moves. |

### UZP_SFXStatics
_Static BlueprintFunctionLibrary that is THE single playback path for world 3D SFX: C++-owned carry/attenuation profiles (Close/Room/Far), distance feel (LPF shelf + reverb send), facility reverb bed, and the manual 3-tier Direct/Diffracted/Transmitted propagation model._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_CloseInnerRadius` | `float` | `100.f` | SFX|Carry | internal (C++ only) | Full-volume inner radius (UU) of the Close carry profile (small foley, ~30 m total reach). |
| `AZP_CloseFalloff` | `float` | `2900.f` | SFX|Carry | internal (C++ only) | Falloff distance (UU) past the inner radius for the Close carry profile - sub-audible past ~30 m. |
| `AZP_RoomInnerRadius` | `float` | `150.f` | SFX|Carry | internal (C++ only) | Full-volume inner radius (UU) of the Room carry profile (doors, machines, ~60 m reach). |
| `AZP_RoomFalloff` | `float` | `5850.f` | SFX|Carry | internal (C++ only) | Falloff distance (UU) for the Room carry profile - sub-audible past ~60 m. |
| `AZP_FarInnerRadius` | `float` | `150.f` | SFX|Carry | internal (C++ only) | Full-volume inner radius (UU) of the Far carry profile (enemy voices, gunshots, ~80 m reach). |
| `AZP_FarFalloff` | `float` | `7850.f` | SFX|Carry | internal (C++ only) | Falloff distance (UU) for the Far carry profile - sub-audible past ~80 m; also (with FarInnerRadius) bounds the propagation early-out range. |
| `AZP_LPFStartFraction` | `float` | `0.35f` | SFX|DistanceFeel | internal (C++ only) | Fraction of a profile's max range at which the gentle air-absorption top-end shelf begins. |
| `AZP_LPFFrequencyAtFar` | `float` | `7000.f` | SFX|DistanceFeel | internal (C++ only) | Low-pass cutoff (Hz) at a sound's maximum range - a shelf for air absorption, deliberately NOT a muffle. |
| `AZP_ReverbWetNear` | `float` | `0.08f` | SFX|DistanceFeel | internal (C++ only) | Reverb send wet level for close sounds (subtle tail); near = mostly dry. |
| `AZP_ReverbWetFar` | `float` | `0.7f` | SFX|DistanceFeel | internal (C++ only) | Reverb send wet level at max range - distant sounds become echo-dominant (why far actually SOUNDS far). |
| `AZP_ReverbMasterVolume` | `float` | `0.35f` | SFX|ReverbBed | internal (C++ only) | Master volume of the level-wide facility (concrete corridor) reverb bed activated by EnsureWorldReverb. |
| `AZP_ReverbDecayTime` | `float` | `2.3f` | SFX|ReverbBed | internal (C++ only) | Reverb tail decay time in seconds - long concrete-corridor tail. |
| `AZP_ReverbDecayHFRatio` | `float` | `0.55f` | SFX|ReverbBed | internal (C++ only) | High-frequency decay ratio - below 1 means concrete keeps the low end ringing longer than the highs. |
| `AZP_ReverbGain` | `float` | `0.32f` | SFX|ReverbBed | internal (C++ only) | Overall reverb effect gain of the facility bed. |
| `AZP_ReverbGainHF` | `float` | `0.55f` | SFX|ReverbBed | internal (C++ only) | High-frequency gain of the facility reverb bed. |
| `AZP_ReverbReflectionsGain` | `float` | `0.14f` | SFX|ReverbBed | internal (C++ only) | Early reflections gain of the facility reverb bed. |
| `AZP_ReverbReflectionsDelay` | `float` | `0.012f` | SFX|ReverbBed | internal (C++ only) | Early reflection delay in seconds - tuned to hallway-width early slap. |
| `AZP_ReverbLateGain` | `float` | `1.1f` | SFX|ReverbBed | internal (C++ only) | Late reverberation gain of the facility bed. |
| `AZP_ReverbLateDelay` | `float` | `0.02f` | SFX|ReverbBed | internal (C++ only) | Late reverberation delay in seconds. |
| `AZP_ReverbDiffusion` | `float` | `0.85f` | SFX|ReverbBed | internal (C++ only) | Echo density / diffusion of the facility reverb tail. |
| `AZP_ReverbDensity` | `float` | `1.0f` | SFX|ReverbBed | internal (C++ only) | Modal density of the facility reverb tail. |
| `AZP_MaxDiffractionDetour` | `float` | `2.4f` | SFX|Propagation | internal (C++ only) | Nav-path-to-straight-line ratio above which an around-the-corner route stops being believable and the sound counts as through-wall (Transmitted). |
| `AZP_DiffractedVolumeMin` | `float` | `1.0f` | SFX|Propagation | internal (C++ only) | Diffracted volume multiplier at detour ratio ~1 (indistinguishable from Direct - seamless blend). |
| `AZP_DiffractedVolumeMax` | `float` | `0.5f` | SFX|Propagation | internal (C++ only) | Diffracted volume multiplier at the longest believable detour (several corners). Note: despite the Max name this is the QUIETER end of the lerp. |
| `AZP_DiffractedLPFMinHz` | `float` | `15000.f` | SFX|Propagation | internal (C++ only) | Low-pass cutoff at detour ratio ~1 - effectively inaudible filtering (top end survives around corners). |
| `AZP_DiffractedLPFMaxHz` | `float` | `3200.f` | SFX|Propagation | internal (C++ only) | Low-pass cutoff at the longest believable detour - mild HF loss, never a pillow-muffle. |
| `AZP_TransmittedVolumeScale` | `float` | `0.3f` | SFX|Propagation | internal (C++ only) | Volume multiplier for genuinely through-wall (Transmitted) sounds. |
| `AZP_TransmittedLowPassHz` | `float` | `500.f` | SFX|Propagation | internal (C++ only) | Heavy low-pass cutoff (Hz) for through-wall sounds - the one place a real muffle is correct physics. |
| `AZP_OcclusionSelfSkin` | `float` | `150.f` | SFX|Propagation | internal (C++ only) | Distance (UU) from the source within which a trace blocker counts as the source's own perch/contact geometry (e.g. a crawler clinging to a wall) rather than an occluding wall. |
| `AZP_ReverbBedFadeTime` | `float` | `2.f` | SFX|ReverbBed | internal (C++ only) | Fade-in time (s) of the facility reverb bed when activated (ActivateReverbEffect FadeTime arg) - should be hoisted to a named header constant alongside the other reverb bed values. |

### UZP_ShamblerBehaviorComponent
_Self-contained ground-zombie (Shambler) AI component: wander/scream/chase/attack/grab state machine driving the owning Character's AIController, slot-montage animation, footsteps/lurk audio, damage/death and revival._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_DetectionRange` | `float` | `1800.f` | Shambler|Detect | EditAnywhere + BP | Max distance (UU) at which the Shambler can notice the player; LOS does the real gating. |
| `AZP_LoseSightTime` | `float` | `5.f` | Shambler|Detect | EditAnywhere + BP | Seconds without a clear sightline before the chase gives up and returns to wander. |
| `AZP_GiveUpRange` | `float` | `3000.f` | Shambler|Detect | EditAnywhere + BP | Hard leash distance (UU) beyond which the chase gives up immediately. |
| `AZP_FacingThreshold` | `float` | `0.25f` | Shambler|Detect | EditAnywhere + BP | dot(forward, dirToPlayer) it must exceed to notice the player; lower = wider awareness cone. |
| `AZP_MaxHealth` | `float` | `100.f` | Shambler|Damage | EditAnywhere + BP | Total health pool copied onto the UZP_HealthComponent at BeginPlay. |
| `AZP_BodyShotDamage` | `float` | `20.f` | Shambler|Damage | EditAnywhere + BP | Damage the Shambler takes per ranged body shot. |
| `AZP_HeadShotDamage` | `float` | `50.f` | Shambler|Damage | EditAnywhere + BP | Damage the Shambler takes per ranged headshot. |
| `AZP_HeadshotMinZ` | `float` | `55.f` | Shambler|Damage | EditAnywhere + BP | Hit height (UU above body centre) at/above which a ranged hit counts as a headshot. |
| `AZP_AttackRange` | `float` | `230.f` | Shambler|Attack | EditAnywhere + BP | Distance (UU) at which it commits to a swing; larger than the hit range on purpose so swings telegraph. |
| `AZP_AttackHitRange` | `float` | `220.f` | Shambler|Attack | EditAnywhere + BP | Distance (UU) the player must still be within at the moment the hit lands; back-stepping makes the swing whiff. |
| `AZP_AttackCooldown` | `float` | `0.4f` | Shambler|Attack | EditAnywhere + BP | Minimum seconds between swing commits from the Chase state. |
| `AZP_AttackDamage` | `float` | `12.5f` | Shambler|Attack | EditAnywhere + BP | Damage dealt to the player per landed swipe (halved from 25 on dev direction 2026-07-03). |
| `AZP_AttackHitTime` | `float` | `1.3f` | Shambler|Attack | EditAnywhere + BP | Clip time (s) of the contact frame in the swipe animation; remapped through the two play rates at swing start. |
| `AZP_AttackDuration` | `float` | `1.7f` | Shambler|Attack | EditAnywhere + BP | Fallback swing length (s) used only when a swing clip is missing. |
| `AZP_WindupEndTime` | `float` | `1.0f` | Shambler|Attack | EditAnywhere + BP | Clip time (s) where the wind-up ends and the strike launches (play-rate snap point). |
| `AZP_WindupPlayRate` | `float` | `0.8f` | Shambler|Attack | EditAnywhere + BP | Play rate of the swing's wind-up portion; below 1 = slower, more readable 'block now' telegraph. |
| `AZP_StrikePlayRate` | `float` | `1.6f` | Shambler|Attack | EditAnywhere + BP | Play rate of the strike + recovery portion; above 1 = the swing snaps once released. |
| `AZP_GrabRange` | `float` | `230.f` | Shambler|Grab | EditAnywhere + BP | Distance (UU) at which it latches the grab; the grab is the opener, melee is the fallback. |
| `AZP_GrabCooldown` | `float` | `30.f` | Shambler|Grab | EditAnywhere + BP | Seconds after a LANDED grab before this zombie may grab again (anti-chain-grab rule). |
| `AZP_GrabFailCooldown` | `float` | `6.f` | Shambler|Grab | EditAnywhere + BP | Seconds after a FAILED grab attempt (deflected/evaded) before the next try; clamped to GrabCooldown at use. |
| `AZP_GrabPairZOffset` | `float` | `0.f` | Shambler|Grab | EditAnywhere + BP | Vertical mesh nudge (UU) during the grab so the paired bodies line up in height. |
| `AZP_GrabPairDistance` | `float` | `70.f` | Shambler|Grab | EditAnywhere + BP | Chest-to-chest spacing (UU) the zombie snaps to at latch — the authored pair distance of the NAAT clips. |
| `AZP_GrabSnapInDuration` | `float` | `0.2f` | Shambler|Grab | EditAnywhere + BP | Seconds the body lunges (ease-out) into GrabPairDistance at the latch; 0 = old one-frame teleport. |
| `AZP_EscapePushbackDelay` | `float` | `0.9f` | Shambler|Grab | EditAnywhere + BP | Seconds the grapple spacing is held after a kick/push escape so the contact reads before the stumble-back starts. |
| `AZP_EscapePushbackDistance` | `float` | `45.f` | Shambler|Grab | EditAnywhere + BP | Displacement (UU) of the escape stumble-back step (dev spec: half a meter or less). |
| `AZP_EscapePushbackDuration` | `float` | `0.45f` | Shambler|Grab | EditAnywhere + BP | Seconds the eased escape step-back takes (stumble-paced, not shove-paced). |
| `AZP_EscapeStunDuration` | `float` | `1.5f` | Shambler|Grab | EditAnywhere + BP | Minimum stun (s) after being kicked/pushed off a grab — the escape-reward punish window; real pause = max(this, reaction clip length). |
| `AZP_DeflectStaggerDuration` | `float` | `1.2f` | Shambler|Grab | EditAnywhere + BP | Stagger (s) applied when a blocking player deflects the grab attempt. |
| `AZP_GrabArmLRotation` | `FRotator` | `FRotator::ZeroRotator` | Shambler|Grab | EditAnywhere + BP | Additive bone-local rotation on the left upper arm during the grapple to dial out arm clipping; zero = off. |
| `AZP_GrabArmRRotation` | `FRotator` | `FRotator::ZeroRotator` | Shambler|Grab | EditAnywhere + BP | Additive bone-local rotation on the right upper arm during the grapple to dial out arm clipping; zero = off. |
| `AZP_GrabHeadRotation` | `FRotator` | `FRotator::ZeroRotator` | Shambler|Grab | EditAnywhere + BP | Additive bone-local rotation on the head during the grapple — angles the mouth into the victim's neck; zero = off. |
| `AZP_GrabEscapeSnapDistance` | `float` | `0.f` | Shambler|Grab | EditAnywhere + BP | Pair spacing (UU) re-snapped to the instant a kick/push escape begins so the push contact reads; 0 = off. |
| `AZP_GrabLoopSound` | `TObjectPtr<USoundBase>` | `null (lazy default /Game/Audio/Shambler/SFX_SHAMBLER_GRAB in BeginPlay)` | Shambler|Grab | EditAnywhere + BP | Looping snarl for the whole grapple — starts at latch, hard-cut when the grab ends on any path. |
| `AZP_GrabAlertSound` | `TObjectPtr<USoundBase>` | `null (lazy default /Game/Audio/Shambler/SFX_GRAB_ALERT in BeginPlay)` | Shambler|Grab | EditAnywhere + BP | One-shot alert sting fired the instant the grab latches, layered under the snarl loop. |
| `AZP_ScreamHoldTime` | `float` | `2.0f` | Shambler|Attack | EditAnywhere + BP | Seconds it holds the stationary scream on SIGHT aggro before breaking into the chase. |
| `AZP_HurtScreamHoldTime` | `float` | `0.5f` | Shambler|Attack | EditAnywhere + BP | Short scream hold (s) when aggro came from taking damage — stops melee spam farming a stationary wail. |
| `AZP_WanderSpeed` | `float` | `120.f` | Shambler|Move | EditAnywhere + BP | CMC MaxWalkSpeed (UU/s) while wandering. |
| `AZP_ChaseSpeed` | `float` | `250.f` | Shambler|Move | EditAnywhere + BP | CMC MaxWalkSpeed (UU/s) during the chase fast-walk (also the walk phase between run bursts). |
| `AZP_RunSpeed` | `float` | `420.f` | Shambler|Move | EditAnywhere + BP | CMC MaxWalkSpeed (UU/s) during sprint-chase run bursts. |
| `AZP_RunTriggerDistance` | `float` | `450.f` | Shambler|Move | EditAnywhere + BP | Chase distance (UU) beyond which it breaks into the sprint; the gap to AttackRange is the hysteresis band. |
| `AZP_RunBurstDuration` | `float` | `2.0f` | Shambler|Move | EditAnywhere + BP | Seconds of each RUN burst inside the sprint-chase (min 0.25 enforced at use). |
| `AZP_RunWalkDuration` | `float` | `4.0f` | Shambler|Move | EditAnywhere + BP | Seconds of fast walk between run bursts; 0 = continuous running. |
| `AZP_RunAnimRefSpeed` | `float` | `350.f` | Shambler|Anim | EditAnywhere + BP | Speed (UU/s) at which the run clip's stride looks natural; the loop plays at RunSpeed / this (foot-skate knob). |
| `AZP_RunHeadStabilize` | `float` | `1.f` | Shambler|Anim | EditAnywhere + BP | World-space head stabilization amount 0..1 during run bursts; read directly (C++) by UZP_ShamblerGrabPoseAnimInstance — rename must update that file too. |
| `AZP_RunAnim` | `TObjectPtr<UAnimSequence>` | `null (lazy default /Game/Enemies/Shambler/Anims/A_Shambler_RunStiffArmsHead)` | Shambler|Anim | EditAnywhere + BP | Run cycle clip played as a slot loop over the walk BlendSpace during the sprint. |
| `AZP_CombatTurnRate` | `float` | `300.f` | Shambler|Move | EditAnywhere + BP | Degrees/sec it pivots to face the player during combat (attack tracking / chase hold). |
| `AZP_WanderRadius` | `float` | `180.f` | Shambler|Wander | EditAnywhere + BP | Leash radius (UU) around the spawn point within which wander legs are picked. |
| `AZP_WanderMinLegDistance` | `float` | `100.f` | Shambler|Wander | EditAnywhere + BP | Min straight-line distance (UU) a candidate wander point must be from the current position. |
| `AZP_WanderTargetDot` | `float` | `0.5f` | Shambler|Wander | EditAnywhere + BP | Target dot(forward, dirToCandidate) for picked wander legs; ~0.5 gives curving organic paths. |
| `AZP_WalkDurationMin` | `float` | `3.0f` | Shambler|Wander | EditAnywhere + BP | Min seconds of the wander walk phase before dropping to idle. |
| `AZP_WalkDurationMax` | `float` | `6.0f` | Shambler|Wander | EditAnywhere + BP | Max seconds of the wander walk phase before dropping to idle. |
| `AZP_IdleDurationMin` | `float` | `6.0f` | Shambler|Wander | EditAnywhere + BP | Min seconds of the wander idle phase before resuming walking. |
| `AZP_IdleDurationMax` | `float` | `9.0f` | Shambler|Wander | EditAnywhere + BP | Max seconds of the wander idle phase before resuming walking. |
| `AZP_IdleMeshZOffset` | `float` | `-8.f` | Shambler|Anim | EditAnywhere + BP | Z offset (UU) applied to the mesh while the idle pose shows, compensating the idle anim's higher pelvis. |
| `AZP_IdleBlendInTime` | `float` | `0.3f` | Shambler|Anim | EditAnywhere + BP | Blend-in time (s) for slot loops (idle/run) — covers the CMC's braking ramp on WALK->IDLE. |
| `AZP_IdleBlendOutTime` | `float` | `0.3f` | Shambler|Anim | EditAnywhere + BP | Blend-out time (s) for slot loops when leaving the idle phase (also used by StopSlotLoop). |
| `AZP_IdleLockDelay` | `float` | `0.35f` | Shambler|Anim | EditAnywhere + BP | Seconds after entering wander-idle before the CMC is hard-locked to MOVE_None (lets braking play out). |
| `AZP_StumbleChancePerSec` | `float` | `0.15f` | Shambler|Wander | EditAnywhere + BP | Per-second probability of a mid-leg stumble (zombie hesitation) during wander. |
| `AZP_StumbleMin` | `float` | `0.25f` | Shambler|Wander | EditAnywhere + BP | Min stumble pause length (s). |
| `AZP_StumbleMax` | `float` | `0.8f` | Shambler|Wander | EditAnywhere + BP | Max stumble pause length (s). |
| `AZP_IdleSound` | `TObjectPtr<USoundBase>` | `null (lazy default /Game/Audio/Shambler/SFX_ZOMBIE_IDLE in BeginPlay)` | Shambler|Audio | EditAnywhere + BP | Groan one-shot played exactly when the wander idle animation starts. |
| `AZP_FootstepSounds` | `TArray<TObjectPtr<USoundBase>>` | `empty (lazy-filled from /Game/Audio/Shambler/Footsteps/SFX_SHAMBLER_FOOTSTEP_01..16 in BeginPlay)` | Shambler|Audio | EditAnywhere + BP | Footstep one-shots, random pick per distance-based step. |
| `AZP_FootstepVolume` | `float` | `1.f` | Shambler|Audio | EditAnywhere + BP | Footstep volume multiplier; 0 = silent feet (the dev knob). |
| `AZP_FootstepStride` | `float` | `80.f` | Shambler|Audio | EditAnywhere + BP | Distance (UU) traveled per footstep SFX — distance-based stepping cadence-matches every gait. |
| `AZP_FootstepPitchVar` | `float` | `0.08f` | Shambler|Audio | EditAnywhere + BP | Random pitch spread per step (1 +/- this) so steps don't sound mechanical. |
| `AZP_LurkRange` | `float` | `1800.f` | Shambler|Audio | EditAnywhere + BP | Player distance (UU) inside which the periodic lurk growl plays during wander. |
| `AZP_LurkIntervalMin` | `float` | `6.f` | Shambler|Audio | EditAnywhere + BP | Min seconds between lurk growls. |
| `AZP_LurkIntervalMax` | `float` | `13.f` | Shambler|Audio | EditAnywhere + BP | Max seconds between lurk growls. |
| `AZP_WalkAnim` | `TObjectPtr<UAnimSequence>` | `null (lazy default /Game/Enemies/Shambler/Anims/A_Shambler_Walk)` | Shambler|Anim | EditAnywhere + BP | Walk cycle clip slot (locomotion via ABP_Shambler's BlendSpace; slot kept for overrides). |
| `AZP_IdleAnim` | `TObjectPtr<UAnimSequence>` | `null (lazy default /Game/Enemies/Shambler/Anims/A_Shambler_Idle)` | Shambler|Anim | EditAnywhere + BP | Idle pose clip played as a slot loop during wander pauses and the loom. |
| `AZP_AttackLAnim` | `TObjectPtr<UAnimSequence>` | `null (lazy default /Game/Enemies/Shambler/Anims/A_Shambler_Attack_L)` | Shambler|Anim | EditAnywhere + BP | Left swipe attack clip. |
| `AZP_AttackRAnim` | `TObjectPtr<UAnimSequence>` | `null (lazy default /Game/Enemies/Shambler/Anims/A_Shambler_Attack_R)` | Shambler|Anim | EditAnywhere + BP | Right swipe attack clip. |
| `AZP_ScreamAnim` | `TObjectPtr<UAnimSequence>` | `null (lazy default /Game/Enemies/Shambler/Anims/A_Shambler_ScreamPinned)` | Shambler|Anim | EditAnywhere + BP | Alert scream clip (lower body pinned to the idle stance). |
| `AZP_DeathFrontAnim` | `TObjectPtr<UAnimSequence>` | `null (lazy default /Game/Enemies/Shambler/Anims/A_Shambler_Death_Front)` | Shambler|Anim | EditAnywhere + BP | Front-fall death clip (shot from the front), held on the final frame as the corpse. |
| `AZP_DeathBackAnim` | `TObjectPtr<UAnimSequence>` | `null (lazy default /Game/Enemies/Shambler/Anims/A_Shambler_Death_Back)` | Shambler|Anim | EditAnywhere + BP | Back-fall death clip (shot from behind), held on the final frame as the corpse. |
| `AZP_HitFrontAnim` | `TObjectPtr<UAnimSequence>` | `null (lazy default /Game/Enemies/Shambler/Anims/A_Shambler_Hit_Front)` | Shambler|Anim | EditAnywhere + BP | Front hit-reaction flinch clip (non-lethal damage from the front; also the stagger clip). |
| `AZP_HitBackAnim` | `TObjectPtr<UAnimSequence>` | `null (lazy default /Game/Enemies/Shambler/Anims/A_Shambler_Hit_Back)` | Shambler|Anim | EditAnywhere + BP | Back hit-reaction flinch clip (non-lethal damage from behind). |
| `AZP_GrabEntryAnim` | `TObjectPtr<UAnimSequence>` | `null (lazy default /Game/Enemies/Shambler/Anims/A_Shambler_GrabEntry)` | Shambler|Grab | EditAnywhere + BP | Attacker-side NAAT grab entry clip, paired 1:1 with the player's victim clip. |
| `AZP_GrabMunchAnim` | `TObjectPtr<UAnimSequence>` | `null (lazy default /Game/Enemies/Shambler/Anims/A_Shambler_GrabMunch)` | Shambler|Grab | EditAnywhere + BP | Grab munch-phase loop clip (paired). |
| `AZP_GrabWrestleAnim` | `TObjectPtr<UAnimSequence>` | `null (lazy default /Game/Enemies/Shambler/Anims/A_Shambler_GrabWrestle)` | Shambler|Grab | EditAnywhere + BP | Grab wrestle/struggle-phase loop clip (paired). |
| `AZP_GrabKickedAnim` | `TObjectPtr<UAnimSequence>` | `null (lazy default /Game/Enemies/Shambler/Anims/A_Shambler_GrabKicked)` | Shambler|Grab | EditAnywhere + BP | Reaction clip when the victim escapes via kick. |
| `AZP_GrabPushedAnim` | `TObjectPtr<UAnimSequence>` | `null (lazy default /Game/Enemies/Shambler/Anims/A_Shambler_GrabPushed)` | Shambler|Grab | EditAnywhere + BP | Reaction clip when the victim escapes via push. |
| `AZP_GrabTakedownAnim` | `TObjectPtr<UAnimSequence>` | `null (lazy default /Game/Enemies/Shambler/Anims/A_Shambler_GrabTakedown)` | Shambler|Grab | EditAnywhere + BP | Takedown clip slot — loaded but currently unplayed (FailKnockdown uses the Scream combo instead; the pack dive floats, see DEAD ENDS 2026-07-02). |
| `AZP_AnimWalkRefSpeed` | `float` | `150.f` | Shambler|Anim | EditAnywhere + BP | Speed (UU/s) at which the walk clip's stride looks natural (foot-skate knob for the walk). |
| `AZP_ScreamMeshZOffset` | `float` | `-8.f` | Shambler|Anim | EditAnywhere + BP | Mesh-Z nudge (UU) during the scream so the pose grounds (matches IdleMeshZOffset with the pinned clip). |
| `AZP_DeathDropZ` | `float` | `90.f` | Shambler|Anim | EditAnywhere + BP | Mesh drop (UU) over the death clip so the root-motion-less corpse settles on the ground. |
| `AZP_DeathDropLead` | `float` | `0.5f` | Shambler|Anim | EditAnywhere + BP | Finish the death drop this many seconds before the clip ends so it lands with the visual fall. |
| `AZP_EvalInterval` | `float` | `0.25f` | Shambler | EditAnywhere + BP | AI decision-loop period (s); also the integration step for lose-sight/stumble-chance math. |
| `AZP_FlinchCooldown` | `float` | `0.5f` | Shambler|Anim | EditAnywhere + BP | Min seconds between flinch-clip plays / swing hitches (mesh punch is never gated). ALSO the anti-stunlock rate limit for `bAZP_HitReactGatesMovement`. |
| `bAZP_HitReactGatesMovement` | `bool` | `true` | Shambler|Anim | EditAnywhere + BP | Hit reactions freeze the AI for the flinch clip (~0.65s of the 0.8s clip) then resume, instead of sliding — the flinch is a full-body slot montage that masks the walk cycle while the capsule kept pathing (dev 2026-07-14). `false` = pre-2026-07-14 cosmetic-only flinch. |
| `AZP_SwingHitchRate` | `float` | `0.05f` | Shambler|Anim | EditAnywhere + BP | Play-rate the in-flight swing drops to when a hit lands mid-swing (near-freeze hit-stop). |
| `AZP_SwingHitchTime` | `float` | `0.16f` | Shambler|Anim | EditAnywhere + BP | Seconds the absorb-hitch lasts before the swing snaps back to its phase rate. |
| `AZP_HitPunchStrength` | `float` | `9.f` | Shambler|Anim | EditAnywhere + BP | Mesh jolt (UU) along the hit direction on every landed hit, spring-decaying; 0 disables. |
| `AZP_HitPunchRecovery` | `float` | `12.f` | Shambler|Anim | EditAnywhere + BP | Hit-punch spring-back speed (per second); higher = snappier recovery. |
| `AZP_CloseSenseRange` | `float` | `350.f` | Shambler|Detect | EditAnywhere + BP | Point-blank distance (UU) within which the Shambler senses the player regardless of facing (also bypasses the through-wall audio veto) — invented name for the hardcoded 350.f in the wander sight check. |
| `AZP_ChaseAcceptanceInset` | `float` | `70.f` | Shambler|Move | EditAnywhere + BP | How far inside AttackRange the chase MoveToActor stops (acceptance = max(AttackRange - 70, 40)) so it reliably crosses the swing threshold instead of parking at the edge — invented name for the hardcoded 70.f (and 40.f floor). |
| `AZP_WanderAcceptRadius` | `float` | `100.f` | Shambler|Wander | EditAnywhere + BP | MoveToLocation acceptance radius (UU) for wander legs ('close enough' so it doesn't orbit the exact point) — same 100.f repeated at the stumble-resume re-issue (ZP_ShamblerBehaviorComponent.cpp:444). |

### UZP_ShamblerGrabPoseAnimInstance
_C++ parent of ABP_Shambler; native post-evaluate overlay that applies the grab arm/head bone rotations and run-burst world-space head stabilization on the Shambler mesh._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_GrabBoneMatchSubstrings` | `FString (x5: leftarm/rightarm/forearm-excluder/head/top-excluder)` | `"head", "top" (exclude), "forearm" (exclude), "leftarm", "rightarm"` | Shambler|Grab | internal (C++ only) | Substring matchers used to resolve the necromorph mesh's upper-arm and head bones (mixamorig_* names vary per export); sites ZP_ShamblerGrabPoseAnimInstance.cpp:73-76. Would need retuning only if the Shambler mesh/skeleton family changes. |

### UZP_SignalSenseComponent
_"The Phone" — Silent-Hill-radio proximity warning component on the player: tag-based void/enemy detection driving layered audio loops, HUD waveform amplitude/history texture, controller rumble, and a stage-changed event._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_VoidmoteTag` | `FName` | `TEXT("Voidmote")` | SignalSense|Detection | EditAnywhere + BP | Actor tag that marks voidzone (voidmote) actors whose bounds trigger the Interference stage. |
| `AZP_EnemyTag` | `FName` | `TEXT("Enemy")` | SignalSense|Detection | EditAnywhere + BP | Actor tag that marks enemies for the proximity Ring/Alarm stages. |
| `AZP_VoidZoneRadius` | `float` | `500.f` | SignalSense|Detection | EditAnywhere + BP | Fallback box half-size for a tagged void actor that reports no usable bounds. |
| `AZP_EnemyRadius` | `float` | `1400.f` | SignalSense|Detection | EditAnywhere + BP | Enemy 'in range' distance that triggers the Ring (Enemy) stage. |
| `AZP_MeleeRadius` | `float` | `250.f` | SignalSense|Detection | EditAnywhere + BP | Enemy 'on you' distance that escalates to the Alarm (Melee) stage. |
| `AZP_EvaluateInterval` | `float` | `0.25f` | SignalSense|Detection | EditAnywhere + BP | Seconds between tag-scan evaluations of void/enemy proximity (clamped to min 0.05s at BeginPlay). |
| `AZP_InterferenceLoop` | `TObjectPtr<USoundBase>` | `/Game/Audio/Signal/MS_Signal_InterferenceBed.MS_Signal_InterferenceBed (ConstructorHelpers, ZP_SignalSenseComponent.cpp:26)` | SignalSense|Audio | EditAnywhere + BP | Always-looping in-void interference drone stem whose volume ramps with proximity to the void centre. |
| `AZP_RingLoop` | `TObjectPtr<USoundBase>` | `/Game/Audio/Signal/SFX_Phone_Ring.SFX_Phone_Ring (ConstructorHelpers, ZP_SignalSenseComponent.cpp:27)` | SignalSense|Audio | EditAnywhere + BP | Loop stem played (crossfaded in) when an enemy is within EnemyRadius (Ring stage). |
| `AZP_AlarmLoop` | `TObjectPtr<USoundBase>` | `/Game/Audio/Signal/SFX_Signal_Alarm.SFX_Signal_Alarm (ConstructorHelpers, ZP_SignalSenseComponent.cpp:28)` | SignalSense|Audio | EditAnywhere + BP | Loop stem played when an enemy is within MeleeRadius (Alarm/Melee stage). |
| `AZP_ClearSting` | `TObjectPtr<USoundBase>` | `/Game/Audio/Signal/SFX_Phone_Silence.SFX_Phone_Silence (ConstructorHelpers, ZP_SignalSenseComponent.cpp:29)` | SignalSense|Audio | EditAnywhere + BP | One-shot 'all clear' sting; currently INERT — the sting scheduling was disabled per dev request 2026-06-14 (SetStage only clears the timer, ZP_SignalSenseComponent.cpp:311-316). |
| `AZP_CrossfadeTime` | `float` | `0.6f` | SignalSense|Audio | EditAnywhere + BP | Seconds for the ping-pong crossfade between enemy stems (Ring<->Alarm and fade-out to none). |
| `AZP_ClearDelay` | `float` | `4.0f` | SignalSense|Audio | EditAnywhere + BP | Seconds after everything goes quiet before the ClearSting would fire; currently INERT because the sting scheduling is disabled in SetStage. |
| `AZP_InterferenceKneeProximity` | `float` | `0.6f` | SignalSense|Audio | EditAnywhere + BP | Crescendo knee position: proximity-to-centre (0 = box surface, 1 = centre) where the gentle outer volume rise ends and the linear ramp to full begins. |
| `AZP_InterferenceKneeVolume` | `float` | `0.3f` | SignalSense|Audio | EditAnywhere + BP | Interference volume at the knee point (dev spec: 30% by 60% proximity, then linear to full at centre). |
| `AZP_VoidFadeDuration` | `float` | `3.0f` | SignalSense|Audio | EditAnywhere + BP | Seconds for the smoothstepped fade-in/out of the interference bed at the enter/leave-all-voids boundary. |
| `AZP_CrescendoSmoothSpeed` | `float` | `5.0f` | SignalSense|Audio | EditAnywhere + BP | Interp speed for how fast the in-void volume crescendo tracks player position (higher = snappier). |
| `AZP_VoidExitGrace` | `float` | `3.0f` | SignalSense|Audio | EditAnywhere + BP | Grace seconds after leaving all voids before the fade-out starts; intended to be matched to VoidFadeDuration so gaps between chained voids stay seamless. |
| `AZP_RingRumble` | `TObjectPtr<UForceFeedbackEffect>` | `null` | SignalSense|Haptics | EditAnywhere + BP | Looping gamepad force-feedback effect for the Enemy (Ring) stage; null-safe until the asset exists. |
| `AZP_AlarmRumble` | `TObjectPtr<UForceFeedbackEffect>` | `null` | SignalSense|Haptics | EditAnywhere + BP | Looping gamepad force-feedback effect for the Melee (Alarm) stage; null-safe until the asset exists. |
| `AZP_AmpSampleRate` | `float` | `60.f` | SignalSense|Waveform | EditAnywhere + BP | Waveform scope sample rate in samples/sec — higher scrolls the HUD amplitude-history texture faster and shows less time on screen. |
| `AZP_AmpHistorySize` | `static constexpr int32` | `256` | SignalSense|Waveform | internal (C++ only) | Ring-buffer length / width of the 256x1 G8 amplitude-history scope texture; structural (texture allocated with it at BeginPlay), so keep as a compile-time constant rather than an editor knob. |
| `AZP_AmplitudeSmoothSpeed` | `float` | `6.f` | SignalSense|Waveform | EditAnywhere + BP | FInterpTo speed with which the HUD waveform amplitude eases toward its stage target — the feel of how quickly the scope reacts. |
| `AZP_MeleeStageAmplitude` | `float` | `1.0f` | SignalSense|Waveform | EditAnywhere + BP | HUD waveform amplitude target while in the Melee (Alarm) stage. |
| `AZP_EnemyStageAmplitude` | `float` | `0.7f` | SignalSense|Waveform | EditAnywhere + BP | HUD waveform amplitude target while in the Enemy (Ring) stage. |

### UZP_TransitMenuWidget
_Floor-selection menu for the Transit system; WBP mode populates a designer-bound DestinationList with WBP_TransitRow widgets, code mode builds a plain C++ button list so a placed panel works with zero assets._

| Knob | Type | Default | Category | Exposure | Tunes |
|---|---|---|---|---|---|
| `AZP_RowWidgetClass` | `TSubclassOf<UZP_TransitRowWidget>` | `null (assigned to WBP_TransitRow via Scripts/Python/_set_rowclass.py)` | Transit | EditAnywhere + BP | Row widget class spawned per destination in WBP mode (WBP_TransitRow); if unset, plain code buttons are built instead. |
| `AZP_TransitMenuHeaderText` | `FText` | `"SELECT DESTINATION"` | Transit|Text | EditAnywhere + BP | Player-facing header label of the code-mode destination list; hardcoded FText::FromString violates the expose-player-facing-text rule (unlike the LOCKED strings, which already use NSLOCTEXT). |
| `AZP_TransitCloseButtonText` | `FText` | `"Close"` | Transit|Text | EditAnywhere + BP | Player-facing label of the code-mode Close button; hardcoded FText::FromString, should be an editable FText. |

