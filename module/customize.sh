SKIPUNZIP=0

[ "$ARCH" = "arm64" ] || abort "! Unsupported architecture: $ARCH (arm64 required)"

DEV="$(getprop ro.product.device)"
VDEV="$(getprop ro.product.vendor.device)"
if [ "$DEV" != "garnet" ] && [ "$VDEV" != "garnet" ]; then
  abort "! Unsupported device: $DEV (garnet required)"
fi

PATCHER="$MODPATH/bin/arm64/garnet_patcher"
[ -f "$PATCHER" ] || abort "! Missing patcher binary"
chmod 0755 "$PATCHER"

mkdir -p "$MODPATH/system/vendor/etc/camera"
mkdir -p "$MODPATH/system/vendor/lib64/hw"
mkdir -p "$MODPATH/apk/empty_oat"

ui_print "- Patching camera HAL..."

if [ -f /vendor/etc/camera/camxoverridesettingsOfUltra.txt ]; then
  "$PATCHER" patch-camx /vendor/etc/camera/camxoverridesettingsOfUltra.txt "$MODPATH/system/vendor/etc/camera/camxoverridesettingsOfUltra.txt" >/dev/null 2>&1 || abort "! Failed to patch CamX config"
else
  "$PATCHER" patch-camx - "$MODPATH/system/vendor/etc/camera/camxoverridesettingsOfUltra.txt" >/dev/null 2>&1 || abort "! Failed to create CamX config"
fi

patch_hal_file() {
  [ -f "$2" ] || return 0
  "$PATCHER" patch-hal "$1" "$2" "$3" >/dev/null 2>&1 || abort "! Failed to patch $1"
}

patch_hal_file camera_qcom      /vendor/lib64/hw/camera.qcom.so                 "$MODPATH/system/vendor/lib64/hw/camera.qcom.so"
patch_hal_file chi_override     /vendor/lib64/hw/com.qti.chi.override.so        "$MODPATH/system/vendor/lib64/hw/com.qti.chi.override.so"
patch_hal_file mcreprocrt       /vendor/lib64/com.qti.feature2.mcreprocrt.so    "$MODPATH/system/vendor/lib64/com.qti.feature2.mcreprocrt.so"
patch_hal_file feature2_generic /vendor/lib64/com.qti.feature2.generic.so       "$MODPATH/system/vendor/lib64/com.qti.feature2.generic.so"

CAM_APK="$(pm path com.android.camera 2>/dev/null | head -n 1 | sed 's/^package://')"
if [ -n "$CAM_APK" ] && [ -f "$CAM_APK" ]; then
  ui_print "- Patching MIUI/Leica Camera..."
  if "$PATCHER" patch-apk "$CAM_APK" "$MODPATH/apk/camera_patched.apk" >/dev/null 2>&1 && [ -f "$MODPATH/apk/camera_patched.apk" ]; then
    (stat -c '%s:%Y' "$CAM_APK" 2>/dev/null || ls -l "$CAM_APK") > "$MODPATH/apk/.src_stamp"
  else
    rm -f "$MODPATH/apk/camera_patched.apk"
  fi
else
  ui_print "- MIUI/Leica Camera not found (HAL-only mode)"
fi

ui_print "- Setting permissions..."
set_perm_recursive "$MODPATH" 0 0 0755 0644
set_perm "$MODPATH/bin/arm64/garnet_patcher" 0 0 0755
set_perm "$MODPATH/service.sh" 0 0 0755
set_perm "$MODPATH/uninstall.sh" 0 0 0755
set_perm_recursive "$MODPATH/system/vendor/etc" 0 0 0755 0644 u:object_r:vendor_configs_file:s0
set_perm_recursive "$MODPATH/system/vendor/lib64" 0 0 0755 0644 u:object_r:vendor_file:s0

if [ -f "$MODPATH/apk/camera_patched.apk" ]; then
  chmod 0644 "$MODPATH/apk/camera_patched.apk"
  case "$CAM_APK" in
    /data/*) chcon u:object_r:apk_data_file:s0 "$MODPATH/apk/camera_patched.apk" 2>/dev/null || true ;;
    *)       chcon u:object_r:system_file:s0   "$MODPATH/apk/camera_patched.apk" 2>/dev/null || true ;;
  esac
fi
chmod 0755 "$MODPATH/apk" "$MODPATH/apk/empty_oat" 2>/dev/null || true

ui_print "- Done! Reboot to apply."
