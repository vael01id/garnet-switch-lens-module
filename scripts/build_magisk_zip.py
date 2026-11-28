#!/usr/bin/env python3
"""
Packages the lightweight zero-overlay anti-bootloop Magisk/KSU/APatch ZIP from module/.
Enforces LF line endings on scripts/props, clean Unix permissions, and rejects any
early-boot /vendor or /system overlay files.
"""
import argparse
import os
import zipfile

TEXT_EXTS = (".sh", ".prop", ".rule", ".txt", ".xml")
EXEC_NAMES = ("update-binary", "garnet_patcher")


def main():
    ap = argparse.ArgumentParser(description="Package Garnet Camera Fix lightweight Magisk ZIP")
    ap.add_argument("--module-dir", default="module", help="Module directory")
    ap.add_argument("--out", required=True, help="Output ZIP path")
    args = ap.parse_args()

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with zipfile.ZipFile(args.out, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for root, dirs, files in os.walk(args.module_dir):
            dirs.sort()
            files.sort()
            for fn in files:
                full = os.path.join(root, fn)
                rel = os.path.relpath(full, args.module_dir).replace("\\", "/")
                # Safety guard: zero early-boot overlay files or static system.prop allowed
                if rel.startswith(("system/", "vendor/")) or rel == "system.prop":
                    raise RuntimeError(f"Refusing to package early-boot overlay/prop file: {rel}")
                data = open(full, "rb").read()
                if rel.endswith(TEXT_EXTS) or fn in ("update-binary", "updater-script"):
                    data = data.replace(b"\r\n", b"\n")
                zi = zipfile.ZipInfo(rel)
                zi.compress_type = zipfile.ZIP_STORED if fn.endswith(".gz") else zipfile.ZIP_DEFLATED
                is_exec = rel.endswith(".sh") or fn in EXEC_NAMES or rel.startswith("bin/")
                zi.external_attr = (0o755 if is_exec else 0o644) << 16
                zf.writestr(zi, data)

    sz = os.path.getsize(args.out)
    print(f"[OK] Built {args.out} ({sz:,d} bytes / {sz/1024:.1f} KB)")


if __name__ == "__main__":
    main()
