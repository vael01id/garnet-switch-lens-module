#!/usr/bin/env python3
"""
Reproducibly patches the 4 vendor HAL shared libraries for garnet (Poco X6 5G /
Redmi Note 13 Pro 5G) on HyperOS 3 vendor OS3.0.304.0.WNRCNXM.

Every patch verifies the exact expected stock bytes before modifying anything.
"""
import argparse
import hashlib
import os

PATCHES = {
    "hw/camera.qcom.so": {
        "src_name": "camera.qcom.so",
        "expected_sha256": "ad17d9da8d80945a32cce8b33f646320569e7182b4c61ccecae7f7ed992a6124",
        "hunks": [
            # 0xfda2d0: QSAT / Video SAT compatibility branch (b.eq -> b)
            (0xFDA2D0, "e00f0054", "8b000014"),
        ],
    },
    "hw/com.qti.chi.override.so": {
        "src_name": "com.qti.chi.override.so",
        "expected_sha256": "0baadab7ce268e968ee647c4a6a0c2a5152cf20f3591d0f1e270120911e550e9",
        "hunks": [
            # Extend RX PT_LOAD filesz/memsz (+0x50) for ARM64 code caves at 0x490a40..0x490a70
            (0x0000D0, "30", "80"),
            (0x0000D8, "30", "80"),
            # Native SAT logical camera & video/photo role configuration patches
            (0x028BB0, "666626", "0c9319"),
            (0x0B6BB4, "3f", "3e"),
            # Hook FinalizeFeatureInstances @0x162ca8 -> b 0x490a50 (SWRemosaic guard cave)
            (0x162CA8, "f603002a", "6ab70c14"),
            (0x1A3304, "e803182a6a0480d2290080520a00c1f2", "6a0480d20a00a6f20a00c1f229008052"),
            (0x1A3316, "c8", "d8"),
            (0x298625, "0400fd", "0800bd"),
            (0x29864E, "3e", "3f"),
            (0x298651, "01", "02"),
            (0x298678, "681200f0", "28100090"),
            (0x298681, "810c", "e135"),
            (0x298688, "08213791", "0001c03d"),
            (0x29869C, "088400f8", "0004813c"),
            (0x2FF6C5, "0101", "f900"),
            (0x3D5650, "f6", "d6"),
            (0x3D5670, "164400b8", "f4ec0294"),
            # Code cave 1 @0x490a40
            (0x490A40, "000000000000000000000000", "e9078052094400b8c0035fd6"),
            # Code cave 2 @0x490a50: ignore SWRemosaic (type 0x22) init error in FinalizeFeatureInstances
            (0x490A50, "00", "c0"),
            (
                0x490A53,
                "0000000000000000000000000000000000000000000000000000000000",
                "34a80352f8084941f9080140b91f890071e003801af603002a9048f317",
            ),
        ],
    },
    "com.qti.feature2.mcreprocrt.so": {
        "src_name": "com.qti.feature2.mcreprocrt.so",
        "expected_sha256": "4bc491519f1ca27229ce635bf4b25824cf2d3701cee1e940cbe164e209fcb949",
        "hunks": [
            # 0x033438: 'MultiCameraVideoSATNoGPU' -> 'MultiCameraSATWithQSAT'
            (0x033439, "d4", "78"),
        ],
    },
    "com.qti.feature2.generic.so": {
        "src_name": "com.qti.feature2.generic.so",
        "expected_sha256": "399241073817b2e497b7271e0875980b69df77b529520a56f67599a5c4d4f51c",
        "hunks": [
            # Extend RX PT_LOAD filesz/memsz (+0x20) for NULL-guard cave @0x0b28d0
            (0x0000D0, "d0", "f0"),
            (0x0000D8, "d0", "f0"),
            # Hook @0x038b34 -> b 0x0b28d0
            (0x038B34, "16215ef9", "67e70114"),
            # Cave @0x0b28d0: guard NULL pPipeline ([x2, #0x18]) before ldr x22, [x8, #0x3c40]
            (0x0B28D0, "0000000000000000000000000000000000000000", "ea0308aa280140b9aa1ec3b456215ef99718fe17"),
        ],
    },
}


def sha256_bytes(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()


def main():
    ap = argparse.ArgumentParser(description="Patch stock garnet OS3.0.304.0 camera HAL binaries")
    ap.add_argument("--stock-dir", required=True, help="Directory containing stock vendor .so dumps")
    ap.add_argument("--out-dir", required=True, help="Output directory (mirrors system/vendor/lib64/)")
    args = ap.parse_args()

    for rel_out, spec in PATCHES.items():
        src_path = os.path.join(args.stock_dir, spec["src_name"])
        if not os.path.isfile(src_path):
            raise FileNotFoundError(f"Missing stock binary: {src_path}")
        buf = bytearray(open(src_path, "rb").read())

        for off, exp_hex, new_hex in spec["hunks"]:
            exp = bytes.fromhex(exp_hex)
            new = bytes.fromhex(new_hex)
            assert len(exp) == len(new)
            cur = bytes(buf[off : off + len(exp)])
            if cur != exp:
                raise RuntimeError(
                    f"{spec['src_name']} mismatch @0x{off:x}: got {cur.hex()}, expected {exp_hex} "
                    "(wrong vendor firmware version?)"
                )
            buf[off : off + len(new)] = new

        got_sha = sha256_bytes(buf)
        if got_sha != spec["expected_sha256"]:
            raise RuntimeError(f"SHA256 mismatch for {rel_out}: got {got_sha}, expected {spec['expected_sha256']}")

        out_path = os.path.join(args.out_dir, rel_out)
        os.makedirs(os.path.dirname(out_path), exist_ok=True)
        with open(out_path, "wb") as f:
            f.write(buf)
        print(f"[OK] {rel_out} ({len(buf)} bytes) SHA256={got_sha}")


if __name__ == "__main__":
    main()
