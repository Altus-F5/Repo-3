# THE RING (working title) — opening-scene demo

A 5–8 minute third-person exploration slice in Unreal Engine 5, played as Elysia:
arrive at the staging area on the ring, use telekinesis to get through a braced
partition and across a broken floor, walk an unpowered corridor, and watch an
unfamiliar being at a quiet ritual without being seen. Return to the team. End.

"THE RING" is a temporary demo title, not the name of the universe.

## Status: written, NOT compiled, NOT run

**This project has never been opened in Unreal.** It was authored in an
environment with no Unreal Editor, no UnrealBuildTool and no engine headers.
Nothing below has been compiled, played, packaged or screenshotted.

What exists is real, complete source for a UE5 C++ project. What does not exist:

- a compiled binary, a packaged Windows build, or any `.uasset` / `.umap`
- a single test result, Play-In-Editor session, screenshot or recording
- Blueprint assets (see "Why C++" below)

Expect to fix some compile errors on first build. The code was read through by a
second reviewer acting as a compiler, but that is not a compiler. Treat every
gameplay claim in this file as *intended behaviour*, and use
[`TEST-CHECKLIST.md`](TEST-CHECKLIST.md) to find out what is true.

## Open and play

| | |
|---|---|
| Engine | Unreal Engine **5.5** (`EngineAssociation` in `TheRing.uproject`). Written to compile on 5.4–5.6. For another version: right-click `TheRing.uproject` → *Switch Unreal Engine version*. |
| Needs | Visual Studio 2022 with the *Game development with C++* workload (this is a C++ project; the editor must build it). |
| Plugins | `EnhancedInput` (ships with the engine; enabled in the `.uproject`). Nothing else. |
| Startup map | `/Engine/Maps/Entry` — the engine's empty map. The level is built in code when Play starts. |
| Game mode | `ARingGameMode`, set as the global default in `Config/DefaultEngine.ini`. |

1. Double-click `TheRing.uproject`. Answer **Yes** to "rebuild missing modules".
   If it fails, right-click the `.uproject` → *Generate Visual Studio project files*,
   open `TheRing.sln`, build *Development Editor / Win64*, and read the errors there.
2. The editor opens on the empty Entry map. **The viewport is black and empty in
   edit mode — that is expected.** Nothing exists until Play.
3. Press **Play** (Selected Viewport or New Editor Window), click in the viewport.
4. **In the editor, Escape ends Play.** Use **P** to pause. Escape pauses in a
   standalone or packaged game.
5. Optional, recommended: run `Tools/Editor/create_ring_materials.py` once
   (*Tools → Execute Python Script*), then Play again. It adds roughness, the
   glossy chamber floor and everything that glows. Also unverified.

Unit tests for the pure rules: *Tools → Session Frontend → Automation*, filter `TheRing.Logic`.

Packaging (*Platforms → Windows → Package Project*) is untried. `Config/DefaultGame.ini`
lists the engine folders that must be cooked because meshes are loaded by path.

## Controls

| Action | Keyboard / mouse | Gamepad |
|---|---|---|
| Move | W A S D | Left stick |
| Look | Mouse | Right stick |
| Brisk walk | Hold Left Shift | Hold left stick click |
| Interact | E | A / Cross |
| Focus telekinesis | Right mouse (hold; toggle in Options) | Left trigger |
| Take hold / let go | Left mouse | Right trigger |
| Held object nearer / farther | Mouse wheel | LB / RB |
| Swap camera shoulder | Q | Right stick click |
| Pause | P or Escape | Start |
| Menus | Arrows or W/S, A/D to change, Enter or Space, Backspace | D-pad, A, B |

On-screen prompts name keyboard and mouse only. There is no jump and no combat.

**Telekinesis.** Focus, aim at an object marked with two brass bands (it gets
corner brackets and its name), take hold, then look and walk to move it. It stays
between 1.4 m and 5.2 m from her, cannot pass through walls, and is dropped if she
gets more than about 11 m away. Let go and it settles straight down.

**Options** (Pause → Options): subtitles on/off and size, four volume sliders,
camera sensitivity, invert vertical look, reduced camera motion, brightness,
hold/toggle focus, graphics quality. Saved between sessions.

**Recovery** (Pause): *Restart from checkpoint* (saved in the corridor before the
encounter), *Reset puzzle objects*, *Restart demo*. Restart reloads the map, and
the map is rebuilt from code, so nothing carries over.

## The sequence

1. **Staging.** Two researchers, cases, a work light, and a long gap looking out
   over the ring. E talks to either of them or looks out.
2. **Brace bar.** Lift the bar off the partition and put it anywhere.
3. **Partition.** Take hold of the partition and look along its track. It only
   slides on that track. It will not close on anyone in the doorway.
4. **Broken floor.** Carry the deck panel to the outlined support position and
   let go near it; it seats itself. The gap cannot be walked into.
5. **Unpowered corridor.** Suit light on. Strategic logs a reading it cannot
   resolve. Checkpoint.
6. **The chamber.** The direct way in is choked with collapsed structure. From
   the corridor or the alcove she watches: prayer with his back to her, arms
   lowered, a walk to the far-wall recess, a small object taken with a
   manipulation arm, a pause, the head and upper body turning to profile, and back.
   He never reacts to her. Leaving early still counts as having seen him.
7. **Return.** Back to the team, the five-line exchange, silence, end card.

## Canon and demo adaptation

Kept from the brief as established: this is a later expedition, not first
contact with the ring; the station is away; the far sector differs by having
**no power**, not by being lower; the being starts with his back to the doorway;
he is not identified and Elysia is not seen; the researchers first assume another
research team.

**Demo adaptations, not canon:** the room layout and route; the brace bar,
sliding partition, deck panel and floor gap; the alcove and the debris; all
dialogue other than the five closing lines; Strategic's "mass reading" line (the
only hint of the cloaked ship); the object's shape; every timing; and both
researchers' looks and lines.

The names *Voraik* and *Che* appear only in source comments and this document.
Nothing player-facing names him, his species, allegiance or history, and nothing
refers to who destroyed the ring.

## Asset and source manifest

There are **no imported assets and no third-party content**. Everything visible
is engine basic shapes placed by code; everything audible is synthesised by code.

| What | Kind | Where |
|---|---|---|
| All source, config, this documentation | Original, written for this demo | `Source/`, `Config/`, `docs/the-ring/` |
| Level (rooms, vista, props, lights) | Procedural geometry | `RingGameMode.cpp` |
| Elysia, Survey Lead, Engineer | **Placeholder** — jointed figures of basic shapes, animated in code, following the concept sheets for palette, proportion and gear | `RingBodyComponent.*` |
| Strategic's relay drone | **Placeholder** — a solid flattened elongated disc docked on Elysia's upper back; never undocks | `RingBodyComponent.cpp` |
| Voraik | **Placeholder** — simplified stand-in built to the approved reference: one pair of long support arms reaching the floor, one pair of small manipulation arms, legs, narrow forward-flared head with one vertical red line, mantle, cloak, tabard; no exposed flesh | `RingVoraik.*` |
| Sound | **Placeholder** — synthesised room tone, footsteps, hum, grind, thud, murmur, tension tone. No voice acting. | `RingSynthComponent.*` |
| Meshes, base material, font, empty map | Engine content (Epic, under the Unreal Engine EULA) | `/Engine/BasicShapes`, `/Engine/EngineMaterials`, `/Engine/Maps/Entry` |
| Two optional materials | Generated locally by script | `Tools/Editor/create_ring_materials.py` |
| Concept sheets | Reference only. Not in the repository, not shown in game. | — |

No skeleton, retargeting or animation assets are used. No marketplace content.

### Putting the real Voraik model in

`ARingVoraik` owns the encounter timeline and is otherwise a pile of shapes.
When the model from the other session is ready: import it, add a
`USkeletalMeshComponent` to `ARingVoraik`, skip `BuildBody()`, and drive the
rig from the same weights `ApplyPose()` already computes (`Pray`, `Walk`,
`Reach`, `Turn`). If it arrives as a `.blend`, check what it actually contains
(armature? animation? cloth?) before relying on any of it. The same applies to
Elysia: `URingBodyComponent` is the only thing to replace.

## Why C++ and not Blueprints

The brief prefers Blueprints. Blueprints, maps, widgets and input assets are
binary files that only the Unreal Editor can write, and no editor was available.
C++ and `.ini` are text, so the demo is entirely C++: the level is spawned in
`ARingGameMode::StartPlay`, input actions are created with `NewObject`, the HUD
is drawn on the canvas, and tuning lives in `Config/DefaultGame.ini`
(`[/Script/TheRing.RingTuning]`). The cost: nothing can be arranged in the
editor viewport until someone moves it into real assets.

## Known limitations

- **Unbuilt and untested.** See the top of this file.
- **Lighting is calculated, not looked at.** Intensities and the exposure clamp
  were chosen by arithmetic. The dark sector may be too dark or too flat. Tune
  `ExposureMinEV`, `ExposureBiasDark`, `SuitLightCandela`, `ChamberRimCandela`,
  `StarLux` in `DefaultGame.ini`, and the Brightness option.
- **Placeholder characters.** Block figures with a procedural walk. Elysia is
  recognisable by silhouette and palette only. Voraik has no cloth motion beyond
  a panel that follows his arms.
- **No voice.** Strategic and everyone else are subtitles. The murmur is shaped
  noise, not a language, and is captioned as unfamiliar speech.
- **No glow without the optional materials.** A small red light stands in for
  the face line and the drone indicator is plain paint.
- **Menus are keyboard / gamepad only.** No mouse pointer.
- **Gamepad is bound but prompts are not adapted to it.**
- **Telekinesis is kinematic.** Held objects keep their orientation and do not
  tumble; that is the price of never flinging or losing one.
- **The researchers do not walk.** They stand at the equipment and turn to face
  Elysia at the end.
- **Restart from checkpoint persists across sessions only as a flag**; a fresh
  launch always starts at the beginning.
- **Repository rule not met:** this repo requires a retained screenshot in
  `production/qa/evidence/` before visual work closes. There is none, because
  nothing has been run.
