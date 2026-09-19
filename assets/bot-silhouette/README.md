# bot-silhouette shader asset

`scripts/bot_silhouette.shader` defines the two shaders the client applies to an
enemy bot's cloned player model to draw the see-through marker:

- `botSilhouette` — a flat filled silhouette of the whole model (`rgbGen entity`
  takes the health colour, `alphaGen entity` the opacity).
- `botOutline` — an inverted-hull contour line (a frequency-0 `deformVertexes
  wave` pushes every vertex out along its normal, `cull back` keeps the rim).

The client sets `RF_DEPTHHACK` on the clone for through-wall visibility, so the
shaders themselves need no depth trick. See `CL_MaybeAddBotSilhouette` in
`code/client/cl_cgame.c`.

The whole thing stays behind the same safety gate as the rest of the aim lab:
a `NA_LOOPBACK` connection and bots only (the server's `skill` configstring).

## Packaging

`build_mingw64.bat` packs this folder's `scripts/` into
`baseq3/zz-bot-silhouette.pk3`, mirroring `zz-hitsound-qc.pk3`, so it also
loads when the local server runs `sv_pure 1` (loose files under `scripts/`
would not be found then). If the pk3 is missing the client falls back to the
plain white `"white"` shader, so bots still get a white silhouette, just no
health tint.
