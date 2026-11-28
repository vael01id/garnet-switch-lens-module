/*
 * garnet_patcher.c — Universal zero-dependency, zero-relocation static-PIE ARM64
 * pattern patcher for garnet (Poco X6 5G / Redmi Note 13 Pro 5G).
 *
 * Works across OxygenOS 16 / ColorOS ports, HyperOS 1/2/3/4, and AOSP / Lunaris.
 *
 * Build (zero relocations):
 *   clang --target=aarch64-linux-gnu -fuse-ld=lld -O2 -Wall -Wextra -Werror \
 *     -nostdlib -ffreestanding -fno-builtin -static-pie -fno-stack-protector \
 *     -fno-asynchronous-unwind-tables -fPIE -Wl,-e,_start -Wl,--strip-all \
 *     scripts/garnet_patcher.c -o module/bin/arm64/garnet_patcher
 *
 * Commands:
 *   garnet_patcher patch-camx  <src_or_dash> <dst>
 *   garnet_patcher patch-hal   <camera_qcom|chi_override|mcreprocrt|feature2_generic> <src> <dst>
 *   garnet_patcher patch-tuned <wide|ultra> <src.bin> <dst.bin>
 *   garnet_patcher patch-apk   <src.apk> <dst.apk>
 */
#include <stdint.h>
#include <stddef.h>

#include "patch_rules.h"
#include "zoom_smoother_dex.h"

/* =========================================================================
 * 1. Freestanding Linux Syscalls & Memory / I/O Primitives
 * ========================================================================= */
#if defined(__aarch64__)
#define SYS_openat      56
#define SYS_close       57
#define SYS_read        63
#define SYS_write       64
#define SYS_lseek       62
#define SYS_mmap        222
#define SYS_munmap      215
#define SYS_exit_group  94

static inline long sc1(long n, long a) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x8) : "memory");
    return x0;
}
static inline long sc2(long n, long a, long b) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1) : "memory");
    return x0;
}
static inline long sc3(long n, long a, long b, long c) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory");
    return x0;
}
static inline long sc4(long n, long a, long b, long c, long d) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    register long x3 __asm__("x3") = d;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3) : "memory");
    return x0;
}
static inline long sc6(long n, long a, long b, long c, long d, long e, long f) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    register long x3 __asm__("x3") = d;
    register long x4 __asm__("x4") = e;
    register long x5 __asm__("x5") = f;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5) : "memory");
    return x0;
}
#elif defined(__x86_64__)
#define SYS_read        0
#define SYS_write       1
#define SYS_close       3
#define SYS_lseek       8
#define SYS_mmap        9
#define SYS_munmap      11
#define SYS_exit_group  231
#define SYS_openat      257

static inline long sc1(long n, long a) {
    long r;
    __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a) : "rcx", "r11", "memory");
    return r;
}
static inline long sc2(long n, long a, long b) {
    long r;
    __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a), "S"(b) : "rcx", "r11", "memory");
    return r;
}
static inline long sc3(long n, long a, long b, long c) {
    long r;
    __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a), "S"(b), "d"(c) : "rcx", "r11", "memory");
    return r;
}
static inline long sc4(long n, long a, long b, long c, long d) {
    long r;
    register long r10 __asm__("r10") = d;
    __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a), "S"(b), "d"(c), "r"(r10) : "rcx", "r11", "memory");
    return r;
}
static inline long sc6(long n, long a, long b, long c, long d, long e, long f) {
    long r;
    register long r10 __asm__("r10") = d;
    register long r8  __asm__("r8")  = e;
    register long r9  __asm__("r9")  = f;
    __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a), "S"(b), "d"(c), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
    return r;
}
#else
#error "Unsupported architecture"
#endif

#define AT_FDCWD       (-100)
#define O_RDONLY        0
#define O_WRONLY_CT     0x241
#define SEEK_END        2
#define PROT_RW         3
#define MAP_PRIV_ANON   0x22
#define ARRAY_LEN(a)    (sizeof(a) / sizeof((a)[0]))

void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) *d++ = *s++;
    return dst;
}

void *memset(void *dst, int c, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    while (n--) *d++ = (uint8_t)c;
    return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
    const uint8_t *p = (const uint8_t *)a, *q = (const uint8_t *)b;
    while (n--) {
        if (*p != *q) return (int)*p - (int)*q;
        p++; q++;
    }
    return 0;
}

static size_t xstrlen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }

static int xstrcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (int)(uint8_t)*a - (int)(uint8_t)*b;
}

static void xputs(int fd, const char *s) { sc3(SYS_write, fd, (long)s, (long)xstrlen(s)); }

static void xput_u32(int fd, uint32_t v) {
    char b[16], r[16];
    int i = 0;
    if (v == 0) b[i++] = '0';
    else while (v) { b[i++] = (char)('0' + (v % 10)); v /= 10; }
    for (int j = 0; j < i; j++) r[j] = b[i - 1 - j];
    sc3(SYS_write, fd, (long)r, i);
}

static uint16_t rd16(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint64_t rd64(const uint8_t *p) { return (uint64_t)rd32(p) | ((uint64_t)rd32(p + 4) << 32); }
static void wr16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void wr32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static void wr64(uint8_t *p, uint64_t v) { wr32(p, (uint32_t)v); wr32(p + 4, (uint32_t)(v >> 32)); }

static uint8_t *alloc_buf(size_t sz) {
    if (sz == 0) sz = 4096;
    long p = sc6(SYS_mmap, 0, (long)sz, PROT_RW, MAP_PRIV_ANON, -1, 0);
    if (p < 0 && p > -4096) return (uint8_t *)0;
    return (uint8_t *)p;
}

static void free_buf(uint8_t *p, size_t sz) {
    if (p) sc2(SYS_munmap, (long)p, (long)(sz ? sz : 4096));
}

static uint8_t *read_file_all(const char *path, size_t *out_sz) {
    long fd = sc4(SYS_openat, AT_FDCWD, (long)path, O_RDONLY, 0);
    if (fd < 0) return (uint8_t *)0;
    long sz = sc3(SYS_lseek, fd, 0, SEEK_END);
    if (sz < 0) { sc1(SYS_close, fd); return (uint8_t *)0; }
    sc3(SYS_lseek, fd, 0, 0);
    uint8_t *buf = alloc_buf((size_t)sz + 4096);
    if (!buf) { sc1(SYS_close, fd); return (uint8_t *)0; }
    size_t pos = 0;
    while (pos < (size_t)sz) {
        size_t chunk = (size_t)sz - pos;
        if (chunk > (16u << 20)) chunk = (16u << 20);
        long r = sc3(SYS_read, fd, (long)(buf + pos), (long)chunk);
        if (r <= 0) { sc1(SYS_close, fd); free_buf(buf, (size_t)sz + 4096); return (uint8_t *)0; }
        pos += (size_t)r;
    }
    sc1(SYS_close, fd);
    *out_sz = (size_t)sz;
    return buf;
}

static int write_file_all(const char *path, const uint8_t *buf, size_t sz) {
    long fd = sc4(SYS_openat, AT_FDCWD, (long)path, O_WRONLY_CT, 0644);
    if (fd < 0) return -1;
    size_t pos = 0;
    while (pos < sz) {
        size_t chunk = sz - pos;
        if (chunk > (16u << 20)) chunk = (16u << 20);
        long w = sc3(SYS_write, fd, (long)(buf + pos), (long)chunk);
        if (w <= 0) { sc1(SYS_close, fd); return -1; }
        pos += (size_t)w;
    }
    sc1(SYS_close, fd);
    return 0;
}

/* =========================================================================
 * 2. Pattern Matching & Checksums (CRC32, Adler32, SHA-1)
 * ========================================================================= */
static size_t find_bytes(const uint8_t *hay, size_t hlen, const uint8_t *ned, size_t nlen, size_t start) {
    if (nlen == 0 || start + nlen > hlen) return (size_t)-1;
    uint8_t first = ned[0], last = ned[nlen - 1];
    size_t limit = hlen - nlen;
    for (size_t i = start; i <= limit; i++) {
        if (hay[i] == first && hay[i + nlen - 1] == last) {
            if (nlen <= 2 || memcmp(hay + i, ned, nlen) == 0) return i;
        }
    }
    return (size_t)-1;
}

static int find_unique(const uint8_t *hay, size_t hlen, const uint8_t *ned, size_t nlen, size_t *out_pos) {
    size_t p1 = find_bytes(hay, hlen, ned, nlen, 0);
    if (p1 == (size_t)-1) return 0;
    if (find_bytes(hay, hlen, ned, nlen, p1 + 1) != (size_t)-1) return 2;
    *out_pos = p1;
    return 1;
}

static int apply_pat_rules(uint8_t *buf, size_t sz, const PatRule *rules, size_t nrules) {
    size_t hits[64];
    uint8_t ok[64];
    int applied = 0;
    /* Two-pass resolve-then-write so overlapping context checks see unmodified bytes */
    for (size_t i = 0; i < nrules && i < 64; i++) {
        hits[i] = 0;
        ok[i] = (find_unique(buf, sz, RULE_BLOB + rules[i].ctx_off, rules[i].ctx_len, &hits[i]) == 1) ? 1 : 0;
    }
    for (size_t i = 0; i < nrules && i < 64; i++) {
        if (ok[i]) {
            memcpy(buf + hits[i] + rules[i].rel, RULE_BLOB + rules[i].rep_off, rules[i].rep_len);
            applied++;
        }
    }
    return applied;
}

static uint32_t crc32_calc(const uint8_t *buf, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)(-(int32_t)(crc & 1u)));
    }
    return ~crc;
}

static uint32_t adler32_calc(const uint8_t *buf, size_t len) {
    uint32_t a = 1, b = 0;
    const uint32_t MOD = 65521u;
    size_t i = 0;
    while (i < len) {
        size_t blk = len - i;
        if (blk > 5552) blk = 5552;
        for (size_t j = 0; j < blk; j++) { a += buf[i + j]; b += a; }
        a %= MOD; b %= MOD;
        i += blk;
    }
    return (b << 16) | a;
}

static inline uint32_t rol32(uint32_t v, int bits) { return (v << bits) | (v >> (32 - bits)); }

static void sha1_block(uint32_t h[5], const uint8_t blk[64]) {
    uint32_t w[80];
    for (int i = 0; i < 16; i++)
        w[i] = ((uint32_t)blk[i*4]<<24)|((uint32_t)blk[i*4+1]<<16)|((uint32_t)blk[i*4+2]<<8)|blk[i*4+3];
    for (int i = 16; i < 80; i++) w[i] = rol32(w[i-3]^w[i-8]^w[i-14]^w[i-16], 1);
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
    for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20)      { f = (b & c) | ((~b) & d); k = 0x5A827999u; }
        else if (i < 40) { f = b ^ c ^ d;            k = 0x6ED9EBA1u; }
        else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDCu; }
        else             { f = b ^ c ^ d;            k = 0xCA62C1D6u; }
        uint32_t t = rol32(a, 5) + f + e + k + w[i];
        e = d; d = c; c = rol32(b, 30); b = a; a = t;
    }
    h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
}

static void sha1_calc(const uint8_t *data, size_t len, uint8_t out[20]) {
    uint32_t h[5] = {0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u, 0xC3D2E1F0u};
    size_t pos = 0;
    while (pos + 64 <= len) { sha1_block(h, data + pos); pos += 64; }
    uint8_t tail[128];
    memset(tail, 0, sizeof(tail));
    size_t rem = len - pos;
    memcpy(tail, data + pos, rem);
    tail[rem] = 0x80;
    size_t total_blk = (rem < 56) ? 64 : 128;
    uint64_t bitlen = (uint64_t)len * 8u;
    for (int i = 0; i < 8; i++) tail[total_blk - 1 - i] = (uint8_t)(bitlen >> (i * 8));
    sha1_block(h, tail);
    if (total_blk == 128) sha1_block(h, tail + 64);
    for (int i = 0; i < 5; i++) {
        out[i*4]   = (uint8_t)(h[i] >> 24); out[i*4+1] = (uint8_t)(h[i] >> 16);
        out[i*4+2] = (uint8_t)(h[i] >> 8);  out[i*4+3] = (uint8_t)h[i];
    }
}

/* =========================================================================
 * 3. ELF64 / AArch64 Inspection, Cave Allocation & Instruction Encoding
 * ========================================================================= */
static int validate_elf64_aarch64(const uint8_t *buf, size_t sz) {
    if (sz < 64 || memcmp(buf, "\x7f""ELF", 4) != 0) return 0;
    if (buf[4] != 2 || buf[5] != 1) return 0; /* ELF64, little-endian */
    if (rd16(buf + 18) != 183) return 0;      /* EM_AARCH64 */
    uint64_t phoff = rd64(buf + 32);
    uint16_t phent = rd16(buf + 54);
    uint16_t phnum = rd16(buf + 56);
    return (phent >= 56 && phnum != 0 && phoff <= sz && (uint64_t)phnum * phent <= sz - phoff);
}

static int elf_has_str(const uint8_t *buf, size_t sz, const char *s) {
    return find_bytes(buf, sz, (const uint8_t *)s, xstrlen(s), 0) != (size_t)-1;
}

static int is_in_rx_seg(const uint8_t *buf, size_t sz, uint64_t off, uint64_t len) {
    (void)sz;
    uint64_t phoff = rd64(buf + 32);
    uint16_t phent = rd16(buf + 54), phnum = rd16(buf + 56);
    for (uint16_t i = 0; i < phnum; i++) {
        uint64_t po = phoff + (uint64_t)i * phent;
        if (rd32(buf + po) == 1 && (rd32(buf + po + 4) & 1)) {
            uint64_t p_off = rd64(buf + po + 8), p_fsz = rd64(buf + po + 32);
            if (off >= p_off && off + len <= p_off + p_fsz) return 1;
        }
    }
    return 0;
}

static int alloc_rx_cave_for(uint8_t *buf, size_t sz, uint64_t hook_off, uint64_t align, uint64_t need, uint64_t *out_cave_off) {
    if (!validate_elf64_aarch64(buf, sz)) return -1;
    uint64_t phoff = rd64(buf + 32);
    uint16_t phent = rd16(buf + 54), phnum = rd16(buf + 56);
    for (uint16_t i = 0; i < phnum; i++) {
        uint64_t po = phoff + (uint64_t)i * phent;
        if (rd32(buf + po) != 1 || !(rd32(buf + po + 4) & 1)) continue;
        uint64_t p_off = rd64(buf + po + 8),  p_vaddr = rd64(buf + po + 16);
        uint64_t p_fsz = rd64(buf + po + 32), p_msz   = rd64(buf + po + 40);
        if (hook_off < p_off || hook_off >= p_off + p_fsz) continue;
        if (p_fsz != p_msz) return -1;
        uint64_t end = p_off + p_fsz;
        uint64_t cave = (end + align - 1) & ~(align - 1);
        uint64_t new_end = cave + need;
        if (new_end > sz) return -1;
        uint64_t dist = (cave > hook_off) ? (cave - hook_off) : (hook_off - cave);
        if (dist >= 0x07F00000ULL) return -1; /* ARM64 B/BL +/-128MB reach */
        uint64_t v_cave = p_vaddr + (cave - p_off);
        uint64_t v_new_end = p_vaddr + (new_end - p_off);
        for (uint16_t j = 0; j < phnum; j++) {
            if (j == i) continue;
            uint64_t q = phoff + (uint64_t)j * phent;
            if (rd32(buf + q) != 1) continue;
            uint64_t qo = rd64(buf + q + 8),  qv  = rd64(buf + q + 16);
            uint64_t qfs = rd64(buf + q + 32), qms = rd64(buf + q + 40);
            if (cave < qo + qfs && new_end > qo) return -1;
            if (v_cave < qv + qms && v_new_end > (qv & ~0xFFFULL)) return -1;
        }
        for (uint64_t k = cave; k < new_end; k++) if (buf[k] != 0) return -1;
        uint64_t grow = new_end - end;
        wr64(buf + po + 32, p_fsz + grow);
        wr64(buf + po + 40, p_msz + grow);
        *out_cave_off = cave;
        return 0;
    }
    return -1;
}

static uint64_t elf_file_to_vaddr(const uint8_t *buf, size_t sz, uint64_t foff) {
    (void)sz;
    uint64_t phoff = rd64(buf + 32);
    uint16_t phent = rd16(buf + 54), phnum = rd16(buf + 56);
    for (uint16_t i = 0; i < phnum; i++) {
        uint64_t po = phoff + (uint64_t)i * phent;
        if (rd32(buf + po) != 1) continue;
        uint64_t p_off = rd64(buf + po + 8), p_va = rd64(buf + po + 16), p_fsz = rd64(buf + po + 32);
        if (foff >= p_off && foff < p_off + p_fsz) return p_va + (foff - p_off);
    }
    return (uint64_t)-1;
}

static uint64_t elf_vaddr_to_file(const uint8_t *buf, size_t sz, uint64_t vaddr) {
    (void)sz;
    uint64_t phoff = rd64(buf + 32);
    uint16_t phent = rd16(buf + 54), phnum = rd16(buf + 56);
    for (uint16_t i = 0; i < phnum; i++) {
        uint64_t po = phoff + (uint64_t)i * phent;
        if (rd32(buf + po) != 1) continue;
        uint64_t p_off = rd64(buf + po + 8), p_va = rd64(buf + po + 16), p_fsz = rd64(buf + po + 32);
        if (vaddr >= p_va && vaddr < p_va + p_fsz) return p_off + (vaddr - p_va);
    }
    return (uint64_t)-1;
}

static int elf_cstr_eq(const uint8_t *buf, size_t sz, uint64_t vaddr, const char *s) {
    uint64_t foff = elf_vaddr_to_file(buf, sz, vaddr);
    if (foff == (uint64_t)-1) return 0;
    size_t slen = xstrlen(s);
    if (foff + slen + 1 > sz) return 0;
    return memcmp(buf + foff, s, slen + 1) == 0;
}

static int elf_is_bss_vaddr(const uint8_t *buf, size_t sz, uint64_t vaddr) {
    (void)sz;
    uint64_t phoff = rd64(buf + 32);
    uint16_t phent = rd16(buf + 54), phnum = rd16(buf + 56);
    for (uint16_t i = 0; i < phnum; i++) {
        uint64_t po = phoff + (uint64_t)i * phent;
        if (rd32(buf + po) != 1) continue;
        uint64_t p_va = rd64(buf + po + 16), p_fsz = rd64(buf + po + 32), p_msz = rd64(buf + po + 40);
        if (p_msz > p_fsz && vaddr >= p_va + p_fsz && vaddr < p_va + p_msz) return 1;
    }
    return 0;
}

static uint32_t arm64_b(uint64_t pc, uint64_t target, int is_bl) {
    int64_t diff = ((int64_t)target - (int64_t)pc) >> 2;
    return (is_bl ? 0x94000000u : 0x14000000u) | ((uint32_t)diff & 0x03FFFFFFu);
}

static int arm64_dec_adrp(uint64_t pc_va, uint32_t insn, uint64_t *out_page, uint32_t *out_rd) {
    if ((insn & 0x9F000000u) != 0x90000000u) return 0;
    int64_t imm = (int64_t)((((insn >> 5) & 0x7FFFFu) << 2) | ((insn >> 29) & 3u));
    if (imm & 0x100000) imm -= 0x200000;
    *out_page = (pc_va & ~0xFFFULL) + (uint64_t)(imm << 12);
    *out_rd = insn & 31u;
    return 1;
}

static int arm64_enc_adrp(uint64_t pc_va, uint64_t target_va, uint32_t rd, uint32_t *out_insn) {
    int64_t delta = ((int64_t)(target_va & ~0xFFFULL) - (int64_t)(pc_va & ~0xFFFULL)) >> 12;
    if (delta < -0x100000 || delta > 0xFFFFF) return 0;
    uint32_t u = (uint32_t)delta & 0x1FFFFFu;
    *out_insn = 0x90000000u | ((u & 3u) << 29) | (((u >> 2) & 0x7FFFFu) << 5) | (rd & 31u);
    return 1;
}

static uint32_t arm64_enc_add12(uint32_t rd, uint32_t rn, uint32_t imm12) {
    return 0x91000000u | ((imm12 & 0xFFFu) << 10) | ((rn & 31u) << 5) | (rd & 31u);
}

static int arm64_match_adrp_add_cstr(const uint8_t *buf, size_t sz, uint64_t f, uint64_t va,
                                     uint64_t max_add_steps, const char *cstr) {
    uint64_t page = 0;
    uint32_t rd = 0;
    if (!arm64_dec_adrp(va, rd32(buf + f), &page, &rd)) return 0;
    uint32_t add_pat = 0x91000000u | (rd << 5) | rd;
    for (uint64_t k = 1; k <= max_add_steps; k++) {
        uint32_t wk = rd32(buf + f + k * 4);
        if ((wk & 0xFFC003FFu) == add_pat) {
            return elf_cstr_eq(buf, sz, page + ((wk >> 10) & 0xFFFu), cstr);
        }
    }
    return 0;
}

static int is_cam_entry(const uint8_t *buf, size_t sz, uint64_t foff,
                        const char *name, uint32_t c1, uint32_t c2, uint32_t c3) {
    if (foff + 32 > sz) return 0;
    if (rd32(buf + foff + 8) != c1 || rd32(buf + foff + 12) != c2 || rd32(buf + foff + 16) != c3) return 0;
    return elf_cstr_eq(buf, sz, rd64(buf + foff), name);
}

static int find_masked_unique_rx(const uint8_t *buf, size_t sz, const uint32_t *words,
                                 const uint32_t *masks, size_t nwords, uint64_t *out_foff) {
    uint64_t phoff = rd64(buf + 32);
    uint16_t phent = rd16(buf + 54), phnum = rd16(buf + 56);
    uint64_t hit = 0;
    int count = 0;
    size_t nbytes = nwords * 4;
    for (uint16_t i = 0; i < phnum; i++) {
        uint64_t po = phoff + (uint64_t)i * phent;
        if (rd32(buf + po) != 1 || !(rd32(buf + po + 4) & 1)) continue;
        uint64_t p_off = rd64(buf + po + 8), p_fsz = rd64(buf + po + 32);
        if (p_fsz < nbytes || p_off + p_fsz > sz) continue;
        uint64_t end = p_off + p_fsz - nbytes;
        uint32_t w0 = words[0], m0 = masks[0];
        for (uint64_t f = p_off; f <= end; f += 4) {
            if ((rd32(buf + f) & m0) != w0) continue;
            int ok = 1;
            for (size_t k = 1; k < nwords; k++) {
                if ((rd32(buf + f + k * 4) & masks[k]) != words[k]) { ok = 0; break; }
            }
            if (ok) {
                hit = f;
                if (++count > 1) return 2;
            }
        }
    }
    if (count == 1) *out_foff = hit;
    return count;
}

/* =========================================================================
 * 4. CamX Override Configuration Patcher
 * ========================================================================= */
static const char *get_camx_key(int idx) {
    const char *p = CAMX_KEYS_FLAT;
    for (int i = 0; i < idx; i++) p += xstrlen(p) + 1;
    return p;
}

static int key_matches_line(const char *line, size_t llen, const char *kv) {
    size_t klen = 0;
    while (kv[klen] && kv[klen] != '=') klen++;
    return (llen > klen && memcmp(line, kv, klen) == 0 && line[klen] == '=');
}

static int cmd_patch_camx(const char *src, const char *dst) {
    size_t ssz = 0;
    uint8_t *sbuf = (xstrcmp(src, "-") == 0) ? (uint8_t *)0 : read_file_all(src, &ssz);
    uint8_t *out = alloc_buf(ssz + 8192);
    if (!out) return 1;
    size_t opos = 0;
    uint8_t seen[NCAMX_KEYS];
    memset(seen, 0, sizeof(seen));

    if (sbuf) {
        size_t pos = 0;
        while (pos < ssz) {
            size_t eol = pos;
            while (eol < ssz && sbuf[eol] != '\n') eol++;
            size_t llen = eol - pos;
            if (llen > 0 && sbuf[pos + llen - 1] == '\r') llen--;
            int replaced = 0;
            for (int k = 0; k < NCAMX_KEYS; k++) {
                const char *kv = get_camx_key(k);
                if (key_matches_line((const char *)(sbuf + pos), llen, kv)) {
                    size_t nlen = xstrlen(kv);
                    memcpy(out + opos, kv, nlen); opos += nlen;
                    out[opos++] = '\n';
                    seen[k] = 1;
                    replaced = 1;
                    break;
                }
            }
            if (!replaced) {
                memcpy(out + opos, sbuf + pos, llen); opos += llen;
                out[opos++] = '\n';
            }
            pos = (eol < ssz) ? (eol + 1) : ssz;
        }
        free_buf(sbuf, ssz + 4096);
    }
    for (int k = 0; k < NCAMX_KEYS; k++) {
        if (!seen[k]) {
            const char *kv = get_camx_key(k);
            size_t nlen = xstrlen(kv);
            memcpy(out + opos, kv, nlen); opos += nlen;
            out[opos++] = '\n';
        }
    }
    int rc = write_file_all(dst, out, opos);
    free_buf(out, ssz + 8192);
    if (rc == 0) xputs(1, "[OK] patched camxoverridesettingsOfUltra.txt\n");
    return rc == 0 ? 0 : 1;
}

/* =========================================================================
 * 5. Modular Vendor HAL Patchers
 * ========================================================================= */
typedef struct {
    uint64_t pc_strd, pc_tbl_adrp, pc_tbl_add, t_vt_va;
    uint64_t pc_mov8, pc_b0, pc_b1, pc_b2, pc_str8;
    uint32_t orig_strd, tbl_reg;
} ChiMcBlock;

static int find_garnet_mc_block(const uint8_t *buf, size_t sz, ChiMcBlock *out) {
    uint64_t phoff = rd64(buf + 32);
    uint16_t phent = rd16(buf + 54), phnum = rd16(buf + 56);
    int matches = 0;
    ChiMcBlock found;
    memset(&found, 0, sizeof(found));

    for (uint16_t i = 0; i < phnum; i++) {
        uint64_t po = phoff + (uint64_t)i * phent;
        if (rd32(buf + po) != 1 || !(rd32(buf + po + 4) & 1)) continue;
        uint64_t p_off = rd64(buf + po + 8), p_va = rd64(buf + po + 16), p_fsz = rd64(buf + po + 32);
        if (p_fsz < 256 || p_off + p_fsz > sz) continue;
        uint64_t end = p_off + p_fsz - 256;

        for (uint64_t f = p_off; f <= end; f += 4) {
            uint64_t va = p_va + (f - p_off);
            if (!arm64_match_adrp_add_cstr(buf, sz, f, va, 2, "MultiCamera")) continue;

            /* 1. str dN, [x0, #8] within +8..+24 */
            uint64_t pc_strd = 0;
            uint32_t orig_strd = 0;
            for (uint64_t d = 8; d <= 24; d += 4) {
                uint32_t w = rd32(buf + f + d);
                if ((w & 0xFFFFFFE0u) == 0xFD000400u) { pc_strd = f + d; orig_strd = w; break; }
            }
            if (!pc_strd) continue;

            /* 2. adrp+add rTbl -> t_mc ([UW, Wide]) followed by t_vt = t_mc+0x40 ([Wide, UW, Front]) */
            uint64_t pc_ta = 0, pc_td = 0, t_vt_va = 0;
            uint32_t rtbl = 0;
            for (uint64_t d = 24; d <= 80; d += 4) {
                uint64_t tpage = 0; uint32_t trd = 0;
                if (!arm64_dec_adrp(va + d, rd32(buf + f + d), &tpage, &trd)) continue;
                uint32_t add_pat = 0x91000000u | (trd << 5) | trd;
                for (uint64_t k = 1; k <= 3; k++) {
                    uint32_t wk = rd32(buf + f + d + k * 4);
                    if ((wk & 0xFFC003FFu) != add_pat) continue;
                    uint64_t t_mc_va = tpage + ((wk >> 10) & 0xFFFu);
                    uint64_t t_mc_fo = elf_vaddr_to_file(buf, sz, t_mc_va);
                    if (t_mc_fo != (uint64_t)-1 &&
                        is_cam_entry(buf, sz, t_mc_fo + 0x00, "UltrawidePhysicalCam", 2, 1, 2) &&
                        is_cam_entry(buf, sz, t_mc_fo + 0x20, "WidePhysicalCam",      0, 1, 0) &&
                        is_cam_entry(buf, sz, t_mc_fo + 0x40, "WidePhysicalCam",      0, 1, 0) &&
                        is_cam_entry(buf, sz, t_mc_fo + 0x60, "UltrawidePhysicalCam", 2, 1, 2) &&
                        is_cam_entry(buf, sz, t_mc_fo + 0x80, "FrontPhysicalCam",     1, 1, 1)) {
                        pc_ta = f + d; pc_td = f + d + k * 4; rtbl = trd; t_vt_va = t_mc_va + 0x40;
                    }
                    break;
                }
                if (pc_ta) break;
            }
            if (!pc_ta) continue;

            /* 3. mov w1, #8 within +4..+16 after pc_tbl_add */
            uint64_t pc_mov8 = 0;
            for (uint64_t d = 4; d <= 16; d += 4) {
                if (rd32(buf + pc_td + d) == 0x52800101u) { pc_mov8 = pc_td + d; break; }
            }
            if (!pc_mov8) continue;

            /* 4. adrp x8,<bss>; add x8,x8,#imm1; add x8,x8,#imm2; mov w1,#0x40; str x8,[x0],#8; VTCamera */
            uint64_t pc_b0 = 0, pc_b1 = 0, pc_b2 = 0, pc_str8 = 0;
            for (uint64_t d = 16; d <= 80; d += 4) {
                uint64_t q = pc_mov8 + d;
                uint64_t bpage = 0; uint32_t brd = 0;
                if (!arm64_dec_adrp(p_va + (q - p_off), rd32(buf + q), &bpage, &brd) || brd != 8) continue;
                uint64_t b1 = 0, b2 = 0;
                for (uint64_t k = 1; k <= 3; k++) {
                    if ((rd32(buf + q + k * 4) & 0xFFC003FFu) == 0x91000108u) { b1 = q + k * 4; break; }
                }
                if (!b1) continue;
                for (uint64_t k = 1; k <= 3; k++) {
                    if ((rd32(buf + b1 + k * 4) & 0xFFC003FFu) == 0x91000108u) { b2 = b1 + k * 4; break; }
                }
                if (!b2) continue;
                for (uint64_t s = 4; s <= 32; s += 4) {
                    if (rd32(buf + b2 + s) == 0xF8008408u) { pc_str8 = b2 + s; break; }
                }
                if (!pc_str8) continue;
                int has_mov64 = 0;
                for (uint64_t m = q; m < pc_str8; m += 4) {
                    if (rd32(buf + m) == 0x52800801u) { has_mov64 = 1; break; }
                }
                if (!has_mov64) { pc_str8 = 0; continue; }
                /* Confirm VTCamera string near pc_b0..pc_str8+24 (HyperOS 4 hoists it before pc_str8) */
                int has_vt = 0;
                uint64_t vt_end = pc_str8 + 24;
                if (vt_end > p_off + p_fsz - 16) vt_end = p_off + p_fsz - 16;
                for (uint64_t v = q; v <= vt_end; v += 4) {
                    if (arm64_match_adrp_add_cstr(buf, sz, v, p_va + (v - p_off), 3, "VTCamera")) {
                        has_vt = 1; break;
                    }
                }
                if (!has_vt) { pc_str8 = 0; continue; }
                pc_b0 = q; pc_b1 = b1; pc_b2 = b2;
                break;
            }
            if (!pc_b0) continue;

            found.pc_strd = pc_strd;     found.orig_strd = orig_strd;
            found.pc_tbl_adrp = pc_ta;   found.pc_tbl_add = pc_td;
            found.tbl_reg = rtbl;        found.t_vt_va = t_vt_va;
            found.pc_mov8 = pc_mov8;     found.pc_b0 = pc_b0;
            found.pc_b1 = pc_b1;         found.pc_b2 = pc_b2;
            found.pc_str8 = pc_str8;
            if (++matches > 1) return 2;
        }
    }
    if (matches == 1) *out = found;
    return matches;
}

static int find_pair_arr(const uint8_t *buf, size_t sz, uint64_t *out_va) {
    uint64_t phoff = rd64(buf + 32);
    uint16_t phent = rd16(buf + 54), phnum = rd16(buf + 56);
    uint64_t strict_va = 0, base_va = 0;
    int strict_cnt = 0, base_cnt = 0;

    for (uint16_t i = 0; i < phnum; i++) {
        uint64_t po = phoff + (uint64_t)i * phent;
        if (rd32(buf + po) != 1 || rd32(buf + po + 4) != 6) continue; /* RW PT_LOAD (.data.rel.ro) */
        uint64_t p_off = rd64(buf + po + 8), p_va = rd64(buf + po + 16), p_fsz = rd64(buf + po + 32);
        if (p_fsz < 0x120 || p_off + p_fsz > sz) continue;
        uint64_t start = (p_off + 0x80 + 7) & ~7ULL;
        uint64_t end = p_off + p_fsz - 0x88;

        for (uint64_t a = start; a <= end; a += 8) {
            uint64_t q0 = rd64(buf + a);
            if (!elf_is_bss_vaddr(buf, sz, q0)) continue;
            int seq_ok = 1;
            for (int k = 1; k < 5; k++) {
                uint64_t qk = rd64(buf + a + (uint64_t)k * 8);
                if (qk != q0 + (uint64_t)k * 0x38ULL || !elf_is_bss_vaddr(buf, sz, qk)) { seq_ok = 0; break; }
            }
            if (!seq_ok) continue;
            if (!is_cam_entry(buf, sz, a - 0x40, "UltrawidePhysicalCam", 2, 1, 2)) continue;
            if (!is_cam_entry(buf, sz, a - 0x20, "WidePhysicalCam",      0, 1, 0)) continue;
            if (!is_cam_entry(buf, sz, a + 0x28, "WidePhysicalCam",      0, 1, 0)) continue;
            if (!is_cam_entry(buf, sz, a + 0x48, "UltrawidePhysicalCam", 2, 1, 2)) continue;
            if (!is_cam_entry(buf, sz, a + 0x68, "FrontPhysicalCam",     1, 1, 1)) continue;

            uint64_t cand_va = p_va + (a - p_off);
            base_va = cand_va;
            base_cnt++;
            if (elf_cstr_eq(buf, sz, rd64(buf + a - 0x80), "RearTOFSensor") &&
                is_cam_entry(buf, sz, a - 0x60, "UltrawidePhysicalCam", 6, 1, 4)) {
                strict_va = cand_va;
                strict_cnt++;
            }
        }
    }
    if (strict_cnt == 1) { *out_va = strict_va; return 1; }
    if (strict_cnt == 0 && base_cnt == 1) { *out_va = base_va; return 1; }
    return 0;
}

static int patch_hal_camera_qcom(uint8_t *buf, size_t sz) {
    if (!elf_has_str(buf, sz, "CamX") || !elf_has_str(buf, sz, "ConstructDefaultRequestSettings")) {
        xputs(1, "[SKIP] camera_qcom: CamX marker not found (unsupported vendor)\n");
        return -1;
    }
    int edits = 0;
    for (size_t i = 0; i < ARRAY_LEN(CAMQCOM_RULES); i++) {
        size_t h = 0;
        if (find_unique(buf, sz, RULE_BLOB + CAMQCOM_RULES[i].ctx_off, CAMQCOM_RULES[i].ctx_len, &h) == 1 &&
            (h & 3) == 0 && is_in_rx_seg(buf, sz, h, CAMQCOM_RULES[i].ctx_len)) {
            memcpy(buf + h + CAMQCOM_RULES[i].rel, RULE_BLOB + CAMQCOM_RULES[i].rep_off, CAMQCOM_RULES[i].rep_len);
            edits++;
        }
    }
    /* TF20/TF201 Luma-Only temporal blend = 0 (Chroma temporal denoising untouched) */
    size_t pos = 0;
    while ((pos = find_bytes(buf, sz, TF201_LUMA_CTX, sizeof(TF201_LUMA_CTX), pos)) != (size_t)-1) {
        if ((pos & 3) == 0 && is_in_rx_seg(buf, sz, pos, sizeof(TF201_LUMA_CTX))) {
            memcpy(buf + pos + 16, TF201_LUMA_REP, sizeof(TF201_LUMA_REP));
            edits++;
        }
        pos += sizeof(TF201_LUMA_CTX);
    }
    /* ANR101 Luma-Only spatial bypass RX cave (Chroma CLPF + CNR 100% active) */
    size_t hpos = 0;
    if (find_unique(buf, sz, ANR101_HOOK_CTX, sizeof(ANR101_HOOK_CTX), &hpos) == 1 &&
        (hpos & 3) == 0 && is_in_rx_seg(buf, sz, hpos, sizeof(ANR101_HOOK_CTX))) {
        uint64_t hook_pc = hpos + 12, cave = 0;
        if (alloc_rx_cave_for(buf, sz, hook_pc, 16, 0x50, &cave) == 0) {
            static const uint32_t anr_cave[15] = {
                0xf9402fe8u, /* ldr  x8, [sp, #0x58] */
                0xf9401908u, /* ldr  x8, [x8, #0x30] */
                0xb4000188u, /* cbz  x8, done        */
                0xf94037e9u, /* ldr  x9, [sp, #0x68] */
                0xb4000149u, /* cbz  x9, done        */
                0xb941150au, /* ldr  w10, [x8, #0x114] */
                0x321f014au, /* orr  w10, w10, #0x2  */
                0xb901150au, /* str  w10, [x8, #0x114] */
                0xb940b90au, /* ldr  w10, [x8, #0xb8]  */
                0x3200014au, /* orr  w10, w10, #0x1  */
                0xb900b90au, /* str  w10, [x8, #0xb8]  */
                0x91147108u, /* add  x8, x8, #0x51c  */
                0xf1000529u, /* subs x9, x9, #1      */
                0x54ffff01u, /* b.ne loop            */
                0x2a1f03f3u  /* mov  w19, wzr (done) */
            };
            for (int i = 0; i < 15; i++) wr32(buf + cave + (uint64_t)i * 4, anr_cave[i]);
            wr32(buf + cave + 60, arm64_b(cave + 60, hook_pc + 4, 0));
            wr32(buf + hook_pc, arm64_b(hook_pc, cave, 0));
            edits++;
        }
    }
    return edits;
}

static int patch_hal_mcreprocrt(uint8_t *buf, size_t sz) {
    size_t h = 0;
    if (!elf_has_str(buf, sz, "MultiCameraSATWithQSAT") ||
        find_unique(buf, sz, RULE_BLOB + MCREPROCRT_RULES[0].ctx_off, MCREPROCRT_RULES[0].ctx_len, &h) != 1 ||
        (h & 3) != 0 || !is_in_rx_seg(buf, sz, h, MCREPROCRT_RULES[0].ctx_len)) {
        return 0;
    }
    uint64_t pc_va = elf_file_to_vaddr(buf, sz, h + 8), page = 0;
    uint32_t rd = 0, i_add = rd32(buf + h + 12);
    if (pc_va == (uint64_t)-1 || !arm64_dec_adrp(pc_va, rd32(buf + h + 8), &page, &rd) ||
        (i_add & 0xFFC00000u) != 0x91000000u) {
        return 0;
    }
    uint32_t old_imm12 = (i_add >> 10) & 0xFFFu;
    uint32_t new_add   = (i_add & ~(0xFFu << 8)) | ((uint32_t)RULE_BLOB[MCREPROCRT_RULES[0].rep_off] << 8);
    uint32_t new_imm12 = (new_add >> 10) & 0xFFFu;
    if (!elf_cstr_eq(buf, sz, page + old_imm12, "MultiCameraVideoSATNoGPU") ||
        !elf_cstr_eq(buf, sz, page + new_imm12, "MultiCameraSATWithQSAT")) {
        return 0;
    }
    memcpy(buf + h + MCREPROCRT_RULES[0].rel, RULE_BLOB + MCREPROCRT_RULES[0].rep_off, MCREPROCRT_RULES[0].rep_len);
    return 1;
}

static int patch_hal_feature2_generic(uint8_t *buf, size_t sz) {
    size_t hpos = 0;
    if (!elf_has_str(buf, sz, "Feature2Generic") ||
        find_unique(buf, sz, GEN_HOOK_CTX, sizeof(GEN_HOOK_CTX), &hpos) != 1 ||
        (hpos & 3) != 0 || !is_in_rx_seg(buf, sz, hpos, sizeof(GEN_HOOK_CTX))) {
        return 0;
    }
    uint64_t hook_pc = hpos + 12, cave = 0;
    if (alloc_rx_cave_for(buf, sz, hook_pc, 16, 0x20, &cave) != 0) return 0;
    static const uint8_t cbody[16] = {
        0xea,0x03,0x08,0xaa, 0x28,0x01,0x40,0xb9,
        0xaa,0x1e,0xc3,0xb4, 0x56,0x21,0x5e,0xf9
    };
    memcpy(buf + cave, cbody, 16);
    wr32(buf + cave + 16, arm64_b(cave + 16, hook_pc + 8, 0));
    wr32(buf + hook_pc, arm64_b(hook_pc, cave, 0));
    return 1;
}

/* Universal atomic 15/15 chi_override patcher (HyperOS 1/2/3/4, OOS16, Lunaris/AOSP). */
static int patch_hal_chi_override(uint8_t *buf, size_t sz) {
    if (!elf_has_str(buf, sz, "chi_hal_override_entry")) return 0;

    size_t z65_hit = 0, fixed_hits[4];
    if (find_unique(buf, sz, CHI_ZOOM65_CTX, sizeof(CHI_ZOOM65_CTX), &z65_hit) != 1) return 0;
    for (size_t i = 0; i < ARRAY_LEN(CHI_FIXED_RULES); i++) {
        if (find_unique(buf, sz, RULE_BLOB + CHI_FIXED_RULES[i].ctx_off, CHI_FIXED_RULES[i].ctx_len, &fixed_hits[i]) != 1)
            return 0;
    }

    /* HOOK1 + RULE12: CameraIdXiaomi logical camera list (BL immediates masked) */
    static const uint32_t h1_words[13] = {
        0x52800801u, 0xf9000a9fu, 0x94000000u, 0x528007f6u,
        0xaa0003f5u, 0x52800081u, 0xb9002016u, 0x52800020u,
        0x94000000u, 0xf90016a0u, 0xaa1403e8u, 0xb8004416u, 0xa90302a0u
    };
    static const uint32_t h1_masks[13] = {
        0xffffffffu, 0xffffffffu, 0xfc000000u, 0xffffffffu,
        0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu,
        0xfc000000u, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu
    };
    /* HOOK2: FinalizeFeatureInstances SWRemosaic guard ([x29, #-0xe0], BL masked) */
    static const uint32_t h2_words[9] = {
        0xf85203a0u, 0xd10243a2u, 0xa9377fbfu, 0xf81803bfu,
        0x94000000u, 0x2a0003f6u, 0x34000120u, 0xa941d353u, 0xeb13029fu
    };
    static const uint32_t h2_masks[9] = {
        0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu,
        0xfc000000u, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu
    };

    uint64_t h1_blk = 0, h2_blk = 0, pair_arr_va = 0;
    ChiMcBlock mc;
    memset(&mc, 0, sizeof(mc));
    if (find_masked_unique_rx(buf, sz, h1_words, h1_masks, 13, &h1_blk) != 1 ||
        find_masked_unique_rx(buf, sz, h2_words, h2_masks, 9, &h2_blk) != 1 ||
        find_garnet_mc_block(buf, sz, &mc) != 1 ||
        find_pair_arr(buf, sz, &pair_arr_va) != 1) {
        return 0;
    }

    uint64_t pc_tbl_adrp_va = elf_file_to_vaddr(buf, sz, mc.pc_tbl_adrp);
    uint64_t pc_b0_va       = elf_file_to_vaddr(buf, sz, mc.pc_b0);
    uint32_t insn_tbl_adrp = 0, insn_b0_adrp = 0;
    uint64_t h1_pc = h1_blk + 44, h2_pc = h2_blk + 20, cave_base = 0;
    if (pc_tbl_adrp_va == (uint64_t)-1 || pc_b0_va == (uint64_t)-1 ||
        !arm64_enc_adrp(pc_tbl_adrp_va, mc.t_vt_va, mc.tbl_reg, &insn_tbl_adrp) ||
        !arm64_enc_adrp(pc_b0_va, pair_arr_va, 8, &insn_b0_adrp) ||
        alloc_rx_cave_for(buf, sz, h1_pc, 32, 0x40, &cave_base) != 0) {
        return 0;
    }
    uint64_t cave2 = cave_base + 0x10;
    uint64_t d2 = (cave2 > h2_pc) ? (cave2 - h2_pc) : (h2_pc - cave2);
    if (d2 >= 0x07F00000ULL) return 0;

    /* Commit atomic 15/15 chi_override patches */
    memcpy(buf + z65_hit + 8, CHI_ZOOM65_REP, 3);
    for (size_t i = 0; i < ARRAY_LEN(CHI_FIXED_RULES); i++) {
        memcpy(buf + fixed_hits[i] + CHI_FIXED_RULES[i].rel,
               RULE_BLOB + CHI_FIXED_RULES[i].rep_off, CHI_FIXED_RULES[i].rep_len);
    }
    wr32(buf + mc.pc_strd,     0xBD000800u | (mc.orig_strd & 0x1Fu));
    wr32(buf + mc.pc_tbl_adrp, insn_tbl_adrp);
    wr32(buf + mc.pc_tbl_add,  arm64_enc_add12(mc.tbl_reg, mc.tbl_reg, (uint32_t)(mc.t_vt_va & 0xFFFu)));
    wr32(buf + mc.pc_mov8,     0x52800201u); /* mov w1, #0x10 */
    wr32(buf + mc.pc_b0,       insn_b0_adrp);
    wr32(buf + mc.pc_b1,       arm64_enc_add12(8, 8, (uint32_t)(pair_arr_va & 0xFFFu)));
    wr32(buf + mc.pc_b2,       0x3DC00100u); /* ldr q0, [x8] */
    wr32(buf + mc.pc_str8,     0x3C810400u); /* str q0, [x0], #16 */
    wr32(buf + h1_blk + 12,    0x528007d6u); /* mov w22, #0x3e */

    static const uint8_t c1[12] = {
        0xe9,0x07,0x80,0x52, 0x09,0x44,0x00,0xb8, 0xc0,0x03,0x5f,0xd6
    };
    memcpy(buf + cave_base, c1, sizeof(c1));
    wr32(buf + h1_pc, arm64_b(h1_pc, cave_base, 1));

    static const uint8_t c2[28] = {
        0xc0,0x00,0x00,0x34, 0xa8,0x03,0x52,0xf8,
        0x08,0x49,0x41,0xf9, 0x08,0x01,0x40,0xb9,
        0x1f,0x89,0x00,0x71, 0xe0,0x03,0x80,0x1a,
        0xf6,0x03,0x00,0x2a
    };
    memcpy(buf + cave2, c2, sizeof(c2));
    wr32(buf + cave2 + 28, arm64_b(cave2 + 28, h2_pc + 4, 0));
    wr32(buf + h2_pc, arm64_b(h2_pc, cave2, 0));
    return 15;
}

static int cmd_patch_hal(const char *kind, const char *src, const char *dst) {
    size_t sz = 0;
    uint8_t *buf = read_file_all(src, &sz);
    if (!buf) { xputs(2, "[ERR] cannot read HAL file\n"); return 1; }
    if (!validate_elf64_aarch64(buf, sz)) {
        free_buf(buf, sz + 4096);
        xputs(2, "[SKIP] "); xputs(2, kind); xputs(2, ": not a valid ELF64 AArch64 binary\n");
        return 3;
    }

    int edits = 0;
    if      (xstrcmp(kind, "camera_qcom") == 0)      edits = patch_hal_camera_qcom(buf, sz);
    else if (xstrcmp(kind, "mcreprocrt") == 0)       edits = patch_hal_mcreprocrt(buf, sz);
    else if (xstrcmp(kind, "feature2_generic") == 0) edits = patch_hal_feature2_generic(buf, sz);
    else if (xstrcmp(kind, "chi_override") == 0)     edits = patch_hal_chi_override(buf, sz);
    else {
        free_buf(buf, sz + 4096);
        xputs(2, "ERR: unknown HAL kind\n");
        return 2;
    }

    if (edits <= 0) {
        free_buf(buf, sz + 4096);
        if (edits == 0) {
            xputs(1, "[SKIP] "); xputs(1, kind); xputs(1, ": 0 safe matches (keeping stock)\n");
        }
        return 3;
    }

    int rc = write_file_all(dst, buf, sz);
    free_buf(buf, sz + 4096);
    if (rc != 0) { xputs(2, "ERR: failed to write HAL output\n"); return 1; }
    xputs(1, "[OK] "); xputs(1, kind); xputs(1, ": applied "); xput_u32(1, (uint32_t)edits); xputs(1, " edits\n");
    return 0;
}

/* =========================================================================
 * 5b. Chromatix7 triglGAV1 AWB Gain-Adjust Patcher (OV64B40 Wide & IMX355 UW)
 * ========================================================================= */
typedef struct {
    char kind[8];
    size_t align_step;
    size_t min_cnt;
    double r_max, b_min;
    int check_cct_tail;
    double r_mul, b_mul;
} TunedWbSpec;

static const TunedWbSpec TUNED_WB_SPECS[] = {
    /* OV64B40 Main Cam: remove purple tint cleanly while keeping subtle natural warmth */
    { "wide",  4, 150, 1.25, 0.75, 0, 0.980, 0.983 },
    /* IMX355 UltraWide Global: matched to OV64B40 Main Cam warmth & tint */
    { "ultra", 1, 200, 1.15, 0.80, 1, 0.998, 0.954 },
};

static float rdf32(const uint8_t *p) {
    uint32_t u = rd32(p);
    float f;
    memcpy(&f, &u, 4);
    return f;
}

static void wrf32(uint8_t *p, float f) {
    uint32_t u;
    memcpy(&u, &f, 4);
    wr32(p, u);
}

static int find_trigl_gav1_table(const uint8_t *buf, size_t sz, const TunedWbSpec *sp,
                                 size_t *out_off, size_t *out_cnt, size_t *out_cct_off) {
    static const uint8_t ONE_F[4] = {0x00, 0x00, 0x80, 0x3f};
    size_t hits = 0, best_off = 0, best_cnt = 0, best_cct = 0;
    size_t pos = 0x100000;

    while (pos + 20 * sp->min_cnt + 4 <= sz) {
        size_t f1 = find_bytes(buf, sz - 4, ONE_F, 4, pos + 12);
        if (f1 == (size_t)-1) break;
        size_t off = f1 - 12;
        if (sp->align_step == 4 && (off & 3) != 0) {
            pos = off + (4 - (off & 3));
            continue;
        }
        size_t cnt = 0, non_unity = 0, j = off;
        while (j + 20 <= sz - 4) {
            float s = rdf32(buf + j), e = rdf32(buf + j + 4);
            float r = rdf32(buf + j + 8), g = rdf32(buf + j + 12), b = rdf32(buf + j + 16);
            if (s >= 0.0f && s <= e && e <= 1000.0f && !(s == 0.0f && e == 0.0f) &&
                g == 1.0f && r >= 0.95f && (double)r <= sp->r_max &&
                (double)b >= sp->b_min && b <= 1.08f) {
                if (r != 1.0f || b != 1.0f) non_unity++;
                cnt++;
                j += 20;
            } else {
                break;
            }
        }
        if (cnt >= sp->min_cnt && non_unity >= 50) {
            size_t cct_off = 0;
            int tail_ok = !sp->check_cct_tail;
            if (sp->check_cct_tail && j + 36 <= sz - 4) {
                float s2 = rdf32(buf + j + 16), e2 = rdf32(buf + j + 20), g2 = rdf32(buf + j + 28);
                if (s2 == 1500.0f && e2 == 12000.0f && g2 == 1.0f) {
                    cct_off = j + 16;
                    tail_ok = 1;
                }
            }
            if (tail_ok) {
                hits++;
                best_off = off; best_cnt = cnt; best_cct = cct_off;
            }
            pos = j + 20;
            continue;
        }
        pos = off + sp->align_step;
    }
    if (hits != 1) return 0;
    *out_off = best_off;
    *out_cnt = best_cnt;
    *out_cct_off = best_cct;
    return 1;
}

static int cmd_patch_tuned(const char *kind, const char *src, const char *dst) {
    const TunedWbSpec *sp = (const TunedWbSpec *)0;
    for (size_t i = 0; i < ARRAY_LEN(TUNED_WB_SPECS); i++) {
        if (xstrcmp(kind, TUNED_WB_SPECS[i].kind) == 0) { sp = &TUNED_WB_SPECS[i]; break; }
    }
    if (!sp) { xputs(2, "ERR: unknown tuned kind\n"); return 2; }

    size_t sz = 0;
    uint8_t *buf = read_file_all(src, &sz);
    if (!buf) { xputs(2, "[ERR] cannot read tuned file\n"); return 1; }
    if (sz < 0x200000 || memcmp(buf, "QTI Chromatix Header", 20) != 0 ||
        crc32_calc(buf, sz - 4) != rd32(buf + sz - 4)) {
        free_buf(buf, sz + 4096);
        xputs(1, "[SKIP] "); xputs(1, kind); xputs(1, ": invalid Chromatix header/CRC32 (keeping stock)\n");
        return 3;
    }

    size_t off = 0, cnt = 0, cct_off = 0;
    if (!find_trigl_gav1_table(buf, sz, sp, &off, &cnt, &cct_off)) {
        free_buf(buf, sz + 4096);
        xputs(1, "[SKIP] "); xputs(1, kind); xputs(1, ": triglGAV1 WB table not uniquely matched (keeping stock)\n");
        return 3;
    }

    size_t patched = 0;
    for (size_t k = 0; k < cnt; k++) {
        size_t eoff = off + k * 20;
        wrf32(buf + eoff + 8,  (float)((double)rdf32(buf + eoff + 8)  * sp->r_mul));
        wrf32(buf + eoff + 16, (float)((double)rdf32(buf + eoff + 16) * sp->b_mul));
        patched++;
    }
    if (cct_off) {
        wrf32(buf + cct_off + 8,  (float)((double)rdf32(buf + cct_off + 8)  * sp->r_mul));
        wrf32(buf + cct_off + 16, (float)((double)rdf32(buf + cct_off + 16) * sp->b_mul));
        patched++;
    }
    wr32(buf + sz - 4, crc32_calc(buf, sz - 4));

    int rc = write_file_all(dst, buf, sz);
    free_buf(buf, sz + 4096);
    if (rc != 0) { xputs(2, "ERR: failed to write tuned output\n"); return 1; }
    xputs(1, "[OK] tuned "); xputs(1, kind); xputs(1, ": patched ");
    xput_u32(1, (uint32_t)patched); xputs(1, " triglGAV1 entries\n");
    return 0;
}

/* =========================================================================
 * 6. DEX Parser, Tier-1 Method Rules & Tier-2 Universal Dynamic DEX Patcher
 * ========================================================================= */
typedef struct {
    uint32_t str_cnt, str_off;
    uint32_t type_cnt, type_off;
    uint32_t meth_cnt, meth_off;
    uint32_t cls_sz, cls_off;
} DexView;

/* Dalvik instruction widths in 16-bit code units (zero relocations) */
static const uint8_t DEX_INSN_W[256] = {
    1,1,2,3,1,2,3,1,2,3,1,1,1,1,1,1, 1,1,1,2,3,2,2,3,5,2,2,3,2,1,1,2,
    2,1,2,2,3,3,3,1,1,2,3,3,3,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,
    1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3, 3,3,3,1,3,3,3,3,3,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,4,4,2,2,2,2,
};

static int dex_parse_view(const uint8_t *dex, size_t dlen, DexView *dv) {
    if (dlen < 112 || memcmp(dex, "dex\n", 4) != 0) return 0;
    dv->str_cnt  = rd32(dex + 56); dv->str_off  = rd32(dex + 60);
    dv->type_cnt = rd32(dex + 64); dv->type_off = rd32(dex + 68);
    dv->meth_cnt = rd32(dex + 88); dv->meth_off = rd32(dex + 92);
    dv->cls_sz   = rd32(dex + 96); dv->cls_off  = rd32(dex + 100);
    return 1;
}

static uint32_t read_uleb128(const uint8_t *b, size_t len, size_t *pos) {
    uint32_t res = 0, shift = 0;
    while (*pos < len) {
        uint8_t v = b[(*pos)++];
        res |= (uint32_t)(v & 0x7Fu) << shift;
        if (!(v & 0x80u)) break;
        shift += 7;
    }
    return res;
}

static const uint8_t *dex_get_str(const uint8_t *dex, size_t dlen, const DexView *dv, uint32_t idx, size_t *out_len) {
    if (idx >= dv->str_cnt) return (const uint8_t *)0;
    size_t so = (size_t)dv->str_off + (size_t)idx * 4;
    if (so + 4 > dlen) return (const uint8_t *)0;
    size_t off = rd32(dex + so);
    if (off >= dlen) return (const uint8_t *)0;
    read_uleb128(dex, dlen, &off);
    size_t p = off;
    while (p < dlen && dex[p] != 0) p++;
    *out_len = p - off;
    return dex + off;
}

static int dex_str_eq(const uint8_t *dex, size_t dlen, const DexView *dv, uint32_t idx, const char *s) {
    size_t len = 0;
    const uint8_t *p = dex_get_str(dex, dlen, dv, idx, &len);
    size_t slen = xstrlen(s);
    return (p && len == slen && memcmp(p, s, slen) == 0);
}

static uint32_t dex_find_str_id(const uint8_t *dex, size_t dlen, const DexView *dv, const char *s) {
    size_t slen = xstrlen(s);
    for (uint32_t i = 0; i < dv->str_cnt; i++) {
        size_t len = 0;
        const uint8_t *p = dex_get_str(dex, dlen, dv, i, &len);
        if (p && len == slen && memcmp(p, s, slen) == 0) return i;
    }
    return 0xFFFFFFFFu;
}

static uint32_t dex_find_meth_id(const uint8_t *dex, size_t dlen, const DexView *dv, const char *cls, const char *mth) {
    uint32_t cls_sid = dex_find_str_id(dex, dlen, dv, cls);
    uint32_t mth_sid = dex_find_str_id(dex, dlen, dv, mth);
    if (cls_sid == 0xFFFFFFFFu || mth_sid == 0xFFFFFFFFu) return 0xFFFFFFFFu;
    uint32_t cls_tid = 0xFFFFFFFFu;
    for (uint32_t t = 0; t < dv->type_cnt; t++) {
        if ((size_t)dv->type_off + (size_t)t * 4 + 4 > dlen) break;
        if (rd32(dex + dv->type_off + (size_t)t * 4) == cls_sid) { cls_tid = t; break; }
    }
    if (cls_tid == 0xFFFFFFFFu) return 0xFFFFFFFFu;
    for (uint32_t m = 0; m < dv->meth_cnt; m++) {
        size_t mo = (size_t)dv->meth_off + (size_t)m * 8;
        if (mo + 8 > dlen) break;
        if (rd16(dex + mo) == cls_tid && rd32(dex + mo + 4) == mth_sid) return m;
    }
    return 0xFFFFFFFFu;
}

static const uint8_t *dex_class_type_str(const uint8_t *dex, size_t dlen, const DexView *dv,
                                         uint32_t cidx, size_t *out_len) {
    if ((size_t)dv->type_off + (size_t)cidx * 4 + 4 > dlen) return (const uint8_t *)0;
    return dex_get_str(dex, dlen, dv, rd32(dex + dv->type_off + (size_t)cidx * 4), out_len);
}

static const uint8_t *dex_method_name_str(const uint8_t *dex, size_t dlen, const DexView *dv,
                                          uint32_t midx, size_t *out_len) {
    if ((size_t)dv->meth_off + (size_t)midx * 8 + 8 > dlen) return (const uint8_t *)0;
    return dex_get_str(dex, dlen, dv, rd32(dex + dv->meth_off + (size_t)midx * 8 + 4), out_len);
}

static void dex_skip_fields(const uint8_t *dex, size_t dlen, size_t *p, uint32_t *out_dm, uint32_t *out_vm) {
    uint32_t sf = read_uleb128(dex, dlen, p);
    uint32_t if_ = read_uleb128(dex, dlen, p);
    *out_dm = read_uleb128(dex, dlen, p);
    *out_vm = read_uleb128(dex, dlen, p);
    for (uint32_t k = 0; k < sf + if_; k++) {
        read_uleb128(dex, dlen, p);
        read_uleb128(dex, dlen, p);
    }
}

static size_t dex_insn_step(const uint8_t *code, size_t clen, size_t i) {
    uint8_t op = code[i];
    if (op == 0x00 && i + 4 <= clen && (code[i + 1] == 1 || code[i + 1] == 2 || code[i + 1] == 3)) {
        uint8_t id = code[i + 1];
        if (id == 1) return ((size_t)rd16(code + i + 2) * 2 + 4) * 2;
        if (id == 2) return ((size_t)rd16(code + i + 2) * 4 + 2) * 2;
        if (i + 8 <= clen) {
            uint32_t ew = rd16(code + i + 2), cnt = rd32(code + i + 4);
            return (((size_t)cnt * ew + 1) / 2 + 4) * 2;
        }
    }
    return (size_t)DEX_INSN_W[op] * 2;
}

static int dex_code_has_str(const uint8_t *code, size_t clen, uint32_t sid, size_t *out_pos) {
    if (sid == 0xFFFFFFFFu) return 0;
    size_t i = 0;
    while (i < clen) {
        size_t st = dex_insn_step(code, clen, i);
        if (st == 0 || i + st > clen) break;
        if ((code[i] == 0x1a && st == 4 && rd16(code + i + 2) == sid) ||
            (code[i] == 0x1b && st == 6 && rd32(code + i + 2) == sid)) {
            if (out_pos) *out_pos = i;
            return 1;
        }
        i += st;
    }
    return 0;
}

static int dex_code_has_invoke(const uint8_t *code, size_t clen, uint32_t mid, size_t *out_pos) {
    if (mid == 0xFFFFFFFFu) return 0;
    size_t i = 0;
    while (i < clen) {
        size_t st = dex_insn_step(code, clen, i);
        if (st == 0 || i + st > clen) break;
        uint8_t op = code[i];
        if (((op >= 0x6e && op <= 0x72) || (op >= 0x74 && op <= 0x78)) && st == 6 && rd16(code + i + 2) == mid) {
            if (out_pos) *out_pos = i;
            return 1;
        }
        i += st;
    }
    return 0;
}

static int apply_dex_meth_rules(uint8_t *dex, size_t dlen) {
    DexView dv;
    if (!dex_parse_view(dex, dlen, &dv)) return 0;
    int edits = 0;

    for (uint32_t i = 0; i < dv.cls_sz; i++) {
        size_t co = (size_t)dv.cls_off + (size_t)i * 32;
        if (co + 32 > dlen) break;
        uint32_t cidx = rd32(dex + co), cdata = rd32(dex + co + 24);
        if (!cdata) continue;
        size_t clen = 0;
        const uint8_t *cstr = dex_class_type_str(dex, dlen, &dv, cidx, &clen);
        if (!cstr) continue;

        int match_cls = 0;
        for (size_t r = 0; r < ARRAY_LEN(DEX_METH_RULES); r++) {
            if (clen == DEX_METH_RULES[r].cls_len &&
                memcmp(cstr, RULE_BLOB + DEX_METH_RULES[r].cls_off, clen) == 0) {
                match_cls = 1; break;
            }
        }
        if (!match_cls) continue;

        size_t p = cdata;
        uint32_t dm = 0, vm = 0;
        dex_skip_fields(dex, dlen, &p, &dm, &vm);
        for (int grp = 0; grp < 2; grp++) {
            uint32_t cnt = (grp == 0) ? dm : vm, midx = 0;
            for (uint32_t k = 0; k < cnt; k++) {
                midx += read_uleb128(dex, dlen, &p);
                read_uleb128(dex, dlen, &p);
                uint32_t code_off = read_uleb128(dex, dlen, &p);
                if (!code_off) continue;
                size_t mlen = 0;
                const uint8_t *mstr = dex_method_name_str(dex, dlen, &dv, midx, &mlen);
                if (!mstr) continue;
                for (size_t r = 0; r < ARRAY_LEN(DEX_METH_RULES); r++) {
                    if (clen == DEX_METH_RULES[r].cls_len && mlen == DEX_METH_RULES[r].mth_len &&
                        memcmp(cstr, RULE_BLOB + DEX_METH_RULES[r].cls_off, clen) == 0 &&
                        memcmp(mstr, RULE_BLOB + DEX_METH_RULES[r].mth_off, mlen) == 0) {
                        size_t off = (size_t)code_off + 16 + DEX_METH_RULES[r].rel;
                        if (off + DEX_METH_RULES[r].len <= dlen &&
                            memcmp(dex + off, RULE_BLOB + DEX_METH_RULES[r].exp_off, DEX_METH_RULES[r].len) == 0) {
                            memcpy(dex + off, RULE_BLOB + DEX_METH_RULES[r].rep_off, DEX_METH_RULES[r].len);
                            edits++;
                        }
                    }
                }
            }
        }
    }
    return edits;
}

/* ProGuard-independent dynamic DEX patcher (HyperOS 4 & universal MiuiCamera support).
 * Preserves native AlgoUp / LocalParallel / Garnet device features while fixing SAT roleId,
 * VideoSAT/PhotoSAT enablement, anti-oversharpening, 0.6x UW LDC, and cooler video bitrates. */
static int apply_dex_dynamic(uint8_t *dex, size_t dlen, int *out_sat_anchors) {
    DexView dv;
    if (!dex_parse_view(dex, dlen, &dv)) return 0;
    int edits = 0;

    uint32_t sid_getsat  = dex_find_str_id(dex, dlen, &dv, "Warning: getSATCameraId(): #init() failed.");
    uint32_t sid_hassat  = dex_find_str_id(dex, dlen, &dv, "Warning: hasSATCamera(): #init() failed.");
    uint32_t sid_vsatdbg = dex_find_str_id(dex, dlen, &dv, "supportVideoSAT: debug prop force true");
    uint32_t sid_satldc  = dex_find_str_id(dex, dlen, &dv, "setSATUltraWideLDC: ");
    uint32_t sid_h264    = dex_find_str_id(dex, dlen, &dv, "setupRecorder: H264 bitrate = ");
    uint32_t sid_hevc    = dex_find_str_id(dex, dlen, &dv, "HEVC: no pre-defined bitrate for ");
    uint32_t mid_sharp   = dex_find_meth_id(dex, dlen, &dv, "Lcom/android/camera2/compat/MiCameraCompatBaseImpl;", "applySharpness");
    uint32_t mid_uwldc   = dex_find_meth_id(dex, dlen, &dv, "Lcom/android/camera2/compat/MiCameraCompatBaseImpl;", "applyUltraWideLDC");

    uint32_t sharp_def_mid = 0xFFFFFFFFu, sat_cls_idx = 0xFFFFFFFFu;
    uint16_t sat_dbg_fld = 0;
    uint8_t  have_sat_dbg = 0;
    uint32_t u0_coff = 0, h0_coff = 0;
    int u0_cnt = 0, h0_cnt = 0;

    /* Pass 1: single-method string/API patches & capture cross-method anchors */
    for (uint32_t i = 0; i < dv.cls_sz; i++) {
        size_t co = (size_t)dv.cls_off + (size_t)i * 32;
        if (co + 32 > dlen) break;
        uint32_t cidx = rd32(dex + co), cdata = rd32(dex + co + 24);
        if (!cdata || (size_t)dv.type_off + (size_t)cidx * 4 + 4 > dlen) continue;
        uint32_t csidx = rd32(dex + dv.type_off + (size_t)cidx * 4);
        int is_cam2mod = dex_str_eq(dex, dlen, &dv, csidx, "Lcom/android/camera/module/Camera2Module;");
        int is_basemod = dex_str_eq(dex, dlen, &dv, csidx, "Lcom/android/camera/module/BaseModule;");

        size_t p = cdata;
        uint32_t dm = 0, vm = 0;
        dex_skip_fields(dex, dlen, &p, &dm, &vm);
        for (int grp = 0; grp < 2; grp++) {
            uint32_t cnt = (grp == 0) ? dm : vm, midx = 0;
            for (uint32_t k = 0; k < cnt; k++) {
                midx += read_uleb128(dex, dlen, &p);
                read_uleb128(dex, dlen, &p);
                uint32_t coff = read_uleb128(dex, dlen, &p);
                if (!coff || (size_t)coff + 16 > dlen) continue;
                uint16_t tries = rd16(dex + coff + 6);
                size_t clen = (size_t)rd32(dex + coff + 12) * 2;
                if ((size_t)coff + 16 + clen > dlen) continue;
                uint8_t *code = dex + coff + 16;

                /* 1. getSATCameraId(): roleId 60 (0x3c) -> 62 (0x3e) */
                if (sid_getsat != 0xFFFFFFFFu && clen <= 160 && dex_code_has_str(code, clen, sid_getsat, 0)) {
                    for (size_t j = 0; j + 10 <= clen; j += 2) {
                        if (code[j] == 0x13 && code[j + 2] == 0x3c && code[j + 3] == 0x00 &&
                            code[j + 4] == 0x6e && code[j + 5] == 0x30) {
                            code[j + 2] = 0x3e;
                            edits++; (*out_sat_anchors)++; break;
                        }
                    }
                }
                /* 2. hasSATCamera(): roleId 60 (0x3c) -> 62 (0x3e) */
                if (sid_hassat != 0xFFFFFFFFu && clen <= 160 && dex_code_has_str(code, clen, sid_hassat, 0)) {
                    for (size_t j = 0; j + 10 <= clen; j += 2) {
                        if (code[j] == 0x13 && code[j + 2] == 0x3c && code[j + 3] == 0x00 &&
                            code[j + 4] == 0x6e && code[j + 5] == 0x20) {
                            code[j + 2] = 0x3e;
                            edits++; (*out_sat_anchors)++; break;
                        }
                    }
                }
                /* 3. DataItemFeature.isSupportVideoSAT() -> true */
                if (sid_vsatdbg != 0xFFFFFFFFu && clen >= 24 && clen <= 120 &&
                    code[0] == 0x60 && code[1] == 0x00 && dex_code_has_str(code, clen, sid_vsatdbg, 0)) {
                    sat_cls_idx = i;
                    sat_dbg_fld = rd16(code + 2);
                    have_sat_dbg = 1;
                    code[0] = 0x12; code[1] = 0x10; code[2] = 0x0f; code[3] = 0x00;
                    edits++; (*out_sat_anchors)++;
                }
                /* 7a. Camera2Module.updateSharpness -> capture default sharpness static callee */
                if (is_cam2mod && clen >= 24 && clen <= 80 && (size_t)dv.meth_off + (size_t)midx * 8 + 8 <= dlen) {
                    uint32_t mnidx = rd32(dex + dv.meth_off + (size_t)midx * 8 + 4);
                    if (dex_str_eq(dex, dlen, &dv, mnidx, "updateSharpness")) {
                        size_t j = 0;
                        while (j < clen) {
                            size_t st = dex_insn_step(code, clen, j);
                            if (st == 0 || j + st > clen) break;
                            if (code[j] == 0x71 && code[j + 1] == 0x10 && st == 6) {
                                sharp_def_mid = rd16(code + j + 2); break;
                            }
                            j += st;
                        }
                    }
                }
                /* 8a. Candidate applySharpness wrapper (len == 32) */
                if (mid_sharp != 0xFFFFFFFFu && clen == 32 &&
                    code[0] == 0x39 && code[1] == 0x01 && code[2] == 0x03 && code[3] == 0x00 &&
                    code[4] == 0x0e && code[5] == 0x00 && code[6] == 0x52 && code[7] == 0x22 &&
                    code[10] == 0x12 && code[11] == 0xf0 && code[12] == 0x32 && code[13] == 0x02 &&
                    code[14] == 0x09 && code[15] == 0x00 && dex_code_has_invoke(code, clen, mid_sharp, 0)) {
                    u0_coff = coff; u0_cnt++;
                }
                /* 9a. Candidate applyUltraWideLDC wrapper (len == 54) */
                if (mid_uwldc != 0xFFFFFFFFu && clen == 54 &&
                    code[28] == 0x0a && code[30] == 0x38 && code[29] == code[31] &&
                    code[32] == 0x0b && code[33] == 0x00 && dex_code_has_invoke(code, clen, mid_uwldc, 0)) {
                    h0_coff = coff; h0_cnt++;
                }
                /* 10. BaseModule.updateUltraWideLDC: bypass B0 check before setSATUltraWideLDC */
                if (is_basemod && sid_satldc != 0xFFFFFFFFu && (size_t)dv.meth_off + (size_t)midx * 8 + 8 <= dlen) {
                    uint32_t mnidx = rd32(dex + dv.meth_off + (size_t)midx * 8 + 4);
                    size_t spos = 0;
                    if (dex_str_eq(dex, dlen, &dv, mnidx, "updateUltraWideLDC") &&
                        dex_code_has_str(code, clen, sid_satldc, &spos) && spos >= 24) {
                        size_t lo = (spos > 64) ? (spos - 64) : 0;
                        for (size_t j = spos - 6; j >= lo; j -= 2) {
                            if (code[j] == 0x0a && code[j + 1] == 0x00 &&
                                code[j + 2] == 0x38 && code[j + 3] == 0x00 && code[j + 5] == 0x00) {
                                memset(code + j + 2, 0, 4);
                                edits++; break;
                            }
                            if (j < lo + 2) break;
                        }
                    }
                }
                /* 11. H.264 cooler bitrate boost (20M->40M, 50M->60M) */
                size_t hpos = 0;
                if (tries == 0 && sid_h264 != 0xFFFFFFFFu && clen >= 1024 &&
                    dex_code_has_str(code, clen, sid_h264, &hpos) && hpos >= 80 &&
                    code[hpos - 4] == 0x02 && code[hpos - 3] == 0x02 && code[hpos - 1] == 0x00) {
                    uint8_t vsrc = code[hpos - 2];
                    size_t lo = (hpos > 240) ? (hpos - 240) : 0, start = 0;
                    for (size_t j = hpos - 64; j >= lo; j -= 2) {
                        if (code[j] == 0x02 && code[j + 1] == vsrc && code[j + 2] == 0x02 && code[j + 3] == 0x00 &&
                            code[j + 4] == 0x62 && code[j + 5] == 0x02) {
                            start = j + 4; break;
                        }
                        if (j < lo + 2) break;
                    }
                    if (start && hpos > start && (hpos - start) >= 64 && (hpos - start) <= 240) {
                        size_t span = hpos - start;
                        int d1 = (int)(span - 18) / 2;
                        int d2 = (int)(span - 26) / 2;
                        int d3 = (int)(span - 38) / 2;
                        if (d1 > 0 && d1 < 127 && d2 > 0 && d2 < 32767 && d3 > 0 && d3 < 127) {
                            uint8_t patch[44] = {
                                0x02, 0x02, vsrc, 0x00,             /* move/from16 v2, vSrc */
                                0x14, 0x03, 0x40, 0x78, 0x7d, 0x01, /* const v3, 25000000 */
                                0x35, 0x32, 0x05, 0x00,             /* if-ge v2, v3, +5 */
                                0xda, 0x02, 0x02, 0x02,             /* mul-int/lit8 v2, v2, #2 (20M->40M) */
                                0x28, (uint8_t)d1,                  /* goto hpos */
                                0x14, 0x03, 0x00, 0x87, 0x93, 0x03, /* const v3, 60000000 */
                                0x35, 0x32, (uint8_t)(d2 & 0xFF), (uint8_t)(d2 >> 8), /* if-ge v2, v3, hpos */
                                0xda, 0x02, 0x02, 0x06,             /* mul-int/lit8 v2, v2, #6 */
                                0xdb, 0x02, 0x02, 0x05,             /* div-int/lit8 v2, v2, #5 (50M->60M) */
                                0x28, (uint8_t)d3,                  /* goto hpos */
                                0x00, 0x00, 0x00, 0x00
                            };
                            memcpy(code + start, patch, 44);
                            memset(code + start + 44, 0, span - 44);
                            edits++;
                        }
                    }
                }
                /* 12. HEVC cooler bitrate boost 1.33x (15.4M->20.5M, 38.5M->51.3M) */
                if (tries == 0 && sid_hevc != 0xFFFFFFFFu && clen >= 64 && clen <= 600 &&
                    dex_code_has_str(code, clen, sid_hevc, 0)) {
                    uint8_t ret_op = code[clen - 2], r = code[clen - 1];
                    uint8_t rr = (uint8_t)((r << 4) | r);
                    if (ret_op == 0x0f && r < 16 &&
                        code[clen - 6] == 0x52 && code[clen - 5] == rr &&
                        code[clen - 26] == 0x28 && code[clen - 25] == 0x0c) {
                        uint8_t fid0 = code[clen - 4], fid1 = code[clen - 3];
                        size_t j = 0;
                        while (j < clen - 24) {
                            size_t st = dex_insn_step(code, clen, j);
                            if (st == 0 || j + st > clen) break;
                            if (code[j] == 0x28 && st == 2) {
                                int8_t rel = (int8_t)code[j + 1];
                                if ((int)j + (int)rel * 2 == (int)(clen - 2) && rel >= 6) {
                                    code[j + 1] = (uint8_t)(rel - 4);
                                }
                            }
                            j += st;
                        }
                        memset(code + clen - 24, 0, 10);
                        code[clen - 14] = 0x52; code[clen - 13] = rr; code[clen - 12] = fid0; code[clen - 11] = fid1;
                        code[clen - 10] = 0xda; code[clen - 9]  = r;  code[clen - 8]  = r;    code[clen - 7]  = 0x04;
                        code[clen - 6]  = 0xdb; code[clen - 5]  = r;  code[clen - 4]  = r;    code[clen - 3]  = 0x03;
                        edits++;
                    }
                }
            }
        }
    }

    /* Commit unique single-candidate wrappers (8 & 9) */
    if (u0_cnt == 1) {
        static const uint8_t rep_u0[6] = {0x3b, 0x02, 0x03, 0x00, 0x12, 0x02};
        memcpy(dex + u0_coff + 16 + 10, rep_u0, sizeof(rep_u0));
        edits++;
    }
    if (h0_cnt == 1) {
        memset(dex + h0_coff + 16 + 30, 0, 4);
        edits++;
    }

    /* Pass 2: cross-method patches (4. isSupportSAT & 7b. default sharpness callee) */
    if (have_sat_dbg && sat_cls_idx < dv.cls_sz) {
        size_t co = (size_t)dv.cls_off + (size_t)sat_cls_idx * 32;
        size_t p = rd32(dex + co + 24);
        uint32_t dm = 0, vm = 0;
        dex_skip_fields(dex, dlen, &p, &dm, &vm);
        uint32_t sat_coff = 0;
        int sat_cnt = 0;
        for (int grp = 0; grp < 2; grp++) {
            uint32_t cnt = (grp == 0) ? dm : vm;
            for (uint32_t k = 0; k < cnt; k++) {
                read_uleb128(dex, dlen, &p);
                read_uleb128(dex, dlen, &p);
                uint32_t coff = read_uleb128(dex, dlen, &p);
                if (!coff || (size_t)coff + 16 > dlen) continue;
                size_t clen = (size_t)rd32(dex + coff + 12) * 2;
                if (clen == 34 && (size_t)coff + 16 + clen <= dlen) {
                    const uint8_t *code = dex + coff + 16;
                    if (code[0] == 0x54 && code[4] == 0x6e && code[16] == 0x60 && code[17] == 0x01 &&
                        rd16(code + 18) == sat_dbg_fld) {
                        sat_coff = coff; sat_cnt++;
                    }
                }
            }
        }
        if (sat_cnt == 1) {
            uint8_t *code = dex + sat_coff + 16;
            code[0] = 0x12; code[1] = 0x10; code[2] = 0x0f; code[3] = 0x00;
            edits++; (*out_sat_anchors)++;
        }
    }

    if (sharp_def_mid != 0xFFFFFFFFu) {
        for (uint32_t i = 0; i < dv.cls_sz; i++) {
            size_t co = (size_t)dv.cls_off + (size_t)i * 32;
            if (co + 32 > dlen) break;
            uint32_t cdata = rd32(dex + co + 24);
            if (!cdata) continue;
            size_t p = cdata;
            uint32_t dm = 0, vm = 0;
            dex_skip_fields(dex, dlen, &p, &dm, &vm);
            for (int grp = 0; grp < 2; grp++) {
                uint32_t cnt = (grp == 0) ? dm : vm, midx = 0;
                for (uint32_t k = 0; k < cnt; k++) {
                    midx += read_uleb128(dex, dlen, &p);
                    read_uleb128(dex, dlen, &p);
                    uint32_t coff = read_uleb128(dex, dlen, &p);
                    if (midx == sharp_def_mid && coff && (size_t)coff + 32 <= dlen) {
                        uint8_t *code = dex + coff + 16;
                        if (code[0] == 0x12 && code[1] == 0x10 && code[2] == 0x71 && code[3] == 0x10 &&
                            code[8] == 0x0c && code[9] == 0x01 && code[10] == 0x39 && code[11] == 0x04 &&
                            code[14] == 0x0f && code[15] == 0x00) {
                            static const uint8_t zret[8] = {0x12, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00};
                            memcpy(code, zret, sizeof(zret));
                            edits++;
                        }
                    }
                }
            }
        }
    }

    /* 5 & 6. Universal VideoSAT maxZoom 12.0f->10.0f & ~500ms spline timing (Common) */
    edits += apply_pat_rules(dex, dlen, &DEX_PAT_RULES[DEX_DYNAMIC_COMMON_IDX], DEX_DYNAMIC_COMMON_CNT);
    return edits;
}

/* =========================================================================
 * 7. ZIP / APK Patcher & 4-Byte Aligned STORED classesN.dex Injector
 * ========================================================================= */
typedef struct {
    const uint8_t *nm;
    uint16_t nlen;
    uint32_t lh_off, csz;
    size_t   cd_pos, next_pos, doff;
    int      is_stored_dex;
} ZipCdEntry;

static int is_classes_dex(const uint8_t *nm, uint16_t nlen) {
    if (nlen < 11 || nlen > 14) return 0;
    return (memcmp(nm, "classes", 7) == 0 && memcmp(nm + nlen - 4, ".dex", 4) == 0);
}

static int parse_classes_dex_num(const uint8_t *nm, uint16_t nlen) {
    if (!is_classes_dex(nm, nlen)) return 0;
    if (nlen == 11) return 1;
    int v = 0;
    for (uint16_t i = 7; i < nlen - 4; i++) {
        if (nm[i] < '0' || nm[i] > '9') return 0;
        v = v * 10 + (nm[i] - '0');
    }
    return v;
}

static size_t zip_find_eocd(const uint8_t *buf, size_t sz) {
    if (sz < 22) return (size_t)-1;
    size_t search_start = (sz > 65557) ? (sz - 65557) : 0;
    for (size_t i = sz - 22; i >= search_start; i--) {
        if (buf[i] == 0x50 && buf[i + 1] == 0x4b && buf[i + 2] == 0x05 && buf[i + 3] == 0x06) {
            uint16_t clen = rd16(buf + i + 20);
            if (i + 22 + clen == sz) return i;
        }
        if (i == search_start) break;
    }
    return (size_t)-1;
}

static int zip_next_cd_entry(const uint8_t *buf, size_t sz, size_t cd_end, size_t pos, ZipCdEntry *e) {
    if (pos + 46 > cd_end || rd32(buf + pos) != 0x02014b50u) return 0;
    uint16_t comp = rd16(buf + pos + 10);
    uint32_t csz  = rd32(buf + pos + 20);
    uint16_t nlen = rd16(buf + pos + 28);
    uint16_t elen = rd16(buf + pos + 30);
    uint16_t clen = rd16(buf + pos + 32);
    uint32_t lh_off = rd32(buf + pos + 42);
    const uint8_t *nm = buf + pos + 46;

    e->nm = nm;
    e->nlen = nlen;
    e->lh_off = lh_off;
    e->csz = csz;
    e->cd_pos = pos;
    e->next_pos = pos + 46 + nlen + elen + clen;
    e->doff = 0;
    e->is_stored_dex = 0;

    if (comp == 0 && is_classes_dex(nm, nlen) && (size_t)lh_off + 30 <= sz) {
        uint16_t lnlen = rd16(buf + lh_off + 26);
        uint16_t lelen = rd16(buf + lh_off + 28);
        size_t doff = (size_t)lh_off + 30 + lnlen + lelen;
        if (doff + csz <= sz) {
            e->doff = doff;
            e->is_stored_dex = 1;
        }
    }
    return 1;
}

static void dex_finalize_crc(uint8_t *buf, const ZipCdEntry *e) {
    uint8_t *dex = buf + e->doff;
    sha1_calc(dex + 32, e->csz - 32, dex + 12);
    wr32(dex + 8, adler32_calc(dex + 12, e->csz - 12));
    uint32_t ncrc = crc32_calc(dex, e->csz);
    wr32(buf + e->lh_off + 14, ncrc);
    wr32(buf + e->cd_pos + 16, ncrc);
}

static int zip_write_with_stored_dex(const char *dst, const uint8_t *buf, size_t sz,
                                     size_t eocd, uint32_t cd_off, uint32_t cd_sz,
                                     int dex_num, const uint8_t *dex_data, uint32_t dex_sz,
                                     char out_name[16]) {
    size_t nlen = 7;
    memcpy(out_name, "classes", 7);
    if (dex_num >= 10) out_name[nlen++] = (char)('0' + (dex_num / 10));
    out_name[nlen++] = (char)('0' + (dex_num % 10));
    memcpy(out_name + nlen, ".dex", 4);
    nlen += 4;
    out_name[nlen] = '\0';

    uint32_t dex_crc     = crc32_calc(dex_data, dex_sz);
    uint32_t pad         = (4u - (((uint32_t)cd_off + 30u + (uint32_t)nlen) & 3u)) & 3u;
    uint32_t new_lfh_off = cd_off + pad;
    uint32_t new_dex_off = new_lfh_off + 30u + (uint32_t)nlen;
    uint32_t new_cd_off  = new_dex_off + dex_sz;
    uint32_t new_cd_sz   = cd_sz + 46u + (uint32_t)nlen;
    size_t   eocd_sz     = sz - eocd;
    size_t   new_sz      = (size_t)new_cd_off + (size_t)new_cd_sz + eocd_sz;

    uint8_t *out = alloc_buf(new_sz + 4096);
    if (!out) return -2;
    memcpy(out, buf, cd_off);
    if (pad) memset(out + cd_off, 0, pad);

    /* Local File Header (STORED, 4-byte aligned DEX payload) */
    uint8_t *lh = out + new_lfh_off;
    memset(lh, 0, 30);
    wr32(lh + 0,  0x04034b50u);
    wr16(lh + 4,  20u);
    wr16(lh + 12, 0x0021u);
    wr32(lh + 14, dex_crc);
    wr32(lh + 18, dex_sz);
    wr32(lh + 22, dex_sz);
    wr16(lh + 26, (uint16_t)nlen);
    memcpy(lh + 30, out_name, nlen);
    memcpy(out + new_dex_off, dex_data, dex_sz);

    /* Central Directory + appended entry */
    memcpy(out + new_cd_off, buf + cd_off, cd_sz);
    uint8_t *ce = out + new_cd_off + cd_sz;
    memset(ce, 0, 46);
    wr32(ce + 0,  0x02014b50u);
    wr16(ce + 4,  20u);
    wr16(ce + 6,  20u);
    wr16(ce + 14, 0x0021u);
    wr32(ce + 16, dex_crc);
    wr32(ce + 20, dex_sz);
    wr32(ce + 24, dex_sz);
    wr16(ce + 28, (uint16_t)nlen);
    wr32(ce + 42, new_lfh_off);
    memcpy(ce + 46, out_name, nlen);

    /* Updated EOCD */
    uint8_t *ne = out + new_cd_off + new_cd_sz;
    memcpy(ne, buf + eocd, eocd_sz);
    wr16(ne + 8,  (uint16_t)(rd16(ne + 8) + 1u));
    wr16(ne + 10, (uint16_t)(rd16(ne + 10) + 1u));
    wr32(ne + 12, new_cd_sz);
    wr32(ne + 16, new_cd_off);

    int rc = write_file_all(dst, out, new_sz);
    free_buf(out, new_sz + 4096);
    return rc;
}

static int cmd_patch_apk(const char *src, const char *dst) {
    size_t sz = 0;
    uint8_t *buf = read_file_all(src, &sz);
    if (!buf) { xputs(2, "ERR: cannot read APK\n"); return 1; }

    size_t eocd = zip_find_eocd(buf, sz);
    if (eocd == (size_t)-1) { free_buf(buf, sz + 4096); xputs(2, "ERR: invalid APK (no EOCD)\n"); return 1; }
    uint32_t cd_sz  = rd32(buf + eocd + 12);
    uint32_t cd_off = rd32(buf + eocd + 16);
    if ((size_t)cd_off + cd_sz > sz) { free_buf(buf, sz + 4096); return 1; }
    size_t cd_end = (size_t)cd_off + cd_sz;

    /* Tier 1: Exact OOS16/OS3 ported MiuiCamera full patch (>=20 edits & >=2 method anchors) */
    int total_edits = 0, meth_edits = 0, dex_patched = 0;
    int max_dex_num = 1, has_smoother_dex = 0, smoother_hooked = 0;
    ZipCdEntry ze;
    for (size_t pos = cd_off; zip_next_cd_entry(buf, sz, cd_end, pos, &ze); pos = ze.next_pos) {
        if (!ze.is_stored_dex) continue;
        int dnum = parse_classes_dex_num(ze.nm, ze.nlen);
        if (dnum > max_dex_num) max_dex_num = dnum;
        uint8_t *dex = buf + ze.doff;
        if (ze.csz == sizeof(ZOOM_SMOOTHER_DEX) && memcmp(dex, ZOOM_SMOOTHER_DEX, 32) == 0) {
            has_smoother_dex = 1;
        }
        int pe = apply_pat_rules(dex, ze.csz, DEX_PAT_RULES, ARRAY_LEN(DEX_PAT_RULES));
        int me = apply_dex_meth_rules(dex, ze.csz);
        int e = pe + me;
        if (find_bytes(dex, ze.csz, RULE_BLOB + DEX_A0_TRAMPOLINE_OFF, 20, 0) != (size_t)-1) {
            smoother_hooked = 1;
        }
        if (e > 0) {
            dex_finalize_crc(buf, &ze);
            total_edits += e;
            meth_edits += me;
            dex_patched++;
        }
    }

    /* Tier 2 fallback: ProGuard-independent dynamic string/API-anchored patch (HyperOS 4 & native MiuiCam) */
    if (total_edits < 20 || meth_edits < 2) {
        free_buf(buf, sz + 4096);
        buf = read_file_all(src, &sz);
        if (!buf) { xputs(2, "ERR: cannot re-read APK\n"); return 1; }
        int dyn_edits = 0, sat_anchors = 0, dyn_dex = 0;
        for (size_t pos = cd_off; zip_next_cd_entry(buf, sz, cd_end, pos, &ze); pos = ze.next_pos) {
            if (!ze.is_stored_dex) continue;
            int e = apply_dex_dynamic(buf + ze.doff, ze.csz, &sat_anchors);
            if (e > 0) {
                dex_finalize_crc(buf, &ze);
                dyn_edits += e;
                dyn_dex++;
            }
        }
        if (dyn_edits < 6 || sat_anchors < 2) {
            free_buf(buf, sz + 4096);
            xputs(1, "[SKIP] APK incompatible ("); xput_u32(1, (uint32_t)dyn_edits);
            xputs(1, " dynamic edits, "); xput_u32(1, (uint32_t)sat_anchors);
            xputs(1, " SAT anchors; keeping stock APK)\n");
            return 3;
        }
        int rc = write_file_all(dst, buf, sz);
        free_buf(buf, sz + 4096);
        if (rc != 0) { xputs(2, "ERR: failed to write patched APK\n"); return 1; }
        xputs(1, "[OK] patched APK (universal dynamic): "); xput_u32(1, (uint32_t)dyn_edits);
        xputs(1, " edits across "); xput_u32(1, (uint32_t)dyn_dex); xputs(1, " dex files\n");
        return 0;
    }

    /* Inject 4-byte aligned STORED classesN.dex (ZoomManager smoother) when Lt5/f;.a0 hook matched */
    if (smoother_hooked && !has_smoother_dex && max_dex_num < 99) {
        char dname[16];
        int rc = zip_write_with_stored_dex(dst, buf, sz, eocd, cd_off, cd_sz, max_dex_num + 1,
                                           ZOOM_SMOOTHER_DEX, (uint32_t)sizeof(ZOOM_SMOOTHER_DEX), dname);
        free_buf(buf, sz + 4096);
        if (rc == -2) { xputs(2, "ERR: OOM injecting smoother DEX\n"); return 1; }
        if (rc != 0)  { xputs(2, "ERR: failed to write patched APK\n"); return 1; }
        xputs(1, "[OK] patched APK: "); xput_u32(1, (uint32_t)total_edits);
        xputs(1, " edits across "); xput_u32(1, (uint32_t)dex_patched);
        xputs(1, " dex files + injected "); xputs(1, dname); xputs(1, " (ZoomManager smoother)\n");
        return 0;
    }

    int rc = write_file_all(dst, buf, sz);
    free_buf(buf, sz + 4096);
    if (rc != 0) { xputs(2, "ERR: failed to write patched APK\n"); return 1; }
    xputs(1, "[OK] patched APK: "); xput_u32(1, (uint32_t)total_edits);
    xputs(1, " edits across "); xput_u32(1, (uint32_t)dex_patched); xputs(1, " dex files\n");
    return 0;
}

/* =========================================================================
 * 8. CLI Dispatcher & Freestanding Entrypoint
 * ========================================================================= */
static int main_c(int argc, char **argv) {
    if (argc < 4) {
        xputs(2, "Usage: garnet_patcher <patch-camx|patch-hal|patch-tuned|patch-apk> ...\n");
        return 2;
    }
    if (xstrcmp(argv[1], "patch-camx") == 0 && argc == 4)  return cmd_patch_camx(argv[2], argv[3]);
    if (xstrcmp(argv[1], "patch-hal") == 0 && argc == 5)   return cmd_patch_hal(argv[2], argv[3], argv[4]);
    if (xstrcmp(argv[1], "patch-tuned") == 0 && argc == 5) return cmd_patch_tuned(argv[2], argv[3], argv[4]);
    if (xstrcmp(argv[1], "patch-apk") == 0 && argc == 4)   return cmd_patch_apk(argv[2], argv[3]);
    xputs(2, "ERR: invalid arguments\n");
    return 2;
}

void _start_c(long *sp) {
    int argc = (int)sp[0];
    char **argv = (char **)(sp + 1);
    sc1(SYS_exit_group, main_c(argc, argv));
}

#if defined(__aarch64__)
__asm__(
    ".global _start\n"
    "_start:\n"
    "  mov x0, sp\n"
    "  and sp, x0, #-16\n"
    "  bl _start_c\n"
);
#elif defined(__x86_64__)
__asm__(
    ".global _start\n"
    "_start:\n"
    "  mov %rsp, %rdi\n"
    "  and $-16, %rsp\n"
    "  call _start_c\n"
);
#endif
