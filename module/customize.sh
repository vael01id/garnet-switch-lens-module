SKIPUNZIP=0

[ "$ARCH" = "arm64" ] || abort "! Unsupported architecture: $ARCH (arm64 required)"

DEV="$(getprop ro.product.device)"
VDEV="$(getprop ro.product.vendor.device)"
PDEV="$(getprop ro.build.product)"
if [ "$DEV" != "garnet" ] && [ "$VDEV" != "garnet" ] && [ "$PDEV" != "garnet" ]; then
  abort "! Unsupported device: $DEV (garnet required)"
fi

PATCHER="$MODPATH/bin/arm64/garnet_patcher"
[ -f "$PATCHER" ] || abort "! Missing patcher binary"
chmod 0755 "$PATCHER"

# Zero-overlay anti-bootloop design: NEVER create $MODPATH/system/ or $MODPATH/vendor/.
# All patched files live in $MODPATH/patched_vendor/ and $MODPATH/apk/ and are bind-mounted
# ONLY after sys.boot_completed=1 by service.sh.
rm -rf "$MODPATH/system" "$MODPATH/vendor" "$MODPATH/system.prop" 2>/dev/null || true
mkdir -p "$MODPATH/patched_vendor/etc/camera"
mkdir -p "$MODPATH/patched_vendor/lib64/hw"
mkdir -p "$MODPATH/apk/empty_oat"

LOG="$MODPATH/patch.log"
: > "$LOG"
echo "=== Garnet Camera Fix v1.5 install ($(date 2>/dev/null || true)) ===" >> "$LOG"
echo "ROM: $(getprop ro.build.fingerprint)" >> "$LOG"

file_stamp() {
  stat -c '%s:%Y' "$1" 2>/dev/null || ls -l "$1" 2>/dev/null
}

# Detect upgrade from legacy v1.0-v1.2 overlay module (which currently overlays /vendor).
# Defer vendor patching to first post-boot service.sh run when /vendor is 100% clean stock.
LEGACY_OVERLAY=0
if [ -d /data/adb/modules/garnet_oos16_uw_fix/system/vendor ]; then
  LEGACY_OVERLAY=1
fi
if grep -qs '/vendor/lib64/hw/camera.qcom.so' /proc/mounts 2>/dev/null; then
  LEGACY_OVERLAY=1
fi

if [ "$LEGACY_OVERLAY" = "1" ]; then
  ui_print "- Upgrading from previous version: vendor HAL will auto-patch cleanly after reboot"
  echo "[DEFER] Legacy overlay/bind-mount active; service.sh will patch from clean stock after reboot" >> "$LOG"
else
  ui_print "- Patching camera HAL into non-overlay staging..."

  CAMX_SRC=""
  CAMX_REL=""
  if [ -f /vendor/etc/camera/camxoverridesettingsOfUltra.txt ]; then
    CAMX_SRC=/vendor/etc/camera/camxoverridesettingsOfUltra.txt
    CAMX_REL=etc/camera/camxoverridesettingsOfUltra.txt
  elif [ -f /vendor/etc/camera/camxoverridesettings.txt ]; then
    CAMX_SRC=/vendor/etc/camera/camxoverridesettings.txt
    CAMX_REL=etc/camera/camxoverridesettings.txt
  fi
  if [ -n "$CAMX_SRC" ]; then
    CAMX_DST="$MODPATH/patched_vendor/$CAMX_REL"
    if "$PATCHER" patch-camx "$CAMX_SRC" "$CAMX_DST" >> "$LOG" 2>&1; then
      file_stamp "$CAMX_SRC" > "${CAMX_DST}.src_stamp"
      ui_print "  [OK] $(basename "$CAMX_SRC")"
    else
      rm -f "$CAMX_DST" "${CAMX_DST}.src_stamp"
      ui_print "  [SKIP] $(basename "$CAMX_SRC") (kept stock)"
    fi
  fi

  patch_hal_file() {
    local KIND="$1" SRC="$2" DST="$3" RC=0
    [ -f "$SRC" ] || return 0
    "$PATCHER" patch-hal "$KIND" "$SRC" "$DST" >> "$LOG" 2>&1
    RC=$?
    if [ "$RC" -eq 0 ] && [ -f "$DST" ]; then
      file_stamp "$SRC" > "${DST}.src_stamp"
      ui_print "  [OK] $KIND"
    else
      rm -f "$DST" "${DST}.src_stamp"
      ui_print "  [SKIP] $KIND (kept stock; see patch.log)"
    fi
  }

  patch_hal_file camera_qcom      /vendor/lib64/hw/camera.qcom.so                 "$MODPATH/patched_vendor/lib64/hw/camera.qcom.so"
  patch_hal_file chi_override     /vendor/lib64/hw/com.qti.chi.override.so        "$MODPATH/patched_vendor/lib64/hw/com.qti.chi.override.so"
  patch_hal_file mcreprocrt       /vendor/lib64/com.qti.feature2.mcreprocrt.so    "$MODPATH/patched_vendor/lib64/com.qti.feature2.mcreprocrt.so"
  patch_hal_file feature2_generic /vendor/lib64/com.qti.feature2.generic.so       "$MODPATH/patched_vendor/lib64/com.qti.feature2.generic.so"
fi

CAM_APK="$(pm path com.android.camera 2>/dev/null | head -n 1 | sed 's/^package://')"
if [ -n "$CAM_APK" ] && [ -f "$CAM_APK" ]; then
  if grep -qs " $CAM_APK " /proc/mounts 2>/dev/null; then
    ui_print "- Camera APK currently mounted: will auto-patch cleanly after reboot"
    echo "[DEFER] Camera APK currently bind-mounted; service.sh will patch after reboot" >> "$LOG"
  else
    ui_print "- Checking MIUI/Leica Camera APK..."
    if "$PATCHER" patch-apk "$CAM_APK" "$MODPATH/apk/camera_patched.apk" >> "$LOG" 2>&1 && [ -f "$MODPATH/apk/camera_patched.apk" ]; then
      file_stamp "$CAM_APK" > "$MODPATH/apk/.src_stamp"
      ui_print "  [OK] MIUI/Leica Camera APK patched"
    else
      rm -f "$MODPATH/apk/camera_patched.apk" "$MODPATH/apk/.src_stamp"
      ui_print "  [SKIP] Camera APK not matched (HAL-only mode; kept stock APK)"
    fi
  fi
else
  ui_print "- MIUI/Leica Camera not found (HAL-only mode)"
fi

ui_print "- Setting permissions..."
set_perm_recursive "$MODPATH" 0 0 0755 0644
set_perm "$MODPATH/bin/arm64/garnet_patcher" 0 0 0755
set_perm "$MODPATH/service.sh" 0 0 0755
set_perm "$MODPATH/uninstall.sh" 0 0 0755

ui_print "- Done! Zero early-boot /vendor overlay (anti-bootloop safe)."
ui_print "- Reboot to apply post-boot."
