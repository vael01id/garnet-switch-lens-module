#!/system/bin/sh
MODDIR=${0%/*}
PATCHER="$MODDIR/bin/arm64/garnet_patcher"
LOG="$MODDIR/patch.log"

export PATH="/data/adb/ksu/bin:/data/adb/ap/bin:/data/adb/magisk:/product/bin:/system/bin:/vendor/bin:$PATH"

# Wait for full Android boot before touching any camera props, HAL files, or APK.
# Guarantees zero risk of early-boot bootloop across all ROMs & root managers.
while [ "$(getprop sys.boot_completed)" != "1" ]; do
  sleep 2
done
sleep 3

echo "=== Garnet Camera Fix v1.5 post-boot ($(date 2>/dev/null || true)) ===" >> "$LOG"
chmod 0755 "$PATCHER" 2>/dev/null || true

RESETPROP="$(command -v resetprop 2>/dev/null || true)"
if [ -z "$RESETPROP" ] && [ -x /data/adb/magisk/magisk ]; then
  RESETPROP="/data/adb/magisk/magisk resetprop"
fi

file_stamp() {
  stat -c '%s:%Y' "$1" 2>/dev/null || ls -l "$1" 2>/dev/null
}

get_ctx() {
  ls -Z "$1" 2>/dev/null | awk '{for(i=1;i<=NF;i++) if($i ~ /^u:object_r:/) {print $i; exit}}'
}

is_mounted() {
  grep -qs " $1 " /proc/mounts 2>/dev/null
}

NEED_RESTART=0

# 1. Dynamic camera privapp & aux props (preserve ROM entries, enforce <= 91 chars PROP_VALUE_MAX)
ensure_cam_prop() {
  local PROP="$1" FALLBACK="$2" CUR NEW
  CUR="$(getprop "$PROP" 2>/dev/null)"
  case ",$CUR," in
    *,com.android.camera,*) return 0 ;;
  esac
  if [ -z "$CUR" ]; then
    NEW="$FALLBACK"
  else
    NEW="${CUR},com.android.camera"
    if [ "${#NEW}" -gt 91 ]; then
      NEW="$FALLBACK"
    fi
  fi
  if [ -n "$RESETPROP" ]; then
    $RESETPROP -n "$PROP" "$NEW" 2>/dev/null || $RESETPROP "$PROP" "$NEW" 2>/dev/null || setprop "$PROP" "$NEW" 2>/dev/null || true
  else
    setprop "$PROP" "$NEW" 2>/dev/null || true
  fi
  echo "[PROP] $PROP=$NEW" >> "$LOG"
  NEED_RESTART=1
}

if [ -d /my_product ] || [ -n "$(getprop ro.oplus.version.my_product 2>/dev/null)" ]; then
  PRIV_FALLBACK="com.android.camera,com.oplus.camera,com.oplus.engineercamera"
  AUX_FALLBACK="android,com.android.camera,com.oplus.camera,com.oplus.engineercamera"
else
  PRIV_FALLBACK="com.android.camera"
  AUX_FALLBACK="android,com.android.camera"
fi

ensure_cam_prop persist.vendor.camera.privapp.list "$PRIV_FALLBACK"
ensure_cam_prop persist.camera.privapp.list        "$PRIV_FALLBACK"
ensure_cam_prop vendor.camera.aux.packagelist      "$AUX_FALLBACK"

# 2. Ensure vendor files are cleanly patched from current stock & bind-mount post-boot
mkdir -p "$MODDIR/patched_vendor/etc/camera" "$MODDIR/patched_vendor/lib64/hw" 2>/dev/null || true

apply_vendor_file() {
  local MODE="$1" KIND="$2" REL="$3"
  local DST="/vendor/$REL"
  local SRC="$MODDIR/patched_vendor/$REL"
  local STAMP="${SRC}.src_stamp"
  local CUR_STAMP OLD_STAMP CTX RC

  [ -f "$DST" ] || return 0
  if is_mounted "$DST"; then
    return 0
  fi

  CUR_STAMP="$(file_stamp "$DST")"
  OLD_STAMP="$(cat "$STAMP" 2>/dev/null || true)"

  if [ ! -f "$SRC" ] || [ "$CUR_STAMP" != "$OLD_STAMP" ]; then
    if [ "$MODE" = "camx" ]; then
      "$PATCHER" patch-camx "$DST" "${SRC}.tmp" >> "$LOG" 2>&1
    else
      "$PATCHER" patch-hal "$KIND" "$DST" "${SRC}.tmp" >> "$LOG" 2>&1
    fi
    RC=$?
    if [ "$RC" -eq 0 ] && [ -f "${SRC}.tmp" ]; then
      mv -f "${SRC}.tmp" "$SRC"
      printf '%s\n' "$CUR_STAMP" > "$STAMP"
    else
      rm -f "${SRC}.tmp" "$SRC" "$STAMP"
      return 0
    fi
  fi

  if [ -f "$SRC" ] && ! cmp -s "$SRC" "$DST"; then
    chmod 0644 "$SRC" 2>/dev/null || true
    chown 0:0 "$SRC" 2>/dev/null || true
    CTX="$(get_ctx "$DST")"
    [ -n "$CTX" ] && chcon "$CTX" "$SRC" 2>/dev/null || true
    if mount -o bind "$SRC" "$DST" 2>/dev/null; then
      echo "[MOUNT] $DST (ctx=${CTX:-default})" >> "$LOG"
      NEED_RESTART=1
    else
      echo "[ERR] bind-mount failed for $DST" >> "$LOG"
    fi
  fi
}

if [ -f /vendor/etc/camera/camxoverridesettingsOfUltra.txt ]; then
  apply_vendor_file camx camx etc/camera/camxoverridesettingsOfUltra.txt
elif [ -f /vendor/etc/camera/camxoverridesettings.txt ]; then
  apply_vendor_file camx camx etc/camera/camxoverridesettings.txt
fi
apply_vendor_file hal camera_qcom      lib64/hw/camera.qcom.so
apply_vendor_file hal chi_override     lib64/hw/com.qti.chi.override.so
apply_vendor_file hal mcreprocrt       lib64/com.qti.feature2.mcreprocrt.so
apply_vendor_file hal feature2_generic lib64/com.qti.feature2.generic.so

if [ "$NEED_RESTART" = "1" ]; then
  echo "[RESTART] Restarting camera provider & cameraserver..." >> "$LOG"
  killall -9 \
    vendor.qti.camera.provider@2.7-service_64 \
    vendor.qti.camera.provider-service_64 \
    android.hardware.camera.provider@2.4-service_64 \
    cameraserver 2>/dev/null || true
  sleep 4
fi

# 3. Universal post-boot MIUI / Leica Camera APK patch & safe bind-mount
TRIES=0
while ! pm path android >/dev/null 2>&1 && [ "$TRIES" -lt 10 ]; do
  sleep 1
  TRIES=$((TRIES + 1))
done

CAM_APK="$(pm path com.android.camera 2>/dev/null | head -n 1 | sed 's/^package://')"
if [ -n "$CAM_APK" ] && [ -f "$CAM_APK" ] && ! is_mounted "$CAM_APK"; then
  CAM_DIR="$(dirname "$CAM_APK")"
  mkdir -p "$MODDIR/apk/empty_oat"
  CUR_STAMP="$(file_stamp "$CAM_APK")"
  OLD_STAMP="$(cat "$MODDIR/apk/.src_stamp" 2>/dev/null || true)"

  if [ ! -f "$MODDIR/apk/camera_patched.apk" ] || [ "$CUR_STAMP" != "$OLD_STAMP" ]; then
    if "$PATCHER" patch-apk "$CAM_APK" "$MODDIR/apk/camera_patched.apk.tmp" >> "$LOG" 2>&1 && [ -f "$MODDIR/apk/camera_patched.apk.tmp" ]; then
      mv -f "$MODDIR/apk/camera_patched.apk.tmp" "$MODDIR/apk/camera_patched.apk"
      printf '%s\n' "$CUR_STAMP" > "$MODDIR/apk/.src_stamp"
    else
      rm -f "$MODDIR/apk/camera_patched.apk.tmp" "$MODDIR/apk/camera_patched.apk" "$MODDIR/apk/.src_stamp"
    fi
  fi

  if [ -f "$MODDIR/apk/camera_patched.apk" ]; then
    chmod 0644 "$MODDIR/apk/camera_patched.apk"
    chmod 0755 "$MODDIR/apk/empty_oat"
    APK_CTX="$(get_ctx "$CAM_APK")"
    if [ -n "$APK_CTX" ]; then
      chcon "$APK_CTX" "$MODDIR/apk/camera_patched.apk" 2>/dev/null || true
    else
      case "$CAM_APK" in
        /data/*) chcon u:object_r:apk_data_file:s0 "$MODDIR/apk/camera_patched.apk" 2>/dev/null || true ;;
        *)       chcon u:object_r:system_file:s0   "$MODDIR/apk/camera_patched.apk" 2>/dev/null || true ;;
      esac
    fi
    if [ -d "$CAM_DIR/oat/arm64" ]; then
      OAT_CTX="$(get_ctx "$CAM_DIR/oat/arm64")"
      [ -n "$OAT_CTX" ] && chcon -R "$OAT_CTX" "$MODDIR/apk/empty_oat" 2>/dev/null || true
      umount -l "$CAM_DIR/oat/arm64" 2>/dev/null || true
      mount -o bind "$MODDIR/apk/empty_oat" "$CAM_DIR/oat/arm64" 2>/dev/null || true
    fi
    rm -f /data/dalvik-cache/arm64/*MiuiCamera* /data/dalvik-cache/arm64/*com.android.camera* 2>/dev/null || true
    rm -rf /data/misc/profiles/cur/0/com.android.camera /data/misc/profiles/ref/com.android.camera 2>/dev/null || true
    if mount -o bind "$MODDIR/apk/camera_patched.apk" "$CAM_APK" 2>/dev/null; then
      echo "[MOUNT] $CAM_APK (ctx=${APK_CTX:-default})" >> "$LOG"
      am force-stop com.android.camera 2>/dev/null || true
    else
      echo "[ERR] bind-mount failed for $CAM_APK" >> "$LOG"
    fi
  fi
fi

# Trim patch.log if it grows beyond ~256 lines
if [ -f "$LOG" ] && [ "$(wc -l < "$LOG" 2>/dev/null || echo 0)" -gt 256 ]; then
  tail -n 200 "$LOG" > "${LOG}.tmp" && mv -f "${LOG}.tmp" "$LOG"
fi
