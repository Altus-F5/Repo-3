# THE RING demo — handoff for a local session

You are picking up a UE5 project that was written entirely without Unreal.
Your job is to make it build, run it, test it honestly, and fix what is wrong.
Read this file, then `docs/the-ring/README.md`.

## Where things stand

- **Repo / branch:** `Altus-F5/Repo-3`, branch `claude/focused-cray-88x7yo`. Work and commit there.
- **Project:** `TheRing.uproject` at the repo root, `EngineAssociation` 5.8. The owner's PC has UE 5.8 installed (a folder named `UE_5.8`).
- **Never compiled, never run.** About 4,800 lines of C++ in `Source/TheRing/`, plus `Config/*.ini`. No `.uasset`, no `.umap`, no Blueprints, no packaged build, no screenshots, no test results.
- One read-through review was done in place of a compiler. Its definite findings are fixed. Its unconfirmed doubts are listed below.
- `docs/the-ring/TEST-CHECKLIST.md` has 38 acceptance tests, every one marked NOT RUN.

## How the project works (so you do not "fix" the design)

- It runs on the engine's empty map `/Engine/Maps/Entry`. `ARingGameMode::StartPlay` spawns the whole level from `/Engine/BasicShapes` meshes. The editor viewport is black until Play. That is intended.
- Input: Enhanced Input, with actions and the mapping context created by `NewObject` in `ARingPlayerController`. No input assets.
- UI: `ARingHUD` draws everything on the canvas, menus included. Menus are keyboard / gamepad only.
- Audio: `URingSynthComponent` (a `USynthComponent`) synthesises all sound.
- Characters: `URingBodyComponent` (Elysia and two researchers) and `ARingVoraik` are **placeholders** made of basic shapes, animated in code.
- Progression: `ERingStage` in `ARingGameMode`, advancing one stage at a time. Restart reloads the map.
- Tuning: `Config/DefaultGame.ini`, section `[/Script/TheRing.RingTuning]`.
- It is C++ because no editor was available to author Blueprints. With an editor you may move things into assets, but get it running as written first.

## Step 1 — build

Find the engine root (the folder containing `Engine\`), then from the repo root:

```bat
"<UE>\Engine\Build\BatchFiles\Build.bat" TheRingEditor Win64 Development -Project="%CD%\TheRing.uproject" -WaitMutex
```

Needs Visual Studio 2022 with the C++ game workload. Fix compile errors until it links. Do not delete features to get green; fix the call.

Doubts the review could not settle (check these first if they error):

- `AHUD::GetTextSize` and `AHUD::Project` are called from `const` members in `RingHUD.cpp`. If either is non-const, drop `const` from `Measure` and `Wrap`.
- `FVector::GetSafeNormal(Tolerance, ResultIfZero)` two-argument form in `RingBuild.cpp`.
- `StaticLoadObject(..., Path, {}, LOAD_NoWarn | LOAD_Quiet)` in `RingBuild.cpp`.
- Module dependencies for `Components/SynthComponent.h`: `AudioMixer`, `AudioExtensions`, `SignalProcessing` are listed in `TheRing.Build.cs`; remove any the build rejects.
- `USynthComponent` members used in the constructor: `NumChannels`, `bAllowSpatialization`, `bIsUISound`.
- `UInputAction::bTriggerWhenPaused` / `bConsumeInput`, `UInputMappingContext::MapKey` returning a mapping whose `Modifiers` can be appended to.
- `ULocalLightComponent::SetIntensityUnits(ELightUnits::Candelas)`.
- `TEXT("Brightness: %+.2f stops")` under format-string validation.
- Anonymous-namespace names colliding in unity builds (`Hull`, `Deck`, `Rib`, `Ink`, `Dim`, `Shade`, `Gravity`, `CapsuleRadius`...). Rename on C4459 / C2084.
- Anything changed between 5.6 and 5.8. The repo's notes are in `docs/engine-reference/unreal/` (`breaking-changes.md`, `deprecated-apis.md`). Read them before guessing an API.

## Step 2 — unit tests

```bat
"<UE>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%CD%\TheRing.uproject" -ExecCmds="Automation RunTests TheRing.Logic; Quit" -unattended -nullrhi -log
```

Five tests in `Source/TheRing/Private/Tests/RingLogicTests.cpp`.

## Step 3 — run it and look

```bat
"<UE>\Engine\Binaries\Win64\UnrealEditor.exe" "%CD%\TheRing.uproject" -game -windowed -ResX=1600 -ResY=900 -log
```

Or open the project and press Play. **In the editor Escape ends Play; P pauses.**
Take real screenshots (`HighResShot 1` in the console) into `production/qa/evidence/`.

Things most likely to be wrong on first run, in rough order:

1. **Startup.** Does `GlobalDefaultGameMode` take effect on the Entry map; is an `ARingCharacter` spawned; does `PlacePlayer` move it into the staging room. If the pawn spawns before any floor exists and falls, `PlacePlayer` should still teleport it in the same frame.
2. **Materials.** `BasicShapeMaterial` is assumed to have a vector parameter named `Color`. If everything is white or grey, that name is wrong. Run `Tools/Editor/create_ring_materials.py` (unverified too) to get `M_RingSurface` / `M_RingEmissive` with `Color`, `Roughness`, `Metallic`, `Strength`.
3. **Exposure and light levels.** All chosen by arithmetic. Expect to retune `ExposureMinEV`, `ExposureMaxEV`, `ExposureBiasDark`, `StarLux`, `WorkLightCandela`, `SuitLightCandela`, `ChamberRimCandela`. The dark corridor must be readable; the chamber must stay visibly unpowered.
4. **Look direction.** `ApplyLook` assumes mouse-up is positive Y. If pitch is inverted by default, flip the sign there, not the option's default.
5. **Input while paused.** Menu actions rely on `bTriggerWhenPaused`. If the pause menu cannot be navigated or closed, that is the place.
6. **Synth.** If there is no sound, check `Synth->Start()` in `ARingCharacter::BeginPlay` and the module dependencies.
7. **Telekinesis collision.** `ARingTKObject::IsSpaceFree` uses a box overlap on `ECC_WorldDynamic`. If the bar or panel will not move at all, something overlaps at rest (brackets, floor). If it passes through walls, the walls are not blocking that channel.
8. **Partition.** Slides +Y from (630, 0, 159) into a wall pocket. Check it clears the doorway and the pocket hides it.
9. **Camera.** Spring arm collision in the 3 m wide corridor and the alcove.
10. **Figures.** Limb directions are computed from vectors, so they should not be mirrored, but check the walk reads as a walk and the telekinesis arm is the right one.
11. **Voraik.** Prayer diamond above the head, long arm tips level with the feet in neutral, the right small arm reaching the shelf object, the head turning to profile. Compare against the owner's reference art.
12. **Vista.** Ring segments through the staging-room gap. Check scale reads and nothing clips the far plane.

## Step 4 — the checklist

Work through `docs/the-ring/TEST-CHECKLIST.md` top to bottom and replace each NOT RUN with what actually happened. Report failures as failures. Fix, re-run, commit.

Then try packaging:

```bat
"<UE>\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%CD%\TheRing.uproject" -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="%CD%\Saved\Package"
```

Meshes and materials are loaded by path, so the cook list in `Config/DefaultGame.ini` matters. If the packaged game is empty or untextured, that is why.

## What the owner asked for (do not drift from this)

The owner's full brief and seven concept sheets are not in the repo. Ask for them. The binding points:

- UE5, PC, third-person. No other engine, no browser game. Small connected level, 5–8 minutes.
- **Elysia**, early twenties, expedition suit per the art. Telekinesis is deliberate force, not spellcasting. No flight, no destruction, no combat.
- **Strategic** speaks through a relay drone: a solid, flattened, elongated disc with **no hole**, docked flush on her back. Never humanoid, never a sphere, never a ring. Quiet and concealed during the encounter.
- **Two unnamed researchers** (survey lead, expedition engineer) stay at the staging area.
- **Voraik:** ONE pair of long support arms ending level with the pointed feet, ONE pair of small manipulation arms, plus legs. Tall, fully enclosed matte-black armour and layered garments, no flesh, not a generic robot. Narrow head, shallow flare mainly forward, flatter rear, one continuous vertical red line. The owner is building his real model in a separate session; expect to swap it in (see README, "Putting the real Voraik model in"). Inspect any `.blend` before trusting it.
- **Knowledge boundaries.** Nothing player-facing may name Voraik, the Che, their allegiance, history, or any link to Sol, or reveal who destroyed the ring. The researchers do not recognise him. The cloaked ship is only hinted by one restrained Strategic line.
- **This is a later expedition**, weeks after earlier discoveries; the station is away. Not first contact with the ring.
- **The far sector is UNPOWERED, not lower.** No glowing wall panels there. Suit light and faint spill only.
- **Encounter:** he starts with his back to the doorway in the prayer pose; unfamiliar speech, captioned, with no invented translation; he takes a small unidentified object from a recess with a small arm; pauses as if sensing something; head and upper body turn to profile. No chase, no attack, no jump scare, no certainty that he noticed. Elysia stays unseen. A physical limit keeps the player out of the chamber. Third-person camera throughout.
- **Closing lines, exactly:** "Someone is here." / "Another research team is here?" / "Not human." / "Did it see you?" / "I don't think so." Then uncertainty and an end card.
- **Controls:** WASD, mouse orbit, Shift faster walk, E interact, RMB focus, LMB take/release, wheel distance, Esc pause. Toggle option for aim.
- **Options:** subtitles on by default, volume, sensitivity, invert-Y, reduced camera motion, brightness, restart / checkpoint. Nothing conveyed by colour alone.
- **Robustness:** triggers fire once, dialogue never overlaps, pause stops input and story time, restart resets everything, checkpoint before the encounter, puzzles repeatable with no soft lock.
- **Honesty:** label placeholders; never claim a build, asset or test that does not exist. Room layout, puzzle objects, extra dialogue and timings are demo adaptations, not canon — keep that distinction in the docs.
- No paid assets, no invented insignia, no unrequested monsters or fantasy ruins.

## Repo rules that apply

`CLAUDE.md` at the root sets them. The ones that bite here: ask before writing files unless the owner has told you to carry on; Conventional Commits; a retained screenshot in `production/qa/evidence/` before visual work is called done; never skip a failing test to get green.

## Done means

The owner can open the project, move around the ring, use both telekinesis interactions, witness the encounter, return to the team and reach the end card with no broken control and no soft lock — and the checklist says so with real results.
