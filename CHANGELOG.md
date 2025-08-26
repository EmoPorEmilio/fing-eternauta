# Changelog

All notable changes to this project will be documented in this file.

The format is based on Keep a Changelog and this project adheres to Semantic
Versioning.

## [Unreleased]

### Added

- (placeholder) Screenshot mode, presets, performance toggles.

## [1.4.0] - 2025-08-24

### Added

- Aesthetic controls: Rim Strength/Power and Exposure on Rendering tab.
- Expanded Rendering controls: glow, sparkle, threshold, noise, tint, fog.
- Impostor world-size clamps (min/max) and global size multiplier.

### Fixed

- Visibility polish at launch; ensure particles show reliably.

## [1.3.0] - 2025-08-23

### Added

- Culling & performance options: distance, screen-space culling, LOD thresholds,
  per-frame impostor draw cap, uniform batching toggle.

## [1.2.0] - 2025-08-22

### Added

- Debug HUD with counters (Active, BVH, Drawn, Off, Tiny, Cap) and footer
  overlay.
- “GUST ACTIVE” badge during blizzard gusts.

## [1.1.0] - 2025-08-21

### Added

- Static environment (floor/table); particles land, linger, respawn.
- Billboarded sphere impostors with disc masking for snow.
- Transparent impostor pass with correct depth/alpha handling.

## [1.0.0] - 2025-08-23

### Added

- SnowGlow material controls: Roughness, Metallic, Subsurface, Anisotropy, Base
  Alpha, Edge Fade, Normal Amp, Crack Scale, Crack Intensity.
- Blizzard gust system with interval, duration, fall and rotation multipliers.
- Pool-based cadence transitions with non-disruptive activation/deactivation.
- Rendering tab: SnowGlow extras and material controls.
- HISTORY.md documenting milestone timeline.

### Changed

- Default cadence: Cycle with 5s period.
- Default shader: SnowGlow.
- Shader: added transparency, subsurface, anisotropic specular, micro-sparkle,
  tone mapping and fog.
- UI: tabs, scrolling, mouse navigation; alpha blending during draw.

### Fixed

- UI stray `else` compile error.
- Avoid visible object pops during cadence changes.

## [0.9.1] - 2025-08-20

### Fixed

- UI compile stability (removed stray else), minor cleanup.

## [0.9.0] - 2025-08-19

### Added

- Smooth transitions between cadence targets using a fixed object pool.

## [0.8.0] - 2025-08-18

### Added

- Blizzard gust effect; jittered boosts to fall/rotation.

## [0.7.0] - 2025-08-17

### Added

- SnowGlow shader (sparkle + glow); Cycle mode default 5s.

## [0.6.0] - 2025-08-16

### Added

- Cadence presets (1/2/3/Cycle) controlling object count, fall and rotation.

## [0.5.0] - 2025-08-15

### Added

- Scrolling lists and scrollbar.

## [0.4.0] - 2025-08-14

### Added

- Tabbed UI: Rendering, Camera, Scene, Lighting.

## [0.3.0] - 2025-08-13

### Added

- Mouse and button navigation; pagination; core options.

## [0.2.0] - 2025-08-12

### Added

- In-game overlay (Enter), Settings system and UI.

## [0.1.0] - 2025-08-11

### Added

- SDL2 + OpenGL baseline with camera and basic scene.

[Unreleased]: https://example.com/compare/v1.0.0...HEAD
[1.0.0]: https://example.com/compare/v0.9.1...v1.0.0
[0.9.1]: https://example.com/compare/v0.9.0...v0.9.1
[0.9.0]: https://example.com/compare/v0.8.0...v0.9.0
[0.8.0]: https://example.com/compare/v0.7.0...v0.8.0
[0.7.0]: https://example.com/compare/v0.6.0...v0.7.0
[0.6.0]: https://example.com/compare/v0.5.0...v0.6.0
[0.5.0]: https://example.com/compare/v0.4.0...v0.5.0
[0.4.0]: https://example.com/compare/v0.3.0...v0.4.0
[0.3.0]: https://example.com/compare/v0.2.0...v0.3.0
[0.2.0]: https://example.com/compare/v0.1.0...v0.2.0
[0.1.0]: https://example.com/releases/v0.1.0
