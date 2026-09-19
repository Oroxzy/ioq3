# Measuring the lab under latency

The detector baseline in `detector-baseline.md` was measured on a loopback game
with no delay at all. The open question is whether the separation it records
survives a realistic ping — a detector that only works at 0 ms is not a
detector. This is how to find out.

## What latency does and does not touch

`net_loopDelay` holds loopback packets back in each direction, so it delays the
path between **your client and the server**. Bots run inside the server and
never touch that path, so their shooting is unaffected by construction. That is
useful rather than a limitation: the bots stay a fixed control while the player
side moves, so any shift in the gap is the player's.

Calibration, read off the server's own `status` ping column:

| `net_loopDelay` | ping |
|---|---|
| 0 | 0 |
| 10 | 50 |
| 25 | 77 |
| 40 | 101 |

Roughly twice the delay, plus the ~25 ms the `sv_fps 20` snapshot rate
quantises to. Pick the delay for the ping you want to simulate, not the other
way round.

## The runs

Three sessions, one per condition, **as alike as possible**: same map, same bot
roster and skill, same weapons, same style of play, and long enough to matter.
The baseline was built from roughly 1700 machinegun shots; a few hundred per
condition is the least that is worth comparing.

| Run | `net_loopDelay` | simulated ping |
|---|---|---|
| A | 0 | 0 (reference) |
| B | 25 | ~77 |
| C | 50 | ~125 |

Set it in the bench under **Spiel → Netz-Verzögerung**, then play normally. The
condition is written into each log's version line, so the runs cannot be mixed
up afterwards:

```
aim log: version 8 built … netdelay 25 netloss 0.0 frame 700
```

Keep the assist on and unchanged across all three — the question is how the
same setup behaves, not which setup is best.

## What to compare afterwards

Per condition, from the logs:

1. **Body-hit share, player.** `aim impact: bullet-flesh` versus `bullet-wall`
   with `other 0`. This is the baseline's headline channel. Note the shooter is
   the `other` field, **not** `client`.
2. **Body-hit share, bots** (`other 1/2/3`). Should not move. If it does, the
   run was not comparable and something else changed.
3. **Residual aim error at the shot**, from `aim shot: … error`. The baseline
   has 72.7 % of shots inside 0.5°.
4. **Swing on the firing frame**, `swing > 5`. 15.9 % overall, 36.6 % for
   rockets. This is the second detector channel and the one most likely to
   change with lag, since the assist is steering against a staler picture.
5. **Hit-sound timing.** The client line now carries `frame`, and the server's
   `hit on …` carries `level.time`; the gap between them is the feedback
   latency, which is exactly what the delay should stretch.
6. **Whether the learned tables still converge.** `aim tune:` `was`/`now` and
   the sample counts.

## The learned tables do not come into it

Every number above is a per-shot line in the log. None of them is read from
`aimtune.cfg` or `aimrate.cfg`, so those tables need no protecting for this
measurement and there is nothing to back up or restore between runs.

They are worth a sentence anyway, because it is easy to assume otherwise: the
lead table is keyed by weapon, flight-time band and target speed, and has **no
latency dimension at all**. A run at 50 ms therefore lands in the same boxes as
a run at 0 ms. That does not affect the measurement, but it does mean the table
ends up a mixture of conditions. If that matters, switch **Zielen → Vorhalt
lernen** off (`cl_aimAssistLearn 0`) for the series — one checkbox, and nothing
is learned while measuring. Do not build a per-latency lead table to fix it;
that is aim accuracy, which is out of scope for this project.

## One real trap

**A quiet run proves nothing.** A three-minute bot-versus-bot run was tried and
produced 104–207 impacts per shooter, at which point the spread *between the
three bots inside one condition* (6.8 % to 23.3 %) was as wide as the spread
between conditions — which is chance, since bots run inside the server and the
delay cannot reach them at all. The noise floor at that sample size swallows
everything being looked for. Play each condition long enough to produce a few
hundred of your own shots.
