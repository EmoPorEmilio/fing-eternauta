# Project History

This repo’s evolution reconstructed as readable milestones. Tags correspond to
snapshots described below.

## v0.1 — Baseline

- SDL2 + OpenGL bootstrap
- Basic pyramid scene, camera, input loop

## v0.2 — In‑game overlay

- Toggleable UI (Enter)
- `Settings.h` and `UI.cpp` introduced
- Labels and basic boxes

## v0.3 — Mouse + buttons; pagination

- Mouse navigation and header buttons
- Core options (camera, FOV, background, light)

## v0.4 — Tabs

- Tabbed UI: Rendering, Camera, Scene, Lighting

## v0.5 — Scrolling

- Wheel scrolling and scrollbar for long lists

## v0.6 — Cadence presets

- Three snow cadences (1/2/3) and Cycle mode
- Falling + rotation speeds from playlist

## v0.7 — SnowGlow shader (first pass)

- Animated sparkle/bloom
- Time uniform and defaults (Cycle=5s)

## v0.8 — Blizzard gusts

- Periodic gusts boost fall/rotation with jitter
- UI exposure of gust settings

## v0.9 — Smooth transitions

- Pool-based objects; active count eases to target
- No hard scene reset during cadence change

## v0.9.1 — UI stability

- Fixed stray else; minor cleanup

## v1.0 — Material upgrade + refined transitions

- SnowGlow upgraded: transparency, subsurface, anisotropy, edge fade, crack/bump
  normals, fog, tone mapping
- Alpha blending enabled during draw
- Rendering tab gains full material controls
- Non‑disruptive cadence transitions finalized (activate from top, retire on
  natural exit)

## v1.1 — Static environment + impostors

- Added static floor and table meshes; particles land, linger, then respawn
- Switched particles to billboarded sphere impostors with disc masking
- Transparent impostor pass uses depth writes disabled for correct blending
- Initial visibility tuning to ensure particles show on launch

## v1.2 — Debug HUD + metrics

- Compact on‑screen counters (Active, BVH, Drawn, Off, Tiny, Cap)
- Footer overlay in UI with the same metrics
- “GUST ACTIVE” badge during blizzard gusts

## v1.3 — Culling, LOD, and performance controls

- Distance and screen‑space culling toggles in Debug tab
- Screen‑space LOD thresholds; per‑frame impostor draw cap
- Uniform batching toggle

## v1.4 — Aesthetic controls and visibility polish

- New SnowGlow controls: Rim Strength/Power and Exposure
- Expanded Rendering tab controls: glow, sparkle, threshold, noise, tint, fog
- Impostor world‑size clamps (min/max) and size multiplier for consistent
  visibility
