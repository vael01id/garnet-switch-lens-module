#!/usr/bin/env python3
"""
Applies all v1.6 DEX bytecode patches and injects the 4-byte aligned STORED
ZoomManager smoother (`classes8.dex`) into the stock MIUI/Leica `com.android.camera.apk`
used on garnet OxygenOS 16 port.

Produces a bit-for-bit identical output APK to `garnet_patcher patch-apk`.
Note: For standalone installation (outside post-boot bind-mount), sign the output APK
with v1 + v2/v3 keys and 4096-byte zipalign uncompressed .so/.dex/.arsc entries.
"""
import argparse
import hashlib
import os
import re
import struct
import zipfile
import zlib

# Peridot->i1() replacement bytecode (52 code units = 104 bytes):
# Photo (163) & Video (162) quick zoom buttons: [0.6x, 1.0x, 3.0x, 5.0x, 10.0x]
I1_NEW = (
    "2205fc03"          # new-instance v5, SparseArray
    "7010bd0c0500"      # invoke-direct {v5}, SparseArray.<init>()V
    "14009a99193f"      # const v0, 0.6f
    "7110f4cd0000"      # Float.valueOf(v0)
    "0c00"              # move-result-object v0
    "1501803f"          # const/high16 v1, 1.0f
    "7110f4cd0100"      # Float.valueOf(v1)
    "0c01"              # move-result-object v1
    "15024040"          # const/high16 v2, 3.0f
    "7110f4cd0200"      # Float.valueOf(v2)
    "0c02"              # move-result-object v2
    "1503a040"          # const/high16 v3, 5.0f
    "7110f4cd0300"      # Float.valueOf(v3)
    "0c03"              # move-result-object v3
    "15042041"          # const/high16 v4, 10.0f
    "7110f4cd0400"      # Float.valueOf(v4)
    "0c04"              # move-result-object v4
    "24542b2a1032"      # filled-new-array {v0,v1,v2,v3,v4}, [Float
    "0c00"              # move-result-object v0
    "1301a300"          # const/16 v1, #163 (Photo)
    "6e30c50c1500"      # v5.put(163, v0)
    "1301a200"          # const/16 v1, #162 (Video)
    "6e30c50c1500"      # v5.put(162, v0)
    "1105"              # return-object v5
    "0000"              # nop
)

# LZ1/l;->onAnimationUpdate (+0x02a..+0x058, 23 units = 46 bytes):
# Instant 4.7% first step at t=0 (1-3ms tap response) + strict monotonic guard (0 VSync #0 bounce)
LZ1L_EXP = (
    "52013b80"
    "5232ca1b"
    "712081df2100"
    "0a01"
    "2e010401"
    "3a011200"
    "52003b80"
    "71207ddf2000"
    "0a00"
    "2d000400"
    "3d000300"
    "2806"
)
LZ1L_NEW = (
    "52013b80"          # iget v1, v0, FragmentDualCameraAdjust;->U:F
    "5232ca1b"          # iget v2, v3, LZ1/l;->b:F
    "2d000401"          # cmpl-float v0, v4, v1
    "39000a00"          # if-nez v0, +0xa (-> monotonic check)
    "a7020201"          # sub-float v2, v2, v1
    "1500403d"          # const/high16 v0, 0.046875f
    "c802"              # mul-float/2addr v2, v0
    "a6040102"          # add-float v4, v1, v2
    "2808"              # goto +8 (-> oh)
    "2d020201"          # cmpl-float v2, v2, v1
    "b220"              # mul-int/2addr v0, v2
    "3d000900"          # if-lez v0, +9 (-> return-void)
    "00000000"          # nop; nop
)

# video/z;->k (H.264 cooler bitrate boost: 1080p30 20M->40M, 4K30/1080p60 50M->60M)
H264_EXP = (
    "62024c306e10b00902000c0262033bd75263fc44526412452207fe23701060d1"
    "07006e2066d137001a0370106e206ad137006e2066d147001a0391106e206ad1"
    "37006e1077d107000c036e20a8d332000a04380437006e20acd332000c021f02"
    "8b241b035d2c0100722022d432000a0438040d00722026d432000c031f03fc23"
    "7110bad003000a032803020312001b04280f0100722022d442000a0738071000"
    "54670445722026d442000c021f02fc237110bad002000a025972523f01322803"
    "02021200"
)
H264_NEW = (
    "02021200140340787d0135320500da020202285914030087930335325500da02"
    "0206db020205284f" + ("00" * 156)
)

# Common->M0()[F: smooth ~500ms ease-out spline timing on L0=[0.5,1,2,5,10,15,50,125]
M0_EXP_HEX = struct.pack("<8f", 0.0, 126.0, 253.0, 420.0, 547.0, 621.0, 840.0, 1000.0).hex()
M0_NEW_HEX = struct.pack("<8f", 0.0, 600.0, 1100.0, 1650.0, 2065.0, 2315.0, 2915.0, 3365.0).hex()

DEX_PATCHES = {
    "classes.dex": [
        ("LZ1/l;->onAnimationUpdate (zero-bounce instant first step)", 0x500232, LZ1L_EXP, LZ1L_NEW),
        ("LZ1/s;->onAnimationStart (NOP A4 targetZoom collision)", 0x50071A, "72207e110100", "000000000000"),
        ("dual/a;->onAnimationStart (NOP A4 targetZoom collision)", 0x7C9BEE, "72207e111000", "000000000000"),
        ("Lb0/U;->i (FHD60 on Wide/VideoSAT; clamp UW/Front/Macro to 30fps+EIS)", 0x6F551C, "7130d8a0210c0a0138010900", "52c13ed7dd01010339010900"),
        ("Lb0/U;->q (unlock FHD60 VideoSAT)", 0x6F5BC0, "38023400", "28340000"),
        ("Lb0/U;->v (unlock FHD60 SAT maxFps cap)", 0x6F66AC, "59cfff74", "00000000"),
        ("Lb0/p0;->getValueDisplayString (Pro Mode crash fix)", 0x6FE458, "6e300ca22100", "6f304aa72100"),
        ("FragmentDualCameraAdjust;->Zh (live bidirectional 1x<->0.6x slider)", 0x7C4026, "39002000", "28200000"),
        ("FragmentDualCameraAdjust;->bi (PathInterpolator 0,0.5625,0,1 extreme ease-out)", 0x7C494A, "54a43f806e107316040054a43f802205cc0b7010022905006e207b16540054a43f80", "6e10731604002205ce0b12061507103f1508803f7058072965676e207b1654000000"),
        ("FragmentDualCameraAdjust;->notifyAfterFrameAvailable (enable spline bi for SAT)", 0x7C6E0E, "62005b0354005d036e10a5f200000a003800110059685f80", "123000000000000000000000000000000000000059605f80"),
        ("FragmentDualCameraAdjust;->provideAnimateElement (fallback Y LinearOutSlowInInterpolator)", 0x7C7E46, "220ecc0b701002290e00", "220ecc167010177a0e00"),
        ("FragmentDualCameraAdjust;->provideAnimateElement (fallback Y 100ms -> 500ms)", 0x7C7EDC, "160e6400", "160ef401"),
        ("FragmentDualCameraAdjust;->provideAnimateElement (fallback Y 150ms -> 500ms)", 0x7C7EF8, "160e9600", "160ef401"),
    ],
    "classes2.dex": [
        ("ZoomRatioToggleView;->b (neutralize AnimatorSet.setDuration 400L -> 0L)", 0x5A7E2E, "16009001", "16000000"),
        ("LD3/e;->C()I (Native Photo SAT roleId 60 -> 62 = cameraId 4)", 0x4B797C, "13033c00", "13033e00"),
        ("LD3/e;->K()Z (Native Photo SAT hasSATCamera roleId 60 -> 62)", 0x4B7CAA, "13023c00", "13023e00"),
        ("StopPointScaleZoomSliderDrawAdapter;->initValue (0.6f lower bound)", 0x5C9F68, "38030500", "28050000"),
        ("Lw5/e;->B3 (isZoomRatioSupported = true)", 0x88CF00, "120038053d00", "12100f000000"),
        ("Lw5/e;->V (default sharpness 1 -> 0 anti-oversharpen)", 0x88FC98, "12107110c2d00000", "12000f0000000000"),
        ("Lw5/E;->u0 (apply sharpness=0 in Video+Photo; keep Pro manual)", 0x86AC8E, "52228ede12f032020900", "52228ede3b0203001202"),
        ("Lw5/E;->H0 (bypass B0 check so applyUltraWideLDC sends level=1)", 0x868CAE, "38020b00", "00000000"),
        ("BaseModule;->updateUltraWideLDC (bypass B0 check for SAT UW LDC)", 0x514994, "38002e00", "00000000"),
        ("video/z;->k (H.264 cooler bitrate boost: 20M->40M, 50M->60M)", 0x568B8E, H264_EXP, H264_NEW),
        ("video/I;->a (HEVC bitrate 1.33x cooler boost: 15.4M->20.5M, 38.5M->51.3M)", 0x560704, "280f5277553f280c6e2027d103000c002321782b7130032c04015277553f", "280a5277553f280c5277553f0f07000000000000da070704db0707030f07"),
        (
            "Lt5/f;.a0(FI)Z (Klee-style ZoomManager.apply smoother trampoline)",
            0x84FA46,
            "220afe231a0b34fb702062d1ba00020b13006e2065d1ba001a0ccd0a6e206ad1ca00540c59dc6e1038220c000c0c6e206ad1ca001a0cd10a6e206ad1ca006e2066d11a006e1077d10a000c0a239c782b1a0d067f7130032cad0c",
            "1a0d067f711052d00d000c0a1a0bca8a1c0eef232430732bee0e0c0c6e3067d0ba0c0c0a7701a3d013000c0b7110c2d001000c0c2430782bb00c0c0b120c6e30e3d1ca0b0c0a380a03000f04020b130000000000000000000000",
        ),
    ],
    "classes3.dex": [
        ("LG6/d;->j (clamp maxZoom to real HAL 10x Photo / 6x Video)", 0x1A7874, "5411ad00", "12011101"),
        ("LR7/d;->a fallback -> Peridot", 0x1AF924, "1200711017ce00000c00", "1c00bc28110000000000"),
        (
            "Peridot;->i1() (5 zoom buttons [0.6x, 1x, 3x, 5x, 10x])",
            0x462398,
            "2205fc037010bd0c050014009a99193f7110f4cd00000c001501803f7110f4cd01000c01150200407110f4cd02000c0224302b2a10020c031304a3006e30c50c450324302b2a10020c031304a2006e30c50c450324302b2a10020c001301ad006e30c50c15001105",
            I1_NEW,
        ),
        ("CommonNote;->V1 (AlgoUp = false)", 0x46E890, "12100f00", "12000f00"),
        ("CommonNote;->i6 (LocalParallel = false)", 0x46ED58, "12100f00", "12000f00"),
        ('Peridot;->E0 ("2:9248x6944" 64MP OV64B40)', 0x462BD8, "f6f7", "b7f7"),
        ("Common;->Q2 (isSupportSatIsZooming = true)", 0x468CB8, "2000a5280f00", "12100f000000"),
        ("Common;->d4 (Macro roleId=22 = true)", 0x4692CC, "2000a8280f00", "12100f000000"),
        ('Common;->Q6 ("ultra_wide:wide:front:macro")', 0x46AF08, "0000", "0cf8"),
        ("Common;->G6 (isSupportSatZoomSpline = true)", 0x469DD0, "2000a5280f00", "12100f000000"),
        ("Common;->k7 (UW lens switch = true)", 0x46A03C, "2000aa280f00", "12100f000000"),
        ("Common;->w5 (isSupportSAT = true)", 0x469998, "2000a5280f00", "12100f000000"),
        ("Common;->x7 (isSupportVideoSAT = true)", 0x46A164, "2000a5280f00", "12100f000000"),
        ("Common;->l0 (VideoSAT maxZoom 12.0f -> 10.0f)", 0x46A1F4, "150040410f00", "150020410f00"),
        ("Common;->M0()[F (smooth ~500ms ease-out spline timing)", 0x46A2C8, M0_EXP_HEX, M0_NEW_HEX),
    ],
    "classes5.dex": [
        ("LZa/g;->i (graceful fallback, no IllegalStateException)", 0x498BBA, "38005400", "00000000"),
    ],
}


def load_zoom_smoother_dex() -> bytes:
    hdr_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "zoom_smoother_dex.h")
    with open(hdr_path, "r", encoding="utf-8") as f:
        text = re.sub(r"/\*.*?\*/", "", f.read(), flags=re.S)
    m = re.search(r"ZOOM_SMOOTHER_DEX\[\d*\]\s*=\s*\{(.*?)\};", text, re.S)
    if not m:
        raise RuntimeError(f"Could not parse ZOOM_SMOOTHER_DEX from {hdr_path}")
    return bytes(int(x, 16) for x in re.findall(r"0x[0-9a-fA-F]{2}", m.group(1)))


def apply_dex(buf: bytearray, patches, label: str) -> int:
    for name, off, exp_h, new_h in patches:
        exp, new = bytes.fromhex(exp_h), bytes.fromhex(new_h)
        assert len(exp) == len(new), f"Length mismatch in {name}"
        cur = bytes(buf[off : off + len(exp)])
        if cur != exp:
            raise RuntimeError(f"[{label}] Mismatch at {name} @0x{off:x}: got {cur.hex()}, expected {exp_h}")
        buf[off : off + len(new)] = new
        print(f"  [{label}] PATCHED {name}")
    buf[12:32] = hashlib.sha1(buf[32:]).digest()
    struct.pack_into("<I", buf, 8, zlib.adler32(buf[12:]) & 0xFFFFFFFF)
    return zlib.crc32(buf) & 0xFFFFFFFF


def append_stored_dex(apk_buf: bytearray, dex_name: str, dex_data: bytes) -> bytes:
    """Append a 4-byte aligned STORED classesN.dex matching garnet_patcher.c layout."""
    eocd = apk_buf.rfind(b"PK\x05\x06", max(0, len(apk_buf) - 65557))
    if eocd < 0:
        raise RuntimeError("Invalid APK: EOCD not found")
    cd_sz, cd_off = struct.unpack_from("<II", apk_buf, eocd + 12)
    name_b = dex_name.encode("ascii")
    nlen = len(name_b)
    dex_sz = len(dex_data)
    dex_crc = zlib.crc32(dex_data) & 0xFFFFFFFF

    pad = (4 - ((cd_off + 30 + nlen) & 3)) & 3
    new_lfh_off = cd_off + pad
    new_cd_off = new_lfh_off + 30 + nlen + dex_sz
    new_cd_sz = cd_sz + 46 + nlen

    lfh = bytearray(30)
    struct.pack_into("<I", lfh, 0, 0x04034B50)
    struct.pack_into("<H", lfh, 4, 20)
    struct.pack_into("<H", lfh, 12, 0x0021)
    struct.pack_into("<III", lfh, 14, dex_crc, dex_sz, dex_sz)
    struct.pack_into("<H", lfh, 26, nlen)

    cd_entry = bytearray(46)
    struct.pack_into("<I", cd_entry, 0, 0x02014B50)
    struct.pack_into("<HH", cd_entry, 4, 20, 20)
    struct.pack_into("<H", cd_entry, 14, 0x0021)
    struct.pack_into("<III", cd_entry, 16, dex_crc, dex_sz, dex_sz)
    struct.pack_into("<H", cd_entry, 28, nlen)
    struct.pack_into("<I", cd_entry, 42, new_lfh_off)

    new_eocd = bytearray(apk_buf[eocd:])
    struct.pack_into("<H", new_eocd, 8, struct.unpack_from("<H", new_eocd, 8)[0] + 1)
    struct.pack_into("<H", new_eocd, 10, struct.unpack_from("<H", new_eocd, 10)[0] + 1)
    struct.pack_into("<II", new_eocd, 12, new_cd_sz, new_cd_off)

    return b"".join([
        bytes(apk_buf[:cd_off]),
        b"\x00" * pad,
        bytes(lfh),
        name_b,
        dex_data,
        bytes(apk_buf[cd_off : cd_off + cd_sz]),
        bytes(cd_entry),
        name_b,
        bytes(new_eocd),
    ])


def main():
    ap = argparse.ArgumentParser(description="Apply v1.6 DEX patches + ZoomManager smoother to stock com.android.camera.apk")
    ap.add_argument("--stock-apk", required=True, help="Path to stock com.android.camera.apk")
    ap.add_argument("--out-apk", required=True, help="Output path for patched APK")
    args = ap.parse_args()

    repls = {}
    max_dex_num = 1
    with zipfile.ZipFile(args.stock_apk, "r") as zin:
        for name in zin.namelist():
            m = re.fullmatch(r"classes(\d*)\.dex", name)
            if m:
                max_dex_num = max(max_dex_num, int(m.group(1)) if m.group(1) else 1)
        for dname, plist in DEX_PATCHES.items():
            zi = zin.getinfo(dname)
            buf = bytearray(zin.read(dname))
            ncrc = apply_dex(buf, plist, dname)
            repls[dname] = (zi, buf, ncrc)

    with open(args.stock_apk, "rb") as f:
        apk_buf = bytearray(f.read())

    for _, (zi, buf, ncrc) in repls.items():
        nlen, elen = struct.unpack_from("<HH", apk_buf, zi.header_offset + 26)
        struct.pack_into("<I", apk_buf, zi.header_offset + 14, ncrc)
        doff = zi.header_offset + 30 + nlen + elen
        apk_buf[doff : doff + len(buf)] = buf

    eocd = apk_buf.rfind(b"PK\x05\x06", max(0, len(apk_buf) - 65557))
    cd_sz, cd_off = struct.unpack_from("<II", apk_buf, eocd + 12)
    pos = cd_off
    cd_end = cd_off + cd_sz
    while pos < cd_end:
        nlen, elen, clen = struct.unpack_from("<HHH", apk_buf, pos + 28)
        name = apk_buf[pos + 46 : pos + 46 + nlen].decode("utf-8")
        if name in repls:
            struct.pack_into("<I", apk_buf, pos + 16, repls[name][2])
        pos += 46 + nlen + elen + clen

    smoother_dex = load_zoom_smoother_dex()
    next_dex = f"classes{max_dex_num + 1}.dex"
    out_bytes = append_stored_dex(apk_buf, next_dex, smoother_dex)
    with open(args.out_apk, "wb") as f:
        f.write(out_bytes)
    print(f"  [APK] INJECTED {next_dex} ({len(smoother_dex)} bytes, ZoomManager smoother)")
    print(f"[OK] Wrote patched APK: {args.out_apk} SHA256={hashlib.sha256(out_bytes).hexdigest()}")


if __name__ == "__main__":
    main()
