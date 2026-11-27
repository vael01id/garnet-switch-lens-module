#!/usr/bin/env python3
"""
Packages the lightweight on-device patching Magisk module (~2.5 MB) directly from module/.
"""
import argparse
import os
import zipfile


def main():
    ap = argparse.ArgumentParser(description="Package Garnet Camera Fix lightweight Magisk ZIP")
    ap.add_argument("--module-dir", default="module", help="Module directory containing bin/, deltas/, and scripts")
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
                # Deltas are already level-9 gzipped; store them directly without double-compressing
                comp = zipfile.ZIP_STORED if fn.endswith(".gz") else zipfile.ZIP_DEFLATED
                zf.write(full, rel, compress_type=comp)

    print(f"[OK] Built {args.out} ({os.path.getsize(args.out):,d} bytes / {os.path.getsize(args.out)/1024/1024:.2f} MB)")


if __name__ == "__main__":
    main()
