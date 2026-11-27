# Garnet Camera & Lens Switch Fix

Universal Magisk / KernelSU / APatch module to fix multi-camera support and native lens switching on **Poco X6 5G / Redmi Note 13 Pro 5G (`garnet`)**.

## Compatibility
- **Device:** `garnet` (Poco X6 5G / Redmi Note 13 Pro 5G)
- **ROM:** Universal (HyperOS, AOSP, ColorOS / OxygenOS Ports)
- **Camera App:** MIUI / Leica Camera (`com.android.camera`) — *not for stock ColorOS/Oplus Camera*
- **Root:** Magisk / KernelSU / APatch

## What's Fixed
- Native lens switching (`0.6x` ↔ `1x` ↔ `2x` / `3x` / `5x`) in Photo and Video mode with live zoom slider
- Smoother zoom transition when tapping zoom buttons
- UltraWide (`0.6x`), Macro, 64MP, and Pro mode
- Lower Noise Reduction & zero oversharpening in Photo + Video (`ANDROID_NOISE_REDUCTION_MODE=FAST`, `ANDROID_EDGE_MODE=OFF` in HAL defaults, `sharpness=0` while keeping Pro manual control, plus CamX `disableTFRefinement=TRUE`, `enableMCTF=FALSE`, `enableIPEAutoNoiseReduction=FALSE`) to remove heavy watercolor smearing while keeping chroma noise clean
- Active `0.6x` UltraWide distortion correction (`applyUltraWideLDC` / `applySATUltraWideLDC` + `enableLDC=TRUE`)
- Higher video bitrates for cleaner low-light detail (H.264 `1080p30`: 20 → 40 Mbps, `4K30`/`1080p60`: 50 → 75 Mbps; HEVC `1080p30`: 15.4 → 30.8 Mbps, `4K30`: 38.5 → 77 Mbps)
- 1080p 60fps on main/video zoom, auto fallback to 1080p 30fps on UltraWide, Front, and Macro
- Dynamic on-device patching (no bloated prebuilt APK/HAL binaries, auto-re-patches on camera app updates)

## Installation
1. Download `garnet-camera-fix-v1.2.zip` from [Releases](https://github.com/vael01id/garnet-switch-lens-module/releases/latest).
2. Flash in Magisk Manager / KernelSU / APatch.
3. Reboot.

## Uninstall
Remove the module from your root manager and reboot.

## Credits
- Vael ([@vael01id](https://github.com/vael01id))
