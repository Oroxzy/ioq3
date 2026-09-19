# What a server can already see

The aim lab in this tree exists to study detection, not to play well. This is
the measured baseline: how far assisted and unassisted play separate in data a
Quake 3 **server already has**, with no client cooperation and no new engine
code.

Everything below was counted from the lab's own logs. Where a number was too
weak to support its headline it is marked, and one signal that looked strong was
thrown out for being an artifact of how the lab books shots.

## The corpus

| | |
|---|---|
| Measured | 2026-09-19 |
| Engine runs | 14 (`logfile opened` lines) |
| Log lines | 29,783 |
| Map | q3dm17 throughout |
| Opponents | 3 bots, skill 4–5 |
| Connection | `NA_LOOPBACK` (the assist runs nowhere else) |
| Shots logged | 2,726 — of which **168 unassisted**, 2,558 assisted |

The unassisted 168 are not a designed control group; they are shots the assist
happened not to steer. The real control is the bots (below), and it is better.

## Channel 1 — per-weapon accuracy

The strongest signal, and the server already counts it: `accuracy_hits` is
incremented at `code/game/g_weapon.c:203`, `accuracy_shots` at
`g_weapon.c:818/820/823`, and the pair is **already on the scoreboard**
(`g_cmds.c:66-67`). Nothing needs to be built to observe this.

Bullet impacts on a body vs on the world, grouped by who fired:

| Shooter | Body | World | Share |
|---|---|---|---|
| **Player (assisted)** | 1285 | 250 | **83.7 %** |
| Bot 1 | 433 | 1560 | 21.7 % |
| Bot 2 | 249 | 1294 | 16.1 % |
| Bot 3 | 340 | 1393 | 19.6 % |

Three controls inside the same corpus, captured by the same code path, all in a
16–22 % band. That is what makes the outlier trustworthy: the measurement cannot
be manufacturing it, or the bots would show it too.

**Quote the player figure as a range, 73–87 %, not as 83.7 %.** The numerator is
firm (1285 body impacts, corroborated within 0.7 % by 1276 server-side
`MOD_MACHINEGUN` damage lines). The denominator is not: joining the 1752
machinegun shot lines to impacts leaves 279 shots (15.9 %) with no logged
impact, and `Bullet_Fire` returns with no event at all on `SURF_NOIMPACT`, so
every shot into the sky is a server-counted miss that never appears here. The
lab's own rater independently books 74.9 %. The conclusion is unaffected at
either end of the range.

## Channel 2 — where the view was pointing when the shot left

From the 2,726 shot lines:

- **72.7 %** were released with the aim already within **0.5°** of the target.
- **15.9 %** carried a view swing of more than **5°** applied on the very frame
  the shot fired.

The swing is not uniform — it is concentrated on the weapons the assist snaps
for:

| Weapon | swing > 5° |
|---|---|
| Rocket | 312/853 (36.6 %) |
| Railgun | 12/85 (14.1 %) |
| Shotgun | 4/36 (11.1 %) |
| Machinegun | 105/1752 (6.0 %) |

A server sees this without any client help: the usercmd stream carries the view
angles, and the attack bit says which command fired. The per-shot angle delta
immediately before the attack bit is the second channel worth instrumenting.

## Thrown out: "railgun is 100 % at every range"

The lab books 84 of 84 rail shots as hits, across all five range bands
including 13/13 at 2000 units. **This is an artifact, not a finding.** Both shot
log sites are gated on a picked target (`cl_input.c:4584` and the steering path
at `:4802`), so a rail shot at nothing is never booked at all. Of 91 player rail
beams, 6 never reached the shot log and 1 was never booked. The honest figure is
**84/91 = 92.3 %**, and per-band rail behaviour is not measurable from this data.

Rocket arithmetic needs the same care: splash is counted per victim, and the
server increments `accuracy_hits` once per rocket whether it hit directly or by
splash, never both. Distinct rockets that scored: **447 of 853 = 52.4 %**.

## Joining the log lines

Server and client lines use different clocks. Measured on rail, where the join
is unambiguous, all 84/84 sit at a fixed offset:

| Line | Source | Time |
|---|---|---|
| `aim shot:` | client | W |
| `hit on …` | server (`level.time`) | W |
| `aim impact:` | client (`cl.snap.serverTime`) | W + 50 |
| `aim rated:` | client | W + 100 |

Joining these on `frame` without the offset yields **zero** matches. Note also
that `aim impact:` overloads its `other` and `client` fields — for bullet
impacts the shooter is `other`, not `client`; reading `client` gives one
meaningless group.

## What this says about detection

The two channels above need **no client change and no new server code** for the
first one, and only usercmd bookkeeping for the second. Both are properties of
the outcome and of the input stream, not of the cheat's implementation, so
neither depends on knowing how a particular assist works.

The per-range hit-rate curve the lab measures for itself is a third
discriminator of the same kind: a human's accuracy falls off with distance in a
characteristic way, and an assisted one does not fall off the same way.

## Caveats worth carrying forward

- One map, one bot roster, one player. Nothing here establishes what *unassisted
  humans* look like — the bots are a control for the measurement, not a model of
  human play. A human control set is the obvious next measurement.
- The assist was on for 94 % of the logged shots, so the within-player
  assisted/unassisted contrast rests on 168 shots.
- All figures are client-side logs of a loopback game. A real detector would
  derive its own features server-side; these numbers say the separation is
  there to be found, not that this is how to find it.
