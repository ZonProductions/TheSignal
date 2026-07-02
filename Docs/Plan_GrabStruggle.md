# Plan — Zombie Grab & Struggle System (Shambler)

**Status:** PLAN — not yet implemented. Written 2026-07-02 from live asset verification (editor was up,
all anim data below read via the MCP Python endpoint), a full Source/ audit, and web research on how
shipped horror games tune this system.

**The feature in one line:** a Shambler in melee range latches onto Marcus, starts biting; the camera
pulls out to a third-person over-the-right-shoulder view with the damage vignette up; the player mashes
attack (LMB) to break free — success plays a random kick/push shove, failure knocks Marcus down
(first-person knockdown + get-up, camera riding the fall). The grab can be blocked.

---

## 1. What other devs do (research digest → rules we adopt)

Full game-by-game breakdown lives in the research bundle; these are the conclusions that shape this plan:

| Rule | Source / evidence |
|---|---|
| **Telegraph the grab** — distinct wind-up before the latch | Techland patched Dying Light: The Beast specifically to make grab wind-ups "more obvious"; RE4R's untelegraphed magnet-grabs are its most-hated mechanic |
| **Free-escape window: NO damage in the first moments of a grab** | The Beast hotfix ("damage will now occur slightly later"), Dying Light 1 *Instant Escape* (<1s = 0 damage), Callisto parasites. Damage-before-prompt is the documented anti-pattern (RE4R) |
| **Anti-chain-grab cooldowns** — per-zombie grab cooldown + player immunity window after escaping | Condemned's instant re-grab is the canonical complaint; The Beast added "first alerted zombie won't open with a grab"; L4D paralyzes the Smoker after a break |
| **Not every enemy grabs, not constantly** | Dead Island 2 (only some zombies grab) is explicitly praised vs Dying Light 2, which needed a patch + a popular mod purely to reduce grab frequency. #1 failure mode of FP zombie grabs = frequency |
| **If mash mitigates, SHOW it; make the meter framerate-independent** | RE3R's invisible mash mitigation read as "does nothing"; Alien Isolation shipped a mash-circle bug where presses didn't register at some framerates |
| **Escape reward ladder** — success should stun the zombie briefly (escape → punish window) | Dying Light 1's upgraded escape (push + stun) is the pattern players love; plain release feels flat |
| **Fail ladder** — fixed damage chunk + knockdown, NOT death | RE2R model (bite chunk, release). Knockdown is our chosen fail state; instant-kill grabs (Callisto Two-Head) are the top community complaint there |
| **Accessibility from day one** — the mash is an abstract 0→1 "escape progress" meter with pluggable drivers (tap / hold) | TLOU2, RE4R, The Quarry all ship "Repeated Button Presses: Tap ↔ Hold"; Game Accessibility Guidelines has a dedicated rule for it. Costs nothing if built in from the start |
| **FP→3P pull-out is legitimate but rare** — done for legibility/spectacle (Deus Ex: HR takedowns, Halo assassinations, Destiny finishers); most FP horror stays FP (Dying Light, Condemned, Isolation, Cyberpunk) | The switch needs a presentable full body + camera that survives walls. We HAVE a full body (MarcusBody) and the wrestle is a fixed short beat — doable, this is the "daring part" and section 5 is how we keep it from fucking everything up |

---

## 2. Asset reality check (VERIFIED in-editor — every value below read live)

All 7 requested clips exist. The NAAT set lives in the **BSP_ZombieAnims** pack
(`/Game/BSP_ZombieAnims/Animations/Interactive/`), which ships **paired attacker/victim clips with
identical lengths** — `Human_*` = the victim (Marcus), `Zombie_*` = the attacker (Shambler):

| Pair | Human (player) | Zombie (Shambler) | Length | Role |
|---|---|---|---|---|
| Grab entry | `AS_NAAT_Human_Idle_To_Grab` | `AS_NAAT_Zombie_Idle_To_Grab` | 0.6 s | Latch |
| Munch | `AS_NAAT_Human_Grab_To_Munching` | `AS_NAAT_Zombie_Grab_To_Munching` | 2.3333 s | Biting (damage) |
| Wrestle | `AS_NAAT_Human_Grab_To_Wrestle` | `AS_NAAT_Zombie_Grab_To_Wrestle` | 1.2333 s | Struggle loop |
| Kick escape | `AS_NAAT_Human_Grab_To_Kick` | `AS_NAAT_Zombie_Grab_To_Kicked` | 2.3 s | Success outcome A |
| Push escape | `AS_NAAT_Human_Grab_To_Push` | `AS_NAAT_Zombie_Grab_To_Pushed` | 2.3 s | Success outcome B |
| (Bonus) Takedown | `AS_NAAT_Human_Grab_To_TakeDown` | `AS_NAAT_Zombie_Grab_To_TakeDown` | 2.2 s | Pack's authored knockdown |
| (Bonus) Ground munch | `AS_NAAT_Human_TakeDown_To_Munching` | `AS_NAAT_Zombie_TakeDown_To_Munching` | ~2.25 s | Pack's authored ground-bite |

Also verified: `FPP_Dag_Knockdown` (FPPMeleeAnimset, 3.87 s, first-person clip) and
`Get_Up_Back`/`Get_Up_Front` (CharacterCustomizer SLS ragdoll set, 1.5 s each).

### 2a. Two corrections to the request (asset data, not opinion)

1. **The struggle pairing as written desyncs.** The request pairs player `Human_Grab_To_Munching`
   (2.3333 s) with Shambler `Grab_To_Wrestle` (1.2333 s). The pack pairs Munching↔Munching and
   Wrestle↔Wrestle at identical lengths; crossing the sets means the two bodies stop matching almost
   immediately. **This plan uses matched pairs:** Munch pair = the "being bitten" phase (damage ticks),
   Wrestle pair = the active struggle loop once you're mashing. Same beats you described, in the pack's
   authored pairs.
2. **The Shambler's clips are the `Zombie_*` ones.** The request names `Human_*` clips for the Shambler —
   those are the victim side. Mapped accordingly above.

### 2b. Skeleton situation — every clip needs an offline retarget (none are playable as-is)

Verified: **no clip in this feature targets a skeleton any of our meshes use**, and no
compatible-skeletons are declared anywhere. Also **zero root motion on every clip** (RootLock=REF_POSE) —
alignment is 100% code (section 6).

| Clips | Source skeleton | Target | Retargeter | Status |
|---|---|---|---|---|
| 5× `Zombie_*` grab clips | NAAT's own UE4 mannequin (`SKEL_UE4_Manaquin_Skeleton`) | Shambler necromorph skeleton | `RTG_UE4_to_Shambler` (`/Game/Enemies/Shambler/Rigs/`) — **already exists**, same pipeline that made all A_Shambler_* clips | Verify the RTG accepts the NAAT pack's own UE4 skeleton asset (same hierarchy, different Skeleton uasset than the AIBehaviorSystem one it was built against) |
| 5× `Human_*` grab clips | Same NAAT UE4 mannequin | **CCMH** (MarcusBody) | **None exists.** Either chain UE4→UE5Manny (`RTG_UE4Manny_UE5Manny` exists in `/Game/AIBehaviorSystem/.../Rigs/`) then Manny→CCMH (the proven Marcus pipeline), or build `RTG_UE4_to_CCMH` directly | NEW retarget work |
| `FPP_Dag_Knockdown` | FPPMeleeAnimset UE4 mannequin (declares compat with AIBehaviorSystem SK_Mannequin) | **UE5 Manny** (hidden `Mesh`) — see section 7 for why | `RTG_UE4Manny_UE5Manny` — exists | Straightforward |
| `Get_Up_Back` | `CC_Skeleton` (CC_Dummy — a THIRD CharacterCustomizer skeleton, NOT CCMH) | UE5 Manny (hidden `Mesh`) | **None exists** — needs a new CC→Manny RTG | NEW retarget work; clip carries a `Disable Movement` float curve — strip or verify inert |

**Hard rule carried over from DEAD ENDS 2026-06-29:** every retarget landing on the necromorph skeleton
must be **curve-audited before first level load** (`AnimationLibrary.remove_curve` for any orphan float
curve) — the A_Shambler_Hit_* orphan-GASP-curve PostLoad crash. Good news: all NAAT source clips were
verified to have **0 float curves**, so the hazard would only come from the retarget pipeline itself.
And never hard-reference the new clips on BP_Shambler — lazy-load via `LoadAnimDefaults()` like the
existing nine. Python retargets need `run_op_initial_setup()` per op (UE5.7+ lesson) or output is static.

---

## 3. Player-facing flow (the spec, consolidated)

```
Shambler in Chase, within GrabRange, off cooldown, grab rules pass
        │
        ▼
  [BLOCK CHECK] player holding melee block → grab DEFLECTED:
        Shambler staggers (existing block-stagger machinery), grab goes on cooldown. Done.
        │ not blocking
        ▼
  ENTRY (0.6s) ── paired Idle_To_Grab. Zero damage (free-escape philosophy starts the
        clock AFTER this). Camera begins 1P→3P blend (fits inside the 0.6s).
        Player move/look/weapons locked; mash prompt fades in.
        ▼
  MUNCH LOOP (2.33s/loop) ── paired Grab_To_Munching. Bite damage ticks once per loop at
        the authored bite frame. Damage vignette held up + pulses per bite.
        First LMB press → transitions to…
        ▼
  WRESTLE LOOP (1.23s/loop) ── paired Grab_To_Wrestle. Mash meter: each LMB press adds,
        meter decays per second. Struggle timer runs.
        │                                    │
        meter ≥ threshold                    timer expires below threshold
        ▼                                    ▼
  ESCAPE (2.3s) — 50/50 random:        FAIL — fail damage chunk +
        Human_Grab_To_Kick+Zombie_Kicked     KNOCKDOWN: camera blends back to 1P,
        or Human_Grab_To_Push+Zombie_Pushed  FPP_Dag_Knockdown plays on the FP rig,
        Shambler shoved back + briefly       camera rides the fall (3.87s), then
        stunned; player grab-immune          Get_Up_Back (1.5s). Shambler backs off
        for a window. Camera blends          (post-grab cooldown running).
        back to 1P over the last ~0.3s.
        ▼
  Back to normal play. Shambler per-zombie grab cooldown running.
```

Death can occur from bite ticks or the fail chunk at any point → normal `HandleDeath` path, with grab
cleanup guaranteed first (section 9).

---

## 4. Architecture (C++ — all of it)

Follows the ladder precedent (state lives in `AZP_GraceCharacter`) + a new Shambler state. No Blueprint
logic; BP touches are limited to nothing at all if lazy-load paths are used (preferred, matches
`LoadAnimDefaults`).

### New files
- **`ZP_Grabbable.h`** — `IZP_Grabbable` C++ interface on the player (pattern: `IZP_Staggerable`):
  `CanBeGrabbed()`, `BeginGrabbedBy(AActor* Grabber, const FZP_GrabParams&)`, `AbortGrab(reason)`.
  Future enemy types (Crawler pounce-pin? Scytheer?) reuse it instead of coupling to Grace.
- **`ZP_GrabTypes.h`** — `EZP_GrabPhase { None, Entry, Munch, Wrestle, EscapeKick, EscapePush, FailKnockdown, GetUp }` + params struct.

### Player side — `AZP_GraceCharacter` additions
- `EZP_GrabPhase GrabPhase` + `AActor* GrabberActor` + timers. All input handlers gain a `GrabPhase != None`
  gate at the top (exact pattern of `bOnLadder`/`bInventoryMenuOpen` gates that already head every handler).
- **Mash interception point:** top of `Input_FireStarted()`, BEFORE the `!KinemationComp->ActiveWeapon`
  early-out (critical — if the grab stows the weapon, presses would otherwise die at that check):
  `if (GrabPhase != None) { GrabMashPressed(); return; }`. No IMC swap needed — bool gates are the
  codebase pattern and carry zero stuck-context risk.
- Escape meter (see section 8), phase advancement on timers, SingleNode clip playback on MarcusBody
  (existing `GetSingleNodeInstance()->SetAnimationAsset` pattern — the player has NEVER used montages;
  keep it that way).
- Block check + dodge interaction (section 8a), damage routing (section 9), camera (section 5),
  mesh visibility (section 7).

### Shambler side — `UZP_ShamblerBehaviorComponent` additions
- New `EShamblerState::Grab`. Entered from Chase in `Evaluate()` when:
  `DistToPlayer <= GrabRange && bSee && GrabCooldownElapsed && GrabRulesPass` (rules = section 8b) —
  checked BEFORE the existing attack trigger so grab outranks a swing at close range.
- On enter: `SetSpeed(0)` + `AICon->StopMovement()` + `CM->SetMovementMode(MOVE_None)` (the exact
  Attack-state + `LockIdleMovement` freeze that already exists), **snap-face once** toward the player
  (the Scream face-lock pattern — no per-frame tracking during a synced anim), then call
  `IZP_Grabbable::BeginGrabbedBy` on the player and play its own clip sequence via the existing
  `PlayOneShot()` / `PlaySlotLoop()` dynamic-montage infra on `DefaultSlot`.
- **Gate the state-forcers:** `OnPointDamage` (`.cpp:910-922`) and `ReceiveStaggerHit` (`.cpp:1010-1015`)
  both currently FORCE Scream on damage/stagger — both must special-case `State == Grab`, otherwise
  shooting the grabber mid-wrestle yanks it out of the paired anim while Marcus stays locked.
  Design call (see DEV DECISIONS): recommended behavior = enough damage/stagger mid-grab **breaks the
  grab** (releases the player, no escape reward anim, zombie flinches) — gives a second weapon-armed
  option and matches RE2R defensive-item logic. Cleanup reuses the `CancelPendingSwing()` pattern
  (kill queued timers + `Montage_Stop`).
- New anim UPROPERTY slots (`GrabEntryAnim`, `GrabMunchAnim`, `GrabWrestleAnim`, `GrabKickedAnim`,
  `GrabPushedAnim`) lazy-defaulted in `LoadAnimDefaults()` to the new `/Game/Enemies/Shambler/Anims/A_Shambler_Grab_*` retargets.
- Escape shove: `LaunchCharacter` backward on the Shambler + temporary stagger (reuse `bStaggered`
  machinery) — no root motion exists in the clips, so the knockback is programmatic.
- `UZP_DeathSaveComponent` / EGUI persistence: grab state is transient runtime-only — nothing about it
  ever serializes. Zombie dies mid-grab (`OnOwnerDied` / `bDead`) → `AbortGrab` the player first.

### Player↔Shambler alignment (no root motion anywhere, so):
1. On grab commit, compute the pair transform: Shambler snap-lerped (~0.1 s) to
   `PlayerLoc + PlayerFacing * GrabPairDistance`, both actors yaw-snapped to face each other
   (player via `Controller->SetControlRotation` + `bUseControllerRotationYaw=false` — the ladder recipe).
2. `MoveIgnoreActors` both capsules mutually (NOT `SetActorEnableCollision(false)` — keeps floor
   collision alive) + `MOVE_None` both. Restore on any exit path.
3. `GrabPairDistance` is a tunable measured once from the pack's `Maps/Animation_Showcase.umap` /
   `BP_Showcase` authored spacing — plus separate small offset tunables, because **MarcusBody is scaled
   0.869**, so the authored mannequin-to-mannequin spacing will need a nudge to make hands land.

---

## 5. The camera (the daring part, done the safe way)

**Mechanism: `CalcCamera` override + state bool. NOT SetViewTargetWithBlend, NOT a spring arm.**

Why this is the only correct option on this rig (verified in Source/):
- `CalcCamera` already has final say and already fully owns the camera during ladder climbs
  (`ZP_GraceCharacter.cpp:1315-1360`) — "No camera detach needed — CalcCamera override has final say"
  is an existing, PIE-proven pattern, and it beats the Kinemation `AC_FirstPersonCamera` per-tick rotation
  writes the same way the ladder does.
- `SetViewTargetWithBlend` to a separate camera actor would change the view owner → **`MarcusBody`
  (OnlyOwnerSee) would VANISH and the hidden `PlayerMesh` (OwnerNoSee, holding the weapon) would APPEAR** —
  the exact wrong result — and view-target manipulation already has a documented failure history here
  (checkpoint `death_respawn_vignette_v2_20260305.md`).
- No spring arm exists anywhere in Source/ and adding one for a 3-second beat buys us its collision probe
  and nothing else we need.

Implementation:
- During Entry, blend (ease-out, `GrabCamBlendIn` s) from the current FP camera transform to an
  over-the-right-shoulder frame computed in `CalcCamera`: start at Marcus's head, offset
  `GrabCamBack/GrabCamRight/GrabCamUp`, aimed at a point between Marcus's head and the Shambler's head.
  (Genre-typical OTS offsets run ~40-60 right, arm 100-200 back — our defaults in section 8c, dev tunes.)
- **Wall safety:** single sphere trace (radius ~12) from head to desired camera position each frame,
  clamp to hit — hand-rolled spring-arm probe, three lines, no component.
- Look input ignored during the grab (`Input_Look` already returns on state bools — add the grab gate).
  `ControlRotation` is NOT changed by the 3P camera; on return to FP there is no snap because control
  rotation never moved (the ladder exit recipe restores `bUseControllerRotationYaw`).
- Return blend: ease back to the live FP camera transform over `GrabCamBlendOut` s — during the tail of
  the Kick/Push clip on success, or at the START of the knockdown on fail (the request: camera back to
  1P, then it follows the fall — section 7).
- **Damage vignette during the grab:** new tiny API on `UZP_HUDWidget` — hold `DamageVignetteOpacity` at
  `GrabVignetteHold` while grabbed (bite ticks additionally pulse it to max via the normal
  `OnHealthChanged` path, which fires because damage routes through `HealthComp` — section 9). The
  existing NativeTick fade handles release. Mash prompt widget: **exposed FText field, neutral
  placeholder** — you author the wording, I never bake player-facing text.

---

## 6. (folded into 4 & 5 — alignment and camera)

## 7. Mesh & visibility matrix per phase

The only full-body player mesh is **MarcusBody** (CCMH, "FULL first-person body — only the head is
hidden", scaled 0.869, OnlyOwnerSee — which is fine, the owner is still the viewer under CalcCamera).

| Phase | Shambler mesh | MarcusBody (CCMH) | PlayerMesh / view models | hidden Mesh (Manny) | Camera |
|---|---|---|---|---|---|
| Entry / Munch / Wrestle / Escape | `A_Shambler_Grab_*` via DefaultSlot dynamic montage | CCMH-retargeted `Human_*` clips, SingleNode; **head bone UN-hidden** for the window (it IS on camera now); clavicle-hide suspended; loco tick suspended | PlayerMesh stays OwnerNoSee-hidden; **MeleeViewMesh/RangedArms/grenade view models force-hidden** (they're camera children — they'd float in the 3P frame); weapon state remembered (ladder `PreLadderWeaponClass` recipe) | untouched (hidden) | 3P OTS via CalcCamera |
| Fail → Knockdown | `Zombie_Grab_To_TakeDown` (recommended — pack's authored shove-down; or hold last wrestle frame → dev call) | **hidden for the fall** (or plays a CCMH knockdown retarget later — v2 polish) | — | **Manny-retargeted `FPP_Dag_Knockdown` plays here, `bCopyAllBones=true`** → full-body copy drives PlayerMesh → **camera rides the FPCamera socket down** — this is exactly the ladder full-body-copy mechanism, already proven | 1P (blended back at knockdown start), follows socket |
| Get-up | Chase resumes / backs off | hidden until stood | Manny-retargeted `Get_Up_Back`, same route | camera rides socket back up | 1P |
| Back to play | normal | restored: head re-hidden, clavicle logic + loco tick resumed | weapon re-equipped (`PreGrabWeaponClass`) | `bCopyAllBones=false` | FP |

Notes:
- The knockdown route through **hidden Mesh + bCopyAllBones** is chosen because it's the one proven way
  full-body motion moves the camera on this rig (ladders, `.cpp:5111-5114`). The FPP clip is
  camera-space-authored so the ride should read correctly — first PIE checkpoint verifies exactly this.
- `AlwaysTickPoseAndRefreshBones` is already set on the relevant meshes (verified for Mesh; confirm
  MarcusBody) so there is no stale-pose T-pose frame on the swap.
- Marcus head unhide/rehide + suspended clavicle-hide must restore on EVERY exit path including death
  (single `EndGrab(bAborted)` cleanup function, called from `HandleDeath` too).

## 8. Struggle QTE, block, and grab rules — all tunables are C++ UPROPERTY defaults in the .h (no magic numbers, no DataAssets per current project rules)

### 8a. Player-side
- Meter: `EscapeProgress` 0→1. Per LMB press `+MashGainPerPress`; decays `MashDecayPerSecond * dt`
  every tick (framerate-independent by construction — press counting, not polling). Threshold 1.0 wins.
  `StruggleTimeLimit` seconds after Munch begins loses.
- **Accessibility hook built in day one:** the meter reads an abstract "drive" — `EGrabEscapeInput { Tap, Hold }`
  UPROPERTY; Hold mode fills at `HoldFillPerSecond`. One enum, no rewrite later, ships with Tap default.
- **Block deflects the grab** (the request): checked at grab initiation — if `bIsBlocking` (melee block,
  RMB, already shipped), grab is deflected: Shambler gets the existing block-stagger treatment
  (`BlockStaggerDuration` machinery), normal block stamina cost applies, grab cooldown starts.
  NOTE: block only exists with a melee weapon equipped — ranged/unarmed players cannot block a grab.
  Their outs: don't be in range, dodge (below), or — recommended — shoot the grabber enough to break it
  (section 4). Flagged in DEV DECISIONS.
- **Dodge:** a Space-dodge that carries the player beyond `GrabHitRange` before the 0.6 s Entry latch
  point = whiff (Shambler plays entry into air, brief recovery). No new code beyond the range re-check
  at latch — same pattern as `AttackHitRange` re-check at the swing impact frame.

### 8b. Grab fairness rules (Shambler-side, the research-mandated ones)
- `GrabCooldown` per zombie (default 8 s) — a zombie that grabbed (or was deflected) can't grab again soon.
- `PostEscapeGrabImmunity` on the player (default 3 s) — NO zombie may grab during it (Condemned rule).
- `bNoGrabAsOpener` (default true) — a Shambler's first attack after aggroing can't be the grab
  (The Beast patch rule; implemented as "grab requires LastAttackTime != never").
- Only ONE grabber ever: global gate = `GrabPhase != None` on the player; other enemies keep swinging
  normally (their damage lands, vignette pulses) but cannot initiate a second grab.
- Entry deals zero damage; first bite lands at the authored bite frame of the first Munch loop —
  a fast reaction (block-break research recommendation aside, mash starts Wrestle immediately) means
  the whole encounter can cost 0 HP. This is deliberate (free-escape window).

### 8c. Proposed tunables block (all `UPROPERTY(EditAnywhere, Category="Grab|...")`, defaults in `ZP_GraceCharacter.h` / `ZP_ShamblerBehaviorComponent.h`)

| Name | Default | Notes |
|---|---|---|
| `GrabRange` | 200 | commit distance (inside AttackRange 230) |
| `GrabPairDistance` | ~110 | measure from pack showcase, then tune (0.869 scale!) |
| `GrabCooldown` | 8.0 s | per zombie |
| `PostEscapeGrabImmunity` | 3.0 s | player-side |
| `bNoGrabAsOpener` | true | |
| `MunchDamagePerBite` | 10 | once per 2.33 s loop, at `MunchBiteTime` |
| `MunchBiteTime` | 1.1 s | authored bite frame — eyeball the clip |
| `MashGainPerPress` | 0.12 | ~9 clean presses |
| `MashDecayPerSecond` | 0.25 | |
| `StruggleTimeLimit` | 4.0 s | from Munch start |
| `FailDamageChunk` | 20 | on knockdown |
| `EscapeShoveSpeed` | 450 | LaunchCharacter on Shambler |
| `EscapeStunDuration` | 1.5 s | Shambler staggered after kick/push |
| `GrabCamBack / Right / Up` | 150 / 55 / 25 | OTS frame — **you tune this in PIE, it's a feel call** |
| `GrabCamBlendIn / Out` | 0.35 / 0.3 s | |
| `GrabVignetteHold` | 0.45 | vignette floor while grabbed |
| `EscapeInputMode` | Tap | Tap / Hold accessibility |
| `HoldFillPerSecond` | 0.5 | Hold mode |

## 9. Damage routing & edge cases

- **All grab damage goes through the normal path**: `UGameplayStatics::ApplyDamage` → `TakeDamage` →
  `HealthComp->ApplyDamage` — the HUD vignette and low-health post-process come free via `OnHealthChanged`.
  ONE exception inside `TakeDamage`: the camera-flinch `AddPitchInput/AddYawInput` block is **skipped
  while `GrabPhase != None`** (it would pollute ControlRotation and corrupt the FP return — same reason
  it's already skipped while blocking). Block damage-reduction must NOT apply to bite ticks (you can't
  hold a block mid-wrestle) — bite ticks bypass the block branch via the grab gate.
- **Death mid-grab** (bite or fail chunk): `EndGrab(bAborted=true)` runs before `HandleDeath` proceeds —
  restores collision ignores, movement modes, bone hides, camera state — then the normal fade-to-black
  respawn. The death fade simply runs from wherever the camera is; acceptable.
- **Zombie killed mid-grab** (grenade, another enemy): `OnOwnerDied` → `AbortGrab` on the player →
  clean release, no escape-reward anim, camera blends home.
- **Save/menus:** `IsModalMenuOpen()`-family bool gates get the grab sibling; save menu, inventory, map
  are unreachable mid-grab (they already gate on state bools). Nothing about a grab ever serializes.
- **Knockdown near walls/ledges:** knockdown is in-place (no root motion), capsule doesn't travel —
  no displacement problem. The 3P camera's sphere-trace clamp covers tight corridors during the wrestle.
- **Second-floor/nav edge:** Shambler in Grab state is `MOVE_None` + timers only — Evaluate skips it
  (same skip flag pattern as `bStaggered`), so no MoveTo can fire mid-grab.

## 10. SFX / blood (hooks only, you author content)

- All grab SFX route through `UZP_SFXStatics` carry profiles (project rule — no bare PlaySoundAtLocation).
  Exposed `USoundBase*` UPROPERTY slots: `GrabLatchSound`, `MunchBiteSound` (per bite tick),
  `StruggleLoopSound`, `EscapeShoveSound`, `KnockdownBodyfallSound`. All default null — you fill them
  (your studio, your call; SFX_ naming).
- Bite ticks can optionally fire the existing `UZP_BloodFXComponent` pathway at the neck/shoulder —
  BloodIntensity 1, off by default (`bGrabBloodFX=false`) until you see it.

## 11. DEV DECISIONS — flagged for you (everything visible/feel; I don't decide these)

1. **Corrected struggle pairing** (section 2a): Munch pair = bite phase, Wrestle pair = struggle. Sign off
   or tell me to run your literal pairing (it will visibly desync — lengths differ by 1.1 s).
2. **Shooting/staggering the grabber breaks the grab?** Recommended YES (ranged players need an out;
   block is melee-only). If no: ranged-armed players eat the full struggle every time.
3. **Escape stun window** on the Shambler after kick/push (1.5 s, punishable): keep, lengthen, or drop
   (research: this is the single most-loved part of Dying Light 1's system — but it's a power-fantasy
   lever, and this is a horror game; 1.5 s is deliberately short).
4. **Fail-state Shambler anim:** `Zombie_Grab_To_TakeDown` (authored shove-down, recommended) vs holding
   the wrestle pose. Also: the pack's full TakeDown→ground-munch chain exists if you ever want a
   "downed and being eaten, mash again or die" v2 — out of scope here, noted for later.
5. **Kick/Push plays in 3P then blends home over its tail** (recommended) vs cutting to FP the instant
   the wrestle resolves. First one shows the shove; second is snappier.
6. **Camera OTS numbers + vignette hold strength** — defaults above are starting points, you tune in PIE.
7. **Marcus head + Cap_01** on camera during the 3P beat: head gets un-hidden (it must — it's on camera);
   the constructor comment says Cap_01 was omitted "Add it back only for a 3P / shadow body" — want the
   cap for this beat, yes/no?
8. **Get_Up_Back needs a brand-new CC→Manny retarget path.** Alternative: skip the get-up clip v1 and do
   a 0.8 s camera-only rise (cheaper, less body presence). Which?

## 12. Implementation order (each step = build + verify before the next)

1. **Retarget batch** (Python via MCP endpoint, offline scripts in `Scripts/Python/`):
   5 Zombie clips → necromorph (`A_Shambler_Grab_*`), 5 Human clips → CCMH (`A_Marcus_Grab_*`),
   `FPP_Dag_Knockdown` + `Get_Up_Back` → Manny. Curve-audit every output. Spawn-verify one clip per
   skeleton in the showcase map before writing any gameplay code.
2. **Shambler Grab state** (enter/exit, freeze, snap-face, clip sequence, state-forcer gates, cooldowns)
   — testable standalone: zombie grabs a stationary debug target.
3. **Player grab state + alignment + input gates + mash meter** — FP camera untouched yet; verify the
   paired bodies line up and the meter math feels right via on-screen debug.
4. **3P camera** (CalcCamera OTS + traces + blends) + mesh visibility matrix + vignette hold.
5. **Escape + fail branches** (kick/push + shove/stun; knockdown ride + get-up + recovery).
6. **Edge-case pass** (death mid-grab, zombie death mid-grab, block deflect, dodge whiff, immunity windows).
7. **PIE checklist:** camera never clips walls during wrestle; no ControlRotation snap on FP return;
   knockdown camera ride reads correctly (THE risk item); weapon restores; head re-hides; second zombie
   behaves; vignette releases; grab frequency feels fair after 10 minutes of normal play.

## 13. Research sources

Key references (full URL list in the session research bundle): Techland's Dying Light: The Beast grab
hotfix statements (Nathan Lemaire — delayed damage, no-opener rule, telegraphing); RE2R/RE3R/RE4R grab &
defensive-item wikis + GameRant grapple-mechanics comparison; TLOU I/II grab + motor-accessibility docs;
Dead Island 1/2, Callisto Protocol, Dead Space, Alien Isolation (Working Joe QTE numbers), Condemned,
ZombiU escape systems; Deus Ex: HR producer statements on FP→3P takedown cameras; Cyberpunk 2077's
all-FP counterposition; Game Developer (Gamasutra) QTE design articles; Game Accessibility Guidelines
"avoid repeated inputs"; Epic docs — Motion Warping, Contextual Animation (evaluated and NOT chosen:
experimental, no 1P/3P help, our clips have no root motion — manual snap+paired-playback is what devs
ship anyway), SetViewTargetWithBlend semantics, `EVisibilityBasedAnimTickOption`, Enhanced Input
context priorities.
