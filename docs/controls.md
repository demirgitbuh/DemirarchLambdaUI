# Default Controls

The default layout is a modern FPS layout. Every action uses a **single press**; there are no
hold, double-press or modifier combinations by default. All actions can be reassigned, and
advanced assignments (hold, double press, modifier combinations, per-context bindings) are
available as options.

Button names: Xbox / PlayStation / Steam Deck. "LS"/"RS" are the stick clicks (L3/R3).

## Gameplay

| Control | Xbox | PlayStation | Steam Deck | Action |
|---|---|---|---|---|
| Left stick | LS | L | L | Move (graded: a small push walks slowly, a full push uses the game's normal top speed) |
| Right stick | RS | R | R | Look |
| Right trigger | RT | R2 | R2 | Primary fire |
| Left trigger | LT | L2 | L2 | The weapon's original secondary fire / scope |
| Bottom face button | A | Cross | A | Jump |
| Right face button | B | Circle | B | Crouch (hold, or toggle in settings) |
| Left face button | X | Square | X | Use / interact |
| Top face button | Y | Triangle | Y | Weapon wheel (hold to open, choose with a stick, release to equip) |
| Right bumper | RB | R1 | R1 | Reload |
| Left bumper | LB | L1 | L1 | Aim snap (single player, when aim assist is on) |
| Right stick click | RS | R3 | R3 | Target lock (single player, when enabled) |
| Left stick click | LS | L3 | L3 | Long jump assist (single player, needs the long jump module) |
| D-pad up | Up | Up | Up | Flashlight (night vision in Opposing Force) |
| D-pad down | Down | Down | Down | Last weapon |
| D-pad left | Left | Left | Left | Quick save |
| D-pad right | Right | Right | Right | Quick load (asks for confirmation by default) |
| Menu | Menu | Options | ☰ | Pause menu |
| View | View | Create/Share | ⧉ | Console |
| Touchpad swipe left/right | — | Touchpad | Trackpad (via Steam Input) | Previous / next weapon |

### Underwater (replaces gameplay bindings while swimming)

| Xbox | PlayStation | Action |
|---|---|---|
| A | Cross | Swim up |
| B | Circle | Swim down |

### Multiplayer (replaces single-player-only bindings)

| Xbox | PlayStation | Action |
|---|---|---|
| View | Create/Share | Scoreboard (while held) |
| — | Touchpad click | Scoreboard (while held) |
| D-pad left | Left | Message wheel |
| D-pad right | Right | Push to talk (while held) |
| LB | L1 | Chat (all) |
| RS | R3 | Chat (team) |
| LS | L3 | Walk |

In multiplayer, all aim assistance and gameplay automation are disabled, so the aim snap,
target lock and long jump assist bindings are replaced by communication actions.
The console is available from the pause menu.

## Menus

| Xbox | PlayStation | Action |
|---|---|---|
| D-pad / left stick | D-pad / left stick | Move focus |
| A | Cross | Select |
| B | Circle | Back |
| X | Square | Option (for example Delete on a save card) |
| Y | Triangle | Details |
| LB / RB | L1 / R1 | Previous / next tab |
| LT / RT | L2 / R2 | Page up / page down |
| Menu | Options | Close the pause menu |

## Recovery

Hold **Menu + View** (Options + Create on PlayStation) for three seconds to open the
recovery prompt and restore the default controls. This combination cannot be reassigned.
From the keyboard, type `lambda_recover confirm` in the console, or start the game with
`-lambdasafe` (see [settings.md](settings.md)).

## Why some actions are where they are

- Reload is on RB rather than X/Square, so Use and Reload never share a button.
- The weapon wheel is on Y/Triangle, the only face button left after jump, crouch and use.
- D-pad actions are things you do occasionally (flashlight, saving), not during aiming.
- Quick load asks for confirmation by default because loading by accident loses progress.
