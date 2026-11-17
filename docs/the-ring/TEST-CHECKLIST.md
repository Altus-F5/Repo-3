# THE RING demo — acceptance checklist

**Every item is NOT RUN.** No Unreal Editor was available where this was written.
Fill in Result when someone runs it; put screenshots in `production/qa/evidence/`.

Start each run from Play In Editor on the Entry map. Use **P** to pause in the editor.

| # | Test | Expected | Result |
|---|---|---|---|
| 0 | Open `TheRing.uproject`, build | Module compiles; editor opens | NOT RUN |
| 1 | Automation: `TheRing.Logic.*` | 5 tests pass | NOT RUN |
| 2 | Press Play | Fade in to the staging room; Elysia visible from behind; three intro subtitles in order, never overlapping | NOT RUN |
| 3 | Move, look, brisk walk | WASD relative to camera; mouse up looks up; Shift is faster; no jump | NOT RUN |
| 4 | Camera against walls and in the corridor | Pulls in, never shows through a wall, no flip when turning round | NOT RUN |
| 5 | E at each researcher and at the gap | One line each; nothing while another line is playing | NOT RUN |
| 6 | Look out of the gap | Ring band curving up both ways; cannot climb out | NOT RUN |
| 7 | Focus at the partition before clearing the bar | Brackets, "(CANNOT MOVE)", "Braced shut" prompt; left mouse does nothing but explain | NOT RUN |
| 8 | Take the brace bar, walk with it, wheel in and out, let go | Follows the view at a bounded distance; stops at walls and at Elysia; settles on the floor with a thud; objective advances | NOT RUN |
| 9 | Put the bar back in its brackets | Partition is braced again; nothing breaks; can be cleared again | NOT RUN |
| 10 | Carry the bar out through the gap and let go | It returns to its brackets | NOT RUN |
| 11 | Slide the partition | Moves only along its track; stops at both ends; will not move onto Elysia in the doorway; stays where left | NOT RUN |
| 12 | Walk at the floor gap | Cannot walk or fall into it | NOT RUN |
| 13 | Carry the panel to the outline, let go near it | "IN POSITION" label, seats itself, can be crossed | NOT RUN |
| 14 | Let go of the panel over the pit | It returns to where it was found | NOT RUN |
| 15 | Stand on the seated panel and try to lift it | Refused: "Step off it first." | NOT RUN |
| 16 | Cross, lift the panel from the far side, re-seat it | Works from both sides | NOT RUN |
| 17 | Walk far away while holding something | "Too far. Lost hold." and it drops | NOT RUN |
| 18 | Pause → Reset puzzle objects, on each side of the partition | Staging side: all as found. Beyond: route stays open, panel returns, player never trapped or dropped | NOT RUN |
| 19 | Enter the corridor | Suit light comes on; no lit fixtures anywhere; "No power" line once; "discrepancy" lines once | NOT RUN |
| 20 | Pass the checkpoint | "Checkpoint saved" toast, once | NOT RUN |
| 21 | Turn the corner | Murmur audible and captioned; "Going passive" once; suit light and drone indicator go out near the chamber | NOT RUN |
| 22 | Try to enter the chamber | Blocked at the debris; can see over it | NOT RUN |
| 23 | Watch the encounter | Back to the door in the diamond prayer pose → arms lower → crosses to the recess → small right arm takes the object → stillness → head and upper body turn to profile → hold → turn back. No chase, no reaction to movement, light, or noise | NOT RUN |
| 24 | Voraik model check | One pair of long arms to the floor, one small pair, narrow forward-flared head, single vertical red line, no flesh | NOT RUN |
| 25 | Focus view during the encounter, both shoulders (Q) | Over-shoulder framing with Elysia in frame; no clipping through the wall | NOT RUN |
| 26 | Pause during the encounter for 30 s, resume | Timeline and subtitles resume exactly where they stopped; no input leaks while paused | NOT RUN |
| 27 | Walk away mid-encounter | Objective changes to return; ending still plays at the team | NOT RUN |
| 28 | Restart from checkpoint during the encounter | Reloads in the corridor; route behind is open and bridged; encounter plays again from the start | NOT RUN |
| 29 | Return to the staging room | The five lines in order; researchers face Elysia; silence; fade; end card | NOT RUN |
| 30 | End card → Replay | Starts from the beginning with everything reset | NOT RUN |
| 31 | Options: each of the 12 settings | Takes effect; survives quitting and relaunching | NOT RUN |
| 32 | Subtitles off | No dialogue text; prompts and objective remain | NOT RUN |
| 33 | Reduced camera motion | No camera lag, no change of field of view when focusing | NOT RUN |
| 34 | Focus set to Toggle | One press enters focus, one leaves; nothing has to be held | NOT RUN |
| 35 | Gamepad | Move, look, focus, take, distance, interact, pause, menus | NOT RUN |
| 36 | Whole route start to finish, twice in one session | 5–8 minutes; no soft lock; no repeated or overlapping line | NOT RUN |
| 37 | Package for Windows and repeat 2, 8, 13, 23, 29 | Same behaviour as in editor | NOT RUN |
