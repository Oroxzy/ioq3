# HitsoundLab icon

The reticle represents the bot aiming and shot-analysis tools; the five-bar
waveform represents hit feedback, pitch settings and sound-event measurement.
The warm illuminated metal treatment follows the Quake arena setting.

- `hitsound-lab.png`: original generated artwork, with transparent corners.
- `hitsound-lab.ico`: 32-bit Windows icon at 16, 20, 24, 32, 40, 48, 64, 96,
  128 and 256 pixels. Embedded in both the executable and the WinForms assembly.
- `Build-Icon.ps1`: deterministic conversion using Windows System.Drawing.
  Run `powershell -File tools/hitsound-lab/assets/Build-Icon.ps1` from the repo root
  after changing the source PNG. An ordinary app build uses the checked-in ICO.

The former `../app.ico` is retained. No game behavior was changed for the icon.

## Generation

Created with the built-in Imagegen tool (not the CLI). Final generation prompt:

```text
Use case: stylized-concept.
Asset type: one finished Windows desktop application icon, square 1024 x 1024, genuinely transparent outside its silhouette.
Primary request: Create a striking premium app icon for HitsoundLab / Trefferton-Labor, a Quake 3 desktop test bench. The source code configures hit feedback sounds and their pitch based on the target's health, assists aiming at bots, and measures shots, hits and missed sound events.
Subject: A single powerful unified emblem fusing an aiming reticle and an audio waveform. A thick broken circular reticle with four short cardinal ticks encloses a compact five-bar sound waveform, with one distinct tall central bar. Think precise hit confirmation made audible. The glyph should have large clean shapes and generous gaps so it survives at 16 and 32 pixels.
Style: refined game-tool icon, mostly crisp graphic geometry with restrained shallow metallic bevels and soft internal illumination. Memorable strong silhouette, polished like a crafted desktop app icon. Charcoal gunmetal rounded-square tile, softly rounded corners, warm bright ember-orange reticle, near-white warm waveform. Reflect Quake's industrial arena atmosphere through material and warm light, not weapons or characters.
Composition: precisely centered, perfectly front-facing, no perspective tilt. Tile nearly fills the square with about 4% transparent outer margin. Emblem occupies about 72% of the tile. A faint bevel on the tile edge, a restrained warm rim light, nearly black clean surface. Keep the symbol brightly readable. Only the tile's corners and outside margin are transparent.
Constraints: single icon only, no contact sheet, no desktop mockup, no text, no letters, no numbers, no watermark, no official Quake logo, no extra badges, no headphones, no tiny interface decorations, no lens flare, no smoke, no particles, no busy grunge. Do not show a checkerboard; actual alpha transparency. Ready to use as the final application icon.
```
