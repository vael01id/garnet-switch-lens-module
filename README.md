# Garnet Camera & Lens Switch Fix

Universal, **zero-overlay anti-bootloop** Magisk / KernelSU / APatch module to fix multi-camera support, native lens switching, zoom smoothness, and image quality on **Poco X6 5G / Redmi Note 13 Pro 5G (`garnet`)**.

## Compatibility
- **Device:** `garnet` (Poco X6 5G / Redmi Note 13 Pro 5G)
- **ROM:** Universal — **OxygenOS 16 / ColorOS Ports, Stock HyperOS (CN/Global/ID/EU/HyperOS 4), and AOSP**
- **Camera App:** MIUI / Leica Camera (`com.android.camera`) — *auto-falls back to HAL-only mode if APK version differs*
- **Root:** Magisk / KernelSU / APatch (no LSPosed / Vector required)

## Architecture (`v1.3`–`v1.6`)
- **Zero early-boot `/vendor` or `/system` overlay:** no `$MODPATH/system/`, no `$MODPATH/vendor/`, and no static `system.prop` — impossible to cause stuck boot logo on any ROM or root manager.
- **Post-boot bind-mounts only (`service.sh` after `sys.boot_completed=1`):** preserves each ROM's exact stock SELinux contexts (`ls -Z` → `chcon`), dynamically appends `com.android.camera` via `resetprop -n` (respecting the 91-char `PROP_VALUE_MAX`), and safely restarts camera HAL once.
- **Built-in Klee-style `ZoomManager` smoother (`v1.6`, `classes8.dex`):** hooks `Lt5/f;.a0(FI)Z` directly inside `com.android.camera.apk` using a strict 1× per-VSync `ValueAnimator` ticker (`tau = 115ms`), neutralizing `ZoomRatioToggleView` `AnimatorSet` (`400ms → 0ms`) and pairing with a `500ms` `PathInterpolator(0, 0.5625, 0, 1)` + `LinearOutSlowInInterpolator` slider curve — zero LSPosed/Vector needed.
- **Universal dynamic ARM64 `chi_override` resolver (`v1.4` / `v1.5`):** dynamically resolves shifted `BL`, `ADRP+ADD` (including 4KB page-crossing & hoisted `VTCamera`), `MultiCamera` logical config tables, and Wide↔UW transition pointer arrays across **HyperOS 1/2/3/4, OOS16 ports, and Lunaris / AOSP** vendor HALs (atomic 15/15 all-or-nothing patching).
- **ProGuard-independent dynamic MiuiCamera APK patcher (`v1.5` / `v1.6`):** supports both OOS16/OS3 ported MiuiCam (full 41-edit + `classes8.dex` profile) and stock **HyperOS 4** `MiuiCamera.apk` via dynamic string/API anchors (SAT `roleId 60→62`, `isSupportSAT`/`isSupportVideoSAT`, maxZoom/spline, anti-oversharpening, `0.6x` UW LDC, video bitrates) while preserving native Xiaomi `AlgoUp` & `LocalParallel`.
- **Strict ELF64/AArch64 & pattern verification (`garnet_patcher`):** validates ELF headers, RX `PT_LOAD` segment bounds, ARM64 instruction alignment, ADRP+ADD `.rodata` targets, atomic `chi_override` matching, and APK anchor thresholds. Incompatible files are skipped cleanly (stock untouched) and logged to `/data/adb/modules/garnet_oos16_uw_fix/patch.log`.
- **Auto-re-patch on ROM/APK updates:** detects changed stock vendor or APK stamps and re-patches cleanly after updates.

## What's Fixed in `v1.6`
- **Native lens switching & quick zoom buttons:** updated **`[0.6x, 1x, 3x, 5x, 10x]`** buttons in Photo (`163`) and Video (`162`) modes with live bidirectional slider (`0.6x ↔ 1x ↔ 10x`).
- **Klee-style smooth zoom curve:** built-in `ZoomManager` VSync smoother (`500ms` UI + `tau = 115ms`) with asymmetric response — linear exponential ease-out on Zoom IN (`0.6x → 10x`) and gentler log-zoom (`× 0.703`) on Zoom OUT (`10x / 1x → 0.6x`).
- **UltraWide (`0.6x`), Macro, 64MP (`OV64B40`), and Pro mode:** fully working including Pro mode display string crash fix.
- **Surgical Luma-only NR bypass + TF20 Chroma refinement (`v1.2`–`v1.6`):** disables oversharpening (`ANDROID_EDGE_MODE=OFF`, `sharpness=0` in Photo & Video while keeping Pro manual control), bypasses Titan570 `TF201` Luma temporal blend and `ANR101` Luma spatial `YLPF` / `DCBlend2`, while keeping **100% hardware Chroma NR + `TF20` Chroma refinement (`disableTFRefinement=FALSE`) active** — removes watercolor smearing without rainbow color noise.
- **Active `0.6x` UltraWide distortion correction:** forces `applyUltraWideLDC` / `applySATUltraWideLDC` + `enableLDC=TRUE`.
- **Thermally balanced video bitrates (`v1.6`):** tuned for high detail without overloading Snapdragon 7s Gen 2's Venus encoder during long recordings:
  - **H.264:** `1080p30`: `20 → 40 Mbps`, `4K30`/`1080p60`: `50 → 60 Mbps`
  - **HEVC:** `1080p30`: `15.4 → 20.5 Mbps`, `4K30`/`1080p60`: `38.5 → 51.3 Mbps`
- **60fps support:** `1080p60` enabled on Wide / VideoSAT, with automatic safe fallback to `1080p30` + EIS on UltraWide, Front, and Macro.

## Installation
1. Download `garnet-camera-fix-v1.6-klee-smooth.zip` from [Releases](https://github.com/vael01id/garnet-switch-lens-module/releases/latest) (or `dist/garnet-camera-fix-v1.6-klee-smooth.zip`).
2. If you previously used **Klee Zoom Mod** (LSPosed / Vector), disable or remove `com.android.camera` from its scope to avoid double-hooking `ZoomManager`.
3. Flash in Magisk Manager / KernelSU / APatch and reboot. Inspect `/data/adb/modules/garnet_oos16_uw_fix/patch.log` after boot if needed.

## Uninstall
Remove the module from your root manager and reboot.

## Credits
- Vael ([@vael01id](https://github.com/vael01id))
