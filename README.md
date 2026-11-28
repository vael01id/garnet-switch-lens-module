# Garnet Camera & Lens Switch Fix

Universal, **zero-overlay anti-bootloop** Magisk / KernelSU / APatch module to fix multi-camera support, native lens switching, and image quality on **Poco X6 5G / Redmi Note 13 Pro 5G (`garnet`)**.

## Compatibility
- **Device:** `garnet` (Poco X6 5G / Redmi Note 13 Pro 5G)
- **ROM:** Universal — **OxygenOS 16 / ColorOS Ports, Stock HyperOS (CN/Global/ID/EU), and AOSP**
- **Camera App:** MIUI / Leica Camera (`com.android.camera`) — *auto-falls back to HAL-only mode if APK version differs*
- **Root:** Magisk / KernelSU / APatch

## Anti-Bootloop & Universal Vendor/APK Architecture (`v1.3`–`v1.5`)
- **Zero early-boot `/vendor` or `/system` overlay:** no `$MODPATH/system/`, no `$MODPATH/vendor/`, and no static `system.prop` — impossible to cause stuck boot logo on any ROM or root manager.
- **Post-boot bind-mounts only (`service.sh` after `sys.boot_completed=1`):** preserves each ROM's exact stock SELinux contexts (`ls -Z` → `chcon`), dynamically appends `com.android.camera` via `resetprop -n` (respecting the 91-char `PROP_VALUE_MAX`), and safely restarts camera HAL once.
- **Universal dynamic ARM64 `chi_override` resolver (`v1.4` / `v1.5`):** dynamically resolves shifted `BL`, `ADRP+ADD` (including 4KB page-crossing & hoisted `VTCamera`), `MultiCamera` logical config tables, and Wide↔UW transition pointer arrays across **HyperOS 1/2/3/4, OOS16 ports, and Lunaris / AOSP** vendor HALs (atomic 15/15 all-or-nothing patching).
- **ProGuard-independent dynamic MiuiCamera APK patcher (`v1.5`):** supports both OOS16/OS3 ported MiuiCam (full 39-edit profile) and stock **HyperOS 4** `MiuiCamera.apk` via dynamic string/API anchors (SAT `roleId 60→62`, `isSupportSAT`/`isSupportVideoSAT`, maxZoom/spline, anti-oversharpening, 0.6x UW LDC, H.264/HEVC bitrate boost) while preserving native Xiaomi `AlgoUp` & `LocalParallel`.
- **Strict ELF64/AArch64 & pattern verification (`garnet_patcher`):** validates ELF headers, RX `PT_LOAD` segment bounds, ARM64 instruction alignment, ADRP+ADD `.rodata` targets, atomic `chi_override` matching, and APK anchor thresholds. Incompatible files are skipped cleanly (stock untouched) and logged to `/data/adb/modules/garnet_oos16_uw_fix/patch.log`.
- **Auto-re-patch on ROM/APK updates:** detects changed stock vendor or APK stamps and re-patches cleanly after updates.

## What's Fixed
- Native lens switching (`0.6x` ↔ `1x` ↔ `2x` / `3x` / `5x`) in Photo and Video mode with live zoom slider
- Smoother zoom transition when tapping zoom buttons
- UltraWide (`0.6x`), Macro, 64MP, and Pro mode
- Surgical Luma-only Noise Reduction bypass + zero oversharpening in Photo & Video (`ANDROID_EDGE_MODE=OFF`, `sharpness=0` while keeping Pro manual control, Titan570 `TF201` Luma temporal blend = `0`, and `ANR101` Luma spatial `YLPF` / `DCBlend2` bypassed) while keeping **100% hardware Chroma NR (`TF Chroma` + `ANR CLPF` + `CNR`) active** — removes watercolor smearing without rainbow color noise
- Active `0.6x` UltraWide distortion correction (`applyUltraWideLDC` / `applySATUltraWideLDC` + `enableLDC=TRUE`)
- Higher video bitrates for cleaner low-light detail (H.264 `1080p30`: 20 → 40 Mbps, `4K30`/`1080p60`: 50 → 75 Mbps; HEVC `1080p30`: 15.4 → 30.8 Mbps, `4K30`: 38.5 → 77 Mbps)
- 1080p 60fps on main/video zoom, auto fallback to 1080p 30fps on UltraWide, Front, and Macro

## Installation
1. Download `garnet-camera-fix-v1.5.zip` from [Releases](https://github.com/vael01id/garnet-switch-lens-module/releases/latest).
2. Flash in Magisk Manager / KernelSU / APatch.
3. Reboot (first boot after upgrading from `v1.2` automatically re-patches clean stock vendor post-boot; inspect `/data/adb/modules/garnet_oos16_uw_fix/patch.log` for details).

## Uninstall
Remove the module from your root manager and reboot.

## Credits
- Vael ([@vael01id](https://github.com/vael01id))
