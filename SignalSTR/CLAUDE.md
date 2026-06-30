# SIGNAL — Story & Design Docs

Story/design workspace for **Signal**, a solo-dev survival-horror game (UE5).
This folder is **writing and planning**, not code. The artifacts are the design docs.

## How to work here (read this first)

This is the user's creative project. Your job is to hold the structure and write down
*their* decisions — not to design the game for them.

- **Place one piece at a time.** When given a piece of the design, record exactly that piece,
  in their words, and stop. Do not extrapolate the rest.
- **Leave blanks blank.** "Leave it blank" / "template" is a hard instruction. Empty fields
  the user controls are worth more than full ones you wrote. Never auto-fill.
- **No bloat.** Short, plain, functional. No purple prose in design docs. Volume is not value.
- **Aim for the middle.** Don't swing between a finished picture and one bare line. If unsure
  how much is wanted, give a little and ask.
- **On correction, just fix it.** Make the change without arguing or re-explaining.

## The docs

- `signal_story_bible.md` — top-level story bible (logline, synopsis, setting, character,
  void, tone, enemies, progression). The anchor doc.
- `marcus_backstory.md` — Marcus's backstory + how the void picks at it. A **texture layer**,
  not the spine of the game. Keep the personal/psychological material in proportion — this is
  survival horror, not Silent Hill 2.
- `campaign_structure.md` — campaign spine (5 chapters), chapter flow, the twist, the endings, the Guide.
- `polaris_background.md` — the org: Shinra-shaped private biotech front + inner circle chasing
  the impossible; contained-not-created; the front/secret document gradient; plantmass creatures.
- `level_map_progression.md` — per-level template. Level 1 = research building,
  Level 2 = office building. Mostly blank by design.
- `level_build1_floor3.md` — drafted vertical-slice / demo level (office building, Floor 3).
- `Apology.docx` / `make_apology.py` — one-off; not part of the design.

## Facts that are locked

- **Marcus** — janitor protagonist; everyman, not a chosen one. Curiosity, not destiny.
- **Polaris** — the org/campus name (an organization, not a person). **Shinra-shaped**: a
  private (no government) world-leading **biotech** front with an inner circle chasing the
  impossible (the far side / beating death). **Contained, not created** — they found the rift
  and held it shut. Not the antagonist. Full background: `polaris_background.md`.
- **The void** — a primordial, mindless infection from a breached rift. Not a mastermind.
  No secret-villain twist; the Guide can be grey but is never the antagonist.
- **The infection** — biological/airborne, shown as the spore-like void-particle haze. Marcus
  caught only the **slow airborne strain** (shielded from the full breach); it's **not visible
  on his body**, it works on his **consciousness**. It reaches his **deep/old memory, never his
  live mind** — which is why the void never learns about the tracker. Soft spatial clock
  (denser particles = worse). Details in `signal_story_bible.md` → The infection.
- **The Guide** — the void's **hidden victim**, not a villain: infected before first contact,
  fully puppeted, no agency. The void's mouthpiece (and the in-fiction earbud voice that knows
  too much about Marcus). The void stays the sole antagonist. Full twist + endings in
  `campaign_structure.md`.
- **Interference tracker** (term "HSO" retired — it was a phantom acronym) — the void registers
  as interference in Marcus's bluetooth earbud, spectrum-analyzer style (diegetic,
  proximity-based). Survival, detection, and threat all run through "signal." It's Marcus's
  **secret edge**: coincidental, not strategic; Guide contact is phone-calls-only, so the void
  never clues in. Don't reintroduce "HSO."
- Avoid the term "Eleven" — it was dropped.
