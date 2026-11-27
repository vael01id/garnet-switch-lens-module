#!/system/bin/sh
MODDIR=${0%/*}
PATCHER="$MODDIR/bin/arm64/garnet_patcher"

while [ "$(getprop sys.boot_completed)" != "1" ]; do
  sleep 2
done
sleep 3

# 1. Fallback vendor bind-mounts if root manager did not overlay /vendor
NEED_RESTART=0
for REL in \
  etc/camera/camxoverridesettingsOfUltra.txt \
  lib64/hw/camera.qcom.so \
  lib64/hw/com.qti.chi.override.so \
  lib64/com.qti.feature2.mcreprocrt.so \
  lib64/com.qti.feature2.generic.so; do
  SRC="$MODDIR/system/vendor/$REL"
  DST="/vendor/$REL"
  if [ -f "$SRC" ] && [ -f "$DST" ]; then
    if ! cmp -s "$SRC" "$DST"; then
      mount -o bind "$SRC" "$DST" 2>/dev/null && NEED_RESTART=1
    fi
  fi
done

if [ "$NEED_RESTART" = "1" ]; then
  killall -9 vendor.qti.camera.provider@2.7-service_64 vendor.qti.camera.provider-service_64 cameraserver 2>/dev/null || true
  sleep 4
fi

# 2. Universal post-boot MIUI / Leica Camera APK patch & safe bind-mount
CAM_APK="$(pm path com.android.camera 2>/dev/null | head -n 1 | sed 's/^package://')"
if [ -n "$CAM_APK" ] && [ -f "$CAM_APK" ]; then
  CAM_DIR="$(dirname "$CAM_APK")"
  mkdir -p "$MODDIR/apk/empty_oat"
  CUR_STAMP="$(stat -c '%s:%Y' "$CAM_APK" 2>/dev/null || ls -l "$CAM_APK")"
  OLD_STAMP="$(cat "$MODDIR/apk/.src_stamp" 2>/dev/null || true)"

  if [ ! -f "$MODDIR/apk/camera_patched.apk" ] || [ "$CUR_STAMP" != "$OLD_STAMP" ]; then
    chmod 0755 "$PATCHER" 2>/dev/null || true
    if "$PATCHER" patch-apk "$CAM_APK" "$MODDIR/apk/camera_patched.apk.tmp" >/dev/null 2>&1; then
      mv -f "$MODDIR/apk/camera_patched.apk.tmp" "$MODDIR/apk/camera_patched.apk"
      printf '%s\n' "$CUR_STAMP" > "$MODDIR/apk/.src_stamp"
    else
      rm -f "$MODDIR/apk/camera_patched.apk.tmp"
    fi
  fi

  if [ -f "$MODDIR/apk/camera_patched.apk" ]; then
    chmod 0644 "$MODDIR/apk/camera_patched.apk"
    chmod 0755 "$MODDIR/apk/empty_oat"
    case "$CAM_APK" in
      /data/*)
        chcon u:object_r:apk_data_file:s0 "$MODDIR/apk/camera_patched.apk" 2>/dev/null || true
        chcon -R u:object_r:dalvikcache_data_file:s0 "$MODDIR/apk/empty_oat" 2>/dev/null || true
        ;;
      *)
        chcon u:object_r:system_file:s0 "$MODDIR/apk/camera_patched.apk" 2>/dev/null || true
        chcon -R u:object_r:system_file:s0 "$MODDIR/apk/empty_oat" 2>/dev/null || true
        ;;
    esac
    if [ -d "$CAM_DIR/oat/arm64" ]; then
      umount -l "$CAM_DIR/oat/arm64" 2>/dev/null || true
      mount -o bind "$MODDIR/apk/empty_oat" "$CAM_DIR/oat/arm64" 2>/dev/null || true
    fi
    rm -f /data/dalvik-cache/arm64/*MiuiCamera* /data/dalvik-cache/arm64/*com.android.camera* 2>/dev/null || true
    rm -rf /data/misc/profiles/cur/0/com.android.camera /data/misc/profiles/ref/com.android.camera 2>/dev/null || true
    umount -l "$CAM_APK" 2>/dev/null || true
    mount -o bind "$MODDIR/apk/camera_patched.apk" "$CAM_APK" 2>/dev/null || true
    am force-stop com.android.camera 2>/dev/null || true
  fi
fi
