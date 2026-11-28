/*
 * garnet_patcher.c — Universal zero-dependency, zero-relocation static-PIE ARM64
 * pattern patcher for garnet (Poco X6 5G / Redmi Note 13 Pro 5G).
 *
 * Works across HyperOS, AOSP, ColorOS / OxygenOS ports & Leica Camera updates.
 *
 * Commands:
 *   garnet_patcher patch-camx <src_or_dash> <dst>
 *   garnet_patcher patch-hal  <camera_qcom|chi_override|mcreprocrt|feature2_generic> <src> <dst>
 *   garnet_patcher patch-apk  <src.apk> <dst.apk>
 */
#include <stdint.h>
#include <stddef.h>

#if defined(__aarch64__)
#define SYS_openat      56
#define SYS_close       57
#define SYS_read        63
#define SYS_write       64
#define SYS_lseek       62
#define SYS_mmap        222
#define SYS_munmap      215
#define SYS_exit_group  94
#define AT_FDCWD       (-100)
#define O_RDONLY        0
#define O_WRONLY_CT     0x241
#define SEEK_END        2
#define PROT_RW         3
#define MAP_PRIV_ANON   0x22

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
#define AT_FDCWD       (-100)
#define O_RDONLY        0
#define O_WRONLY_CT     0x241
#define SEEK_END        2
#define PROT_RW         3
#define MAP_PRIV_ANON   0x22

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
    while (n--) { if (*p != *q) return (int)*p - (int)*q; p++; q++; }
    return 0;
}
static size_t xstrlen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }
static int xstrcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (int)(uint8_t)*a - (int)(uint8_t)*b;
}
static void xputs(int fd, const char *s) { sc3(SYS_write, fd, (long)s, (long)xstrlen(s)); }
static void xput_u32(int fd, uint32_t v) {
    char b[16]; int i = 0;
    if (v == 0) b[i++] = '0';
    else while (v) { b[i++] = (char)('0' + (v % 10)); v /= 10; }
    char r[16]; for (int j = 0; j < i; j++) r[j] = b[i - 1 - j];
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
static void free_buf(uint8_t *p, size_t sz) { if (p) sc2(SYS_munmap, (long)p, (long)(sz ? sz : 4096)); }

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
    size_t p2 = find_bytes(hay, hlen, ned, nlen, p1 + 1);
    if (p2 != (size_t)-1) return 2;
    *out_pos = p1;
    return 1;
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
    uint32_t a=h[0], b=h[1], c=h[2], d=h[3], e=h[4];
    for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20)      { f = (b & c) | ((~b) & d); k = 0x5A827999u; }
        else if (i < 40) { f = b ^ c ^ d;            k = 0x6ED9EBA1u; }
        else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDCu; }
        else             { f = b ^ c ^ d;            k = 0xCA62C1D6u; }
        uint32_t t = rol32(a, 5) + f + e + k + w[i];
        e = d; d = c; c = rol32(b, 30); b = a; a = t;
    }
    h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e;
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
        out[i*4]=(uint8_t)(h[i]>>24); out[i*4+1]=(uint8_t)(h[i]>>16);
        out[i*4+2]=(uint8_t)(h[i]>>8); out[i*4+3]=(uint8_t)h[i];
    }
}

/* Single flat data blob + integer-offset descriptors -> ZERO relocations! */
static const uint8_t RULE_BLOB[] = {
    0xa8,0x1f,0x00,0x90,0x3f,0x07,0x00,0x71,0x08,0xe5,0x45,0xf9,0xe0,0x0f,0x00,0x54,0x59,0x11,0x00,0x35,
    0x08,0x91,0x40,0x39,0xc8,0x16,0x00,0x37,0x8b,0x00,0x00,0x14,0x1f,0x01,0x09,0x6b,0x81,0x70,0x00,0x54,
    0x21,0xff,0xff,0xd0,0x21,0xd4,0x00,0x91,0xe0,0x03,0x13,0xaa,0xc4,0xe9,0x00,0x94,0x1f,0x00,0x00,0x71,
    0x78,0x00,0x00,0xf0,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x66,0x66,0x26,0x3f,0x72,0xf9,0x7f,
    0x3f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0c,0x93,0x19,0x03,0x00,0x00,0x00,0x16,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x3f,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x06,0x00,0x00,0x00,
    0x3e,0xe1,0x03,0x00,0x54,0x1f,0xaf,0x00,0x71,0x88,0x03,0x00,0x54,0xe8,0x03,0x18,0x2a,0x6a,0x04,0x80,
    0xd2,0x29,0x00,0x80,0x52,0x0a,0x00,0xc1,0xf2,0x28,0x21,0xc8,0x9a,0x1f,0x01,0x0a,0xea,0xa1,0xfd,0xff,
    0x54,0x6a,0x04,0x80,0xd2,0x0a,0x00,0xa6,0xf2,0x0a,0x00,0xc1,0xf2,0x29,0x00,0x80,0x52,0x6a,0x04,0x80,
    0xd2,0x29,0x00,0x80,0x52,0x0a,0x00,0xc1,0xf2,0x28,0x21,0xc8,0x9a,0x1f,0x01,0x0a,0xea,0xa1,0xfd,0xff,
    0x54,0x14,0x00,0x00,0x14,0xd8,0x68,0xf0,0xff,0xf0,0x08,0x39,0x00,0x91,0x60,0xee,0x07,0xf9,0x0f,0x04,
    0x00,0xfd,0x17,0x10,0x00,0xb9,0x08,0x00,0x00,0xf9,0x1f,0x7c,0x02,0xa9,0x08,0x00,0xbd,0xcc,0xd4,0x07,
    0x94,0x09,0x10,0x00,0xb0,0x08,0x00,0x01,0x91,0x29,0x61,0x3e,0x91,0x01,0x01,0x80,0x52,0x7f,0xff,0x03,
    0xa9,0x60,0xa3,0x01,0xa9,0x3f,0x09,0x10,0x00,0xb0,0x08,0x00,0x01,0x91,0x29,0x61,0x3e,0x91,0x01,0x01,
    0x80,0x52,0x7f,0xff,0x03,0xa9,0x60,0xa3,0x01,0xa9,0x20,0x05,0x40,0xad,0x02,0x02,0x0c,0x01,0xad,0x20,
    0x00,0x80,0x52,0xbf,0xd4,0x07,0x94,0x68,0x12,0x00,0xf0,0x60,0x1b,0x00,0xf9,0x08,0x81,0x0c,0x91,0xe9,
    0x03,0x1b,0xaa,0x28,0x10,0x00,0x90,0xbf,0xd4,0x07,0x94,0x68,0x12,0x00,0xf0,0x60,0x1b,0x00,0xf9,0x08,
    0x81,0x0c,0x91,0xe9,0x03,0x1b,0xaa,0x08,0x21,0x37,0x91,0x01,0x08,0x80,0x52,0xe1,0x35,0x60,0x1b,0x00,
    0xf9,0x08,0x81,0x0c,0x91,0xe9,0x03,0x1b,0xaa,0x08,0x21,0x37,0x91,0x01,0x08,0x80,0x52,0x6b,0x2b,0x00,
    0xfd,0x77,0x5b,0x00,0xb9,0x00,0x01,0xc0,0x3d,0x6b,0x2b,0x00,0xfd,0x77,0x5b,0x00,0xb9,0x37,0x00,0x80,
    0x52,0x08,0x84,0x00,0xf8,0xc8,0xee,0xff,0x90,0x60,0x83,0x03,0xa9,0x08,0x59,0x06,0x91,0x00,0x04,0x81,
    0x3c,0x33,0x00,0x80,0x52,0x1f,0xf1,0x00,0x71,0x80,0x00,0x00,0x54,0x1f,0x01,0x01,0x71,0x40,0x00,0x00,
    0x54,0xf3,0x03,0x1f,0x2a,0xe0,0x0b,0x40,0xf9,0xf9,0x00,0x01,0x08,0x80,0x52,0x9f,0x0a,0x00,0xf9,0xc9,
    0xe0,0x02,0x94,0xf6,0x07,0x80,0x52,0xf5,0x03,0x00,0xaa,0x81,0x00,0x80,0x52,0x16,0x20,0x00,0xb9,0xd6,
    0x52,0x01,0x3b,0x80,0x52,0x32,0xca,0x1b,0x71,0x20,0x81,0xdf,0x21,0x00,0x0a,0x01,0x2e,0x01,0x04,0x01,
    0x3a,0x01,0x12,0x00,0x52,0x00,0x3b,0x80,0x71,0x20,0x7d,0xdf,0x20,0x00,0x0a,0x00,0x2d,0x00,0x04,0x00,
    0x3d,0x00,0x03,0x00,0x28,0x06,0x52,0x01,0x3b,0x80,0x52,0x32,0xca,0x1b,0x2d,0x00,0x04,0x01,0x39,0x00,
    0x0a,0x00,0xa7,0x02,0x02,0x01,0x15,0x00,0x40,0x3d,0xc8,0x02,0xa6,0x04,0x01,0x02,0x28,0x08,0x2d,0x02,
    0x02,0x01,0xb2,0x20,0x3d,0x00,0x09,0x00,0x00,0x00,0x00,0x00,0x38,0x01,0x05,0x00,0x72,0x20,0x7e,0x11,
    0x01,0x00,0x55,0x22,0xda,0x1b,0x00,0x00,0x00,0x00,0x00,0x00,0x72,0x20,0x7e,0x11,0x10,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x90,0x0a,0x0a,0x03,0x38,0x03,0x0f,0x00,0x71,0x30,0xd8,0xa0,0x21,0x0c,0x0a,0x01,
    0x38,0x01,0x09,0x00,0x71,0x10,0x5c,0xdf,0x00,0x00,0x0c,0x00,0x52,0xc1,0x3e,0xd7,0xdd,0x01,0x01,0x03,
    0x39,0x01,0x09,0x00,0x45,0x00,0x0a,0x02,0x38,0x02,0x34,0x00,0x54,0x42,0x54,0xd7,0x28,0x34,0x00,0x00,
    0x59,0xcf,0xff,0x74,0x00,0x00,0x00,0x00,0x6e,0x30,0x0c,0xa2,0x21,0x00,0x6f,0x30,0x4a,0xa7,0x21,0x00,
    0x29,0x00,0x75,0x00,0x39,0x00,0x20,0x00,0x22,0x10,0x16,0x25,0x28,0x20,0x00,0x00,0x22,0x05,0xcc,0x0b,
    0x70,0x10,0x02,0x29,0x05,0x00,0x22,0x05,0xc9,0x0b,0x70,0x10,0xff,0x28,0x05,0x00,0x62,0x00,0x5b,0x03,
    0x54,0x00,0x5d,0x03,0x6e,0x10,0xa5,0xf2,0x00,0x00,0x0a,0x00,0x38,0x00,0x11,0x00,0x59,0x68,0x5f,0x80,
    0x12,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x59,0x60,0x5f,0x80,0x22,0x0e,0xcc,0x0b,0x70,0x10,0x02,0x29,0x0e,0x00,0x22,0x0e,0xcc,0x16,0x70,0x10,
    0x17,0x7a,0x0e,0x00,0x54,0x0b,0x3e,0x80,0x16,0x0e,0x64,0x00,0x6e,0x30,0x78,0x16,0x16,0x0e,0x2c,0x01,
    0x16,0x0e,0x96,0x00,0x16,0x0e,0x2c,0x01,0x54,0x40,0x77,0x03,0x13,0x03,0x3c,0x00,0x6e,0x30,0x68,0x22,
    0x13,0x03,0x3e,0x00,0x54,0x40,0x77,0x03,0x13,0x02,0x3c,0x00,0x6e,0x20,0x69,0x22,0x13,0x02,0x3e,0x00,
    0xdc,0x55,0x28,0x14,0x38,0x03,0x05,0x00,0x15,0x03,0x80,0x3f,0x28,0x05,0x00,0x00,0x3f,0x00,0x00,0x00,
    0x12,0x00,0x38,0x05,0x3d,0x00,0x54,0x51,0x27,0xe1,0x12,0x10,0x0f,0x00,0x00,0x00,0x0e,0x00,0x00,0x00,
    0x54,0x11,0xad,0x00,0x6e,0x10,0x73,0xe0,0x12,0x01,0x11,0x01,0x39,0x00,0x07,0x00,0x12,0x00,0x71,0x10,
    0x17,0xce,0x00,0x00,0x0c,0x00,0x6e,0x10,0x08,0xce,0x1c,0x00,0xbc,0x28,0x11,0x00,0x00,0x00,0x00,0x00,
    0x22,0x05,0xfc,0x03,0x70,0x10,0xbd,0x0c,0x05,0x00,0x14,0x00,0x9a,0x99,0x19,0x3f,0x71,0x10,0xf4,0xcd,
    0x00,0x00,0x0c,0x00,0x15,0x01,0x80,0x3f,0x71,0x10,0xf4,0xcd,0x01,0x00,0x0c,0x01,0x15,0x02,0x00,0x40,
    0x71,0x10,0xf4,0xcd,0x02,0x00,0x0c,0x02,0x24,0x30,0x2b,0x2a,0x10,0x02,0x0c,0x03,0x13,0x04,0xa3,0x00,
    0x6e,0x30,0xc5,0x0c,0x45,0x03,0x24,0x30,0x2b,0x2a,0x10,0x02,0x0c,0x03,0x13,0x04,0xa2,0x00,0x6e,0x30,
    0xc5,0x0c,0x45,0x03,0x24,0x30,0x2b,0x2a,0x10,0x02,0x0c,0x00,0x13,0x01,0xad,0x00,0x6e,0x30,0xc5,0x0c,
    0x15,0x00,0x11,0x05,0x22,0x05,0xfc,0x03,0x70,0x10,0xbd,0x0c,0x05,0x00,0x14,0x00,0x9a,0x99,0x19,0x3f,
    0x71,0x10,0xf4,0xcd,0x00,0x00,0x0c,0x00,0x15,0x01,0x80,0x3f,0x71,0x10,0xf4,0xcd,0x01,0x00,0x0c,0x01,
    0x15,0x02,0x00,0x40,0x71,0x10,0xf4,0xcd,0x02,0x00,0x0c,0x02,0x15,0x03,0x40,0x40,0x71,0x10,0xf4,0xcd,
    0x03,0x00,0x0c,0x03,0x15,0x04,0xa0,0x40,0x71,0x10,0xf4,0xcd,0x04,0x00,0x0c,0x04,0x24,0x54,0x2b,0x2a,
    0x10,0x32,0x0c,0x00,0x13,0x01,0xa3,0x00,0x6e,0x30,0xc5,0x0c,0x15,0x00,0x13,0x01,0xa2,0x00,0x6e,0x30,
    0xc5,0x0c,0x15,0x00,0x11,0x05,0x00,0x00,0x20,0x00,0xaa,0x28,0x0f,0x00,0x00,0x00,0x01,0x00,0x01,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x20,0x00,0xa5,0x28,0x0f,0x00,0x00,0x00,
    0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x20,0x00,0xab,0x28,
    0xdf,0x00,0x12,0x10,0x0f,0x00,0x00,0x00,0x20,0x00,0xab,0x28,0x0f,0x00,0x00,0x00,0x01,0x00,0x01,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x20,0x00,0xa8,0x28,0x0f,0x00,0x00,0x00,
    0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x20,0x00,0xaf,0x28,
    0x0f,0x00,0x12,0x10,0x0f,0x00,0x00,0x00,0x0a,0x00,0x00,0x00,0x14,0x01,0xb3,0x98,0x71,0x41,0x1a,0x00,
    0x00,0x00,0x71,0x20,0xe7,0x01,0x01,0x00,0x0c,0x01,0x11,0x01,0x01,0x00,0x0c,0xf8,0x01,0x00,0x01,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x20,0x00,0xaa,0x28,0x0f,0x00,0x00,0x00,
    0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x00,0x12,0x10,0x0f,0x00,0x00,0x00,
    0xdf,0x00,0x00,0x01,0x0f,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x03,0x00,0x00,0x00,0x20,0x00,0xa5,0x28,0x0f,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x20,0x00,0x91,0x28,0xdf,0x00,0x12,0x10,0x0f,0x00,0x00,0x00,
    0x20,0x00,0xb5,0x28,0x0f,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x03,0x00,0x00,0x00,0x20,0x00,0xa5,0x28,0x0f,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x20,0x00,0xa5,0x28,0x0f,0x00,0x12,0x10,0x0f,0x00,0x00,0x00,
    0x15,0x00,0x40,0x41,0x0f,0x00,0x15,0x00,0x20,0x41,0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfc,0x42,
    0x00,0x00,0x7d,0x43,0x00,0x00,0xd2,0x43,0x00,0xc0,0x08,0x44,0x00,0x40,0x1b,0x44,0x00,0x00,0x52,0x44,
    0x00,0x00,0x7a,0x44,0x00,0x00,0x00,0x00,0x00,0x00,0xb4,0x43,0x00,0x00,0x25,0x44,0x00,0x80,0x77,0x44,
    0x00,0x00,0x9b,0x44,0x00,0xc0,0xad,0x44,0x00,0xc0,0xda,0x44,0x00,0x80,0xfc,0x44,0x1a,0x02,0x8d,0x93,
    0x38,0x00,0x54,0x00,0x71,0x10,0x99,0xa8,0x00,0x00,0x00,0x00,0x4c,0xe4,0xb0,0x97,0xe4,0xb0,0x9b,0xe4,
    0xb0,0x99,0xe4,0xb1,0x9a,0xe4,0xb0,0x99,0xe4,0xb0,0x9d,0xe4,0xb1,0x9a,0xe4,0xb0,0x90,0xe4,0xb0,0x91,
    0xe4,0xb0,0x82,0xe4,0xb0,0x9d,0xe4,0xb0,0x97,0xe4,0xb0,0x91,0xe4,0xb1,0x9a,0xe4,0xb0,0x86,0xe4,0xb0,
    0x91,0xe4,0xb0,0x90,0xe4,0xb0,0x99,0xe4,0xb0,0x9d,0xe4,0xb1,0x9a,0xe4,0xb0,0xb7,0xe4,0xb0,0x9b,0xe4,
    0xb0,0x99,0xe4,0xb0,0x99,0xe4,0xb0,0x9b,0xe4,0xb0,0x9a,0xe4,0xb0,0xba,0xe4,0xb0,0x9b,0xe4,0xb0,0x80,
    0xe4,0xb0,0x91,0x3b,0x56,0x31,0x12,0x10,0x0f,0x00,0x12,0x00,0x0f,0x00,0x4c,0xe4,0xb0,0x97,0xe4,0xb0,
    0x9b,0xe4,0xb0,0x99,0xe4,0xb1,0x9a,0xe4,0xb0,0x99,0xe4,0xb0,0x9d,0xe4,0xb1,0x9a,0xe4,0xb0,0x90,0xe4,
    0xb0,0x91,0xe4,0xb0,0x82,0xe4,0xb0,0x9d,0xe4,0xb0,0x97,0xe4,0xb0,0x91,0xe4,0xb1,0x9a,0xe4,0xb0,0x86,
    0xe4,0xb0,0x91,0xe4,0xb0,0x90,0xe4,0xb0,0x99,0xe4,0xb0,0x9d,0xe4,0xb1,0x9a,0xe4,0xb0,0xb7,0xe4,0xb0,
    0x9b,0xe4,0xb0,0x99,0xe4,0xb0,0x99,0xe4,0xb0,0x9b,0xe4,0xb0,0x9a,0xe4,0xb0,0xba,0xe4,0xb0,0x9b,0xe4,
    0xb0,0x80,0xe4,0xb0,0x91,0x3b,0x69,0x36,0x12,0x10,0x0f,0x00,0x12,0x00,0x0f,0x00,0x4c,0xed,0x89,0x83,
    0xed,0x89,0x8f,0xed,0x89,0x8d,0xed,0x88,0x8e,0xed,0x89,0x8d,0xed,0x89,0x89,0xed,0x88,0x8e,0xed,0x89,
    0x84,0xed,0x89,0x85,0xed,0x89,0x96,0xed,0x89,0x89,0xed,0x89,0x83,0xed,0x89,0x85,0xed,0x88,0x8e,0xed,
    0x89,0xb0,0xed,0x89,0x85,0xed,0x89,0x92,0xed,0x89,0x89,0xed,0x89,0x84,0xed,0x89,0x8f,0xed,0x89,0x94,
    0x3b,0x45,0x30,0xf6,0xf7,0xb7,0xf7,0x4c,0xe4,0x98,0x8f,0xe4,0x98,0x83,0xe4,0x98,0x81,0xe4,0x99,0x82,
    0xe4,0x98,0x81,0xe4,0x98,0x85,0xe4,0x99,0x82,0xe4,0x98,0x88,0xe4,0x98,0x89,0xe4,0x98,0x9a,0xe4,0x98,
    0x85,0xe4,0x98,0x8f,0xe4,0x98,0x89,0xe4,0x99,0x82,0xe4,0x98,0x8f,0xe4,0x98,0x83,0xe4,0x98,0x81,0xe4,
    0x98,0x81,0xe4,0x98,0x83,0xe4,0x98,0x82,0xe4,0x99,0x82,0xe4,0x98,0xaf,0xe4,0x98,0x83,0xe4,0x98,0x81,
    0xe4,0x98,0x81,0xe4,0x98,0x83,0xe4,0x98,0x82,0x3b,0x47,0x36,0x20,0x00,0xa5,0x28,0x0f,0x00,0x12,0x10,
    0x0f,0x00,0x00,0x00,0x12,0x10,0x71,0x10,0xc2,0xd0,0x00,0x00,0x0c,0x01,0x39,0x04,0x03,0x00,0x0f,0x00,
    0x12,0x00,0x0f,0x00,0x00,0x00,0x00,0x00,0x39,0x01,0x03,0x00,0x0e,0x00,0x52,0x22,0x8e,0xde,0x12,0xf0,
    0x32,0x02,0x09,0x00,0x71,0x00,0x6c,0x45,0x00,0x00,0x0c,0x00,0x52,0x22,0x8e,0xde,0x3b,0x02,0x03,0x00,
    0x12,0x02,0x6e,0x20,0xf0,0xe5,0x02,0x00,0x0a,0x02,0x38,0x02,0x0b,0x00,0x55,0x32,0x92,0xde,0x71,0x00,
    0x6c,0x45,0x00,0x00,0x00,0x00,0x6e,0x20,0xf0,0xe5,0x10,0x00,0x0a,0x00,0x38,0x00,0x2e,0x00,0x54,0x55,
    0x66,0x42,0x72,0x10,0xa9,0xdd,0x00,0x00,0x00,0x00,0x02,0x12,0x02,0x00,0x62,0x02,0x4c,0x30,0x6e,0x10,
    0xb0,0x09,0x02,0x00,0x0c,0x02,0x62,0x03,0x3b,0xd7,0x52,0x63,0xfc,0x44,0x02,0x02,0x12,0x00,0x14,0x03,
    0x40,0x78,0x7d,0x01,0x35,0x32,0x05,0x00,0xda,0x02,0x02,0x02,0x28,0x59,0x14,0x03,0x00,0x87,0x93,0x03,
    0x35,0x32,0x55,0x00,0xda,0x02,0x02,0x03,0xdb,0x02,0x02,0x02,0x28,0x4f,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x71,0x10,0xba,0xd0,0x07,0x00,0x0a,0x07,0x28,0x0f,
    0x52,0x77,0x55,0x3f,0x28,0x0c,0x6e,0x20,0x27,0xd1,0x03,0x00,0x28,0x0a,0x52,0x77,0x55,0x3f,0x28,0x0c,
    0x52,0x77,0x55,0x3f,0x0f,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0xda,0x07,0x07,0x02,0x0f,0x07,0x00,0x00,
    0x00,0x00,0xe9,0xe7,0x05,0xb9,0x09,0x65,0x47,0xb9,0xff,0xfb,0x05,0xb9,0x29,0x06,0x00,0x34,0xbf,0x16,
    0x00,0x71,0x80,0x02,0x00,0x54,0xbf,0x0a,0x00,0x71,0x31,0x00,0x00,0x14,0xe2,0x0f,0x06,0xbd,0xe0,0x13,
    0x06,0xbd,0xeb,0x17,0x06,0xb9,0x2a,0x06,0x00,0x34,0xbf,0x16,0x00,0x71,0x80,0x02,0x00,0x54,0xbf,0x0a,
    0x00,0x71,0x61,0x04,0x00,0x54,0xeb,0x03,0x1f,0xaa,0x28,0x00,0x80,0x52,0xe8,0x1b,0x06,0xb9,0x2f,0x00,
    0x00,0x14,0x54,0xa4,0x3f,0x80,0x6e,0x10,0x73,0x16,0x04,0x00,0x54,0xa4,0x3f,0x80,0x22,0x05,0xcc,0x0b,
    0x70,0x10,0x02,0x29,0x05,0x00,0x6e,0x20,0x7b,0x16,0x54,0x00,0x54,0xa4,0x3f,0x80,0x6e,0x10,0x73,0x16,
    0x04,0x00,0x22,0x05,0xce,0x0b,0x12,0x06,0x15,0x07,0x10,0x3f,0x15,0x08,0x80,0x3f,0x70,0x58,0x07,0x29,
    0x65,0x67,0x6e,0x20,0x7b,0x16,0x54,0x00,0x00,0x00,
    /* 2430..2453 (24B): ZoomRatioToggleView;->b AnimatorSet.setDuration(400L) context */
    0xff,0xd0,0x00,0x00,0x6e,0x20,0x0e,0x1b,0x06,0x00,0x54,0x55,0xbc,0x4d,0x16,0x00,0x90,0x01,0x6e,0x30,
    0x0d,0x1b,0x05,0x01,
    /* 2454..2457 (4B): AnimatorSet.setDuration(0L) replacement (rel=14) */
    0x16,0x00,0x00,0x00,
    /* 2458..2479 (22B padding so A0_EXP stays at 2480!) */
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,
    /* 2480..2569 (90B): Lt5/f;.a0(FI)Z +0x3a..+0x94 Log.d block */
    0x22,0x0a,0xfe,0x23,0x1a,0x0b,0x34,0xfb,0x70,0x20,0x62,0xd1,0xba,0x00,0x02,0x0b,0x13,0x00,0x6e,0x20,
    0x65,0xd1,0xba,0x00,0x1a,0x0c,0xcd,0x0a,0x6e,0x20,0x6a,0xd1,0xca,0x00,0x54,0x0c,0x59,0xdc,0x6e,0x10,
    0x38,0x22,0x0c,0x00,0x0c,0x0c,0x6e,0x20,0x6a,0xd1,0xca,0x00,0x1a,0x0c,0xd1,0x0a,0x6e,0x20,0x6a,0xd1,
    0xca,0x00,0x6e,0x20,0x66,0xd1,0x1a,0x00,0x6e,0x10,0x77,0xd1,0x0a,0x00,0x0c,0x0a,0x23,0x9c,0x78,0x2b,
    0x1a,0x0d,0x06,0x7f,0x71,0x30,0x03,0x2c,0xad,0x0c,
    /* 2570..2659 (90B): Lt5/f;.a0(FI)Z -> ZoomManager.apply(this, Float, Integer) trampoline */
    0x1a,0x0d,0x06,0x7f,0x71,0x10,0x52,0xd0,0x0d,0x00,0x0c,0x0a,0x1a,0x0b,0xca,0x8a,0x1c,0x0e,0xef,0x23,
    0x24,0x30,0x73,0x2b,0xee,0x0e,0x0c,0x0c,0x6e,0x30,0x67,0xd0,0xba,0x0c,0x0c,0x0a,0x77,0x01,0xa3,0xd0,
    0x13,0x00,0x0c,0x0b,0x71,0x10,0xc2,0xd0,0x01,0x00,0x0c,0x0c,0x24,0x30,0x78,0x2b,0xb0,0x0c,0x0c,0x0b,
    0x12,0x0c,0x6e,0x30,0xe3,0xd1,0xca,0x0b,0x0c,0x0a,0x38,0x0a,0x03,0x00,0x0f,0x04,0x02,0x0b,0x13,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

#include "zoom_smoother_dex.h"

typedef struct {
    uint16_t ctx_off, ctx_len, rel, rep_off, rep_len;
} PatRule;

typedef struct {
    uint16_t cls_off, cls_len, mth_off, mth_len, rel, exp_off, rep_off, len;
} DexMethRule;

static const PatRule CAMQCOM_RULES[] = {
    {0, 28, 12, 28, 4},       /* 0xfda2d0: QSAT / multi-cam bypass */
    {2282, 28, 12, 2310, 4},  /* 0x665968: ConstructDefaultRequestSettings EDGE_MODE -> OFF (0) */
    {2314, 36, 12, 2350, 12}, /* 0x665a88: ConstructDefaultRequestSettings NR_MODE -> FAST (1) */
};

/* Titan480/570 TF20/TF201: zero Luma temporal blend (a2MaxY/a2MinY/constBlendY=0), keep Chroma (a2MaxC/a2MinC/constBlendC) 100% untouched */
static const uint8_t TF201_LUMA_CTX[] = {
    0x09,0x2d,0x45,0x39,0xa9,0x42,0x1e,0xb8,0x09,0x29,0x45,0x39,0xa9,0xc2,0x1d,0xb8,
    0x09,0x25,0x45,0x39,0xa9,0x02,0x1e,0xb8,0x09,0x21,0x45,0x39,0xa9,0x82,0x1d,0xb8,
    0x09,0x35,0x45,0x39,0xa9,0xc2,0x1e,0xb8,0x08,0x31,0x45,0x39,0x04,0x00,0x00,0x14,
    0xe8,0x03,0x1f,0x2a
};
static const uint8_t TF201_LUMA_REP[] = {
    0xe9,0x03,0x1f,0x2a,0xa9,0x02,0x1e,0xb8,0xe9,0x03,0x1f,0x2a,0xa9,0x82,0x1d,0xb8,
    0x09,0x35,0x45,0x39,0xa9,0xc2,0x1e,0xb8,0xe8,0x03,0x1f,0x2a
};
/* Titan570 ANR101_480::Process exit hook: bypass Luma spatial NR (YLPF_FILTERING + DCBlend2_Bypass_Luma) while keeping Chroma CLPF + CNR 100% active */
static const uint8_t ANR101_HOOK_CTX[] = {
    0xe7,0x58,0x0a,0x91,0x28,0x00,0x80,0x52,0xbc,0xfe,0xff,0x17,0xf3,0x03,0x1f,0x2a,
    0xf4,0x33,0x40,0xf9,0x88,0x16,0x40,0xf9,0xa9,0x83,0x5e,0xf8
};

static const PatRule MCREPROCRT_RULES[] = {
    {32, 28, 13, 60, 1}, /* 0x33439 */
};

static const PatRule CHI_FIXED_RULES[] = {
    {92, 28, 12, 120, 1},   /* logical cam count / roleId table 0x3f -> 0x3e */
    {121, 40, 12, 161, 16}, /* SAT transition guard patch A */
    {177, 28, 14, 205, 1},  /* SAT transition guard patch B */
    {421, 28, 13, 449, 2},  /* roleId 0x3f -> 0x3e compare */
};
/* Universal 0.65f -> 0.6001f zoom threshold (handles reordered [1.8,2.68] / [0.65,0.9999] pairs) */
static const uint8_t CHI_ZOOM65_CTX[20] = {
    0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
    0x66,0x66,0x26,0x3f, 0x72,0xf9,0x7f,0x3f,
    0x00,0x00,0x00,0x00
};
static const uint8_t CHI_ZOOM65_REP[3] = {0x0c, 0x93, 0x19};
static const uint8_t GEN_HOOK_CTX[]  = {0xa8,0x83,0x1f,0xf8,0x48,0x0c,0x40,0xf9,0x09,0x48,0x41,0xf9,0x16,0x21,0x5e,0xf9,0x28,0x01,0x40,0xb9,0x96,0x0b,0x00,0xb4,0x1f,0xb9,0x00,0x71};

static const PatRule DEX_PAT_RULES[] = {
    {480, 46, 0, 526, 46}, /* LZ1/l;->onAnimationUpdate (zero-bounce instant f */
    {572, 14, 4, 586, 6}, /* LZ1/s;->onAnimationStart (NOP A4 targetZoom coll */
    {592, 6, 0, 598, 6}, /* dual/a;->onAnimationStart (NOP A4 targetZoom col */
    {604, 28, 8, 632, 12}, /* Lb0/U;->i (FHD60 on Wide/VideoSAT; clamp UW/Fron */
    {644, 12, 4, 656, 4}, /* Lb0/U;->q (unlock FHD60 VideoSAT) */
    {660, 4, 0, 664, 4}, /* Lb0/U;->v (unlock FHD60 SAT maxFps cap) */
    {668, 6, 0, 674, 6}, /* Lb0/p0;->getValueDisplayString (Pro Mode crash f */
    {680, 12, 4, 692, 4}, /* FragmentDualCameraAdjust;->Zh (live bidirectiona */
    {2362, 34, 0, 2396, 34}, /* FragmentDualCameraAdjust;->bi (PathInterpolator 0,0.5625,0,1) */
    {716, 24, 0, 740, 24}, /* FragmentDualCameraAdjust;->notifyAfterFrameAvail */
    {764, 10, 0, 774, 10}, /* FragmentDualCameraAdjust;->provideAnimateElement */
    {784, 12, 4, 796, 4}, /* FragmentDualCameraAdjust;->provideAnimateElement */
    {800, 4, 0, 804, 4}, /* FragmentDualCameraAdjust;->provideAnimateElement */
    {808, 12, 4, 820, 4}, /* LD3/e;->C()I (Native Photo SAT roleId 60 -> 62 = */
    {824, 12, 4, 836, 4}, /* LD3/e;->K()Z (Native Photo SAT hasSATCamera role */
    {840, 12, 4, 852, 4}, /* StopPointScaleZoomSliderDrawAdapter;->initValue  */
    {856, 14, 4, 870, 6}, /* Lw5/e;->B3 (isZoomRatioSupported = true) */
    {876, 12, 4, 888, 4}, /* LG6/d;->j (clamp maxZoom to real HAL 10x Photo / */
    {892, 18, 4, 910, 10}, /* LR7/d;->a fallback -> Peridot */
    {920, 104, 0, 1024, 104}, /* Peridot;->i1() (5 zoom buttons [0.6x, 1x, 2x, 3x */
    {1128, 54, 24, 1182, 6}, /* Common;->Q2 (isSupportSatIsZooming = true) */
    {1188, 54, 24, 1242, 6}, /* Common;->d4 (Macro roleId=22 = true) */
    {1248, 26, 12, 1274, 2}, /* Common;->Q6 ("ultra_wide:wide:front:macro") */
    {1276, 38, 16, 1314, 6}, /* Common;->k7 (UW lens switch = true) */
    {1320, 54, 24, 1374, 6}, /* Common;->w5 (isSupportSAT = true) */
    {1380, 54, 24, 1434, 6}, /* Common;->x7 (isSupportVideoSAT = true) */
    {1440, 6, 0, 1446, 6}, /* Common;->l0 (VideoSAT maxZoom 12.0f -> 10.0f) */
    {1452, 32, 0, 1484, 32}, /* Common;->M0()[F (smooth ~450ms ease-out spline timing) */
    {1516, 12, 4, 1528, 4}, /* LZa/g;->i (graceful fallback, no IllegalStateExc */
    {1904, 16, 0, 1920, 8}, /* Lw5/e;->V (default sharpness 1 -> 0 anti-oversharpen) */
    {1928, 24, 6, 1952, 10}, /* Lw5/E;->u0 (apply sharpness=0 in Video+Photo; keep Pro manual) */
    {1962, 20, 8, 1982, 4}, /* Lw5/E;->H0 (bypass B0 check so applyUltraWideLDC sends level=1) */
    {1986, 20, 8, 2006, 4}, /* BaseModule;->updateUltraWideLDC (bypass B0 check for SAT UW LDC) */
    {2010, 24, 4, 2034, 196}, /* video/z;->k (H264 bitrate boost: 20M->40M, 50M->75M) */
    {2230, 22, 8, 2252, 30}, /* video/I;->a (HEVC bitrate 2x boost: 15.4M->30.8M, 38.5M->77M) */
    {2430, 24, 14, 2454, 4}, /* ZoomRatioToggleView;->b (neutralize AnimatorSet.setDuration 400L -> 0L) */
    {2480, 90, 0, 2570, 90}, /* Lt5/f;.a0(FI)Z (Klee-style ZoomManager.apply trampoline) */
};

static const DexMethRule DEX_METH_RULES[] = {
    {1532, 92, 1624, 2, 0, 1626, 1630, 4}, /* CommonNote;->V1 (AlgoUp = false) */
    {1634, 92, 1726, 2, 0, 1728, 1732, 4}, /* CommonNote;->i6 (LocalParallel = false) */
    {1736, 65, 1801, 2, 8, 1803, 1805, 2}, /* Peridot;->E0 ("2:9248x6944" 64MP OV64B40) */
    {1807, 83, 1890, 2, 0, 1892, 1898, 6}, /* Common;->G6 (isSupportSatZoomSpline = true) */
};

static int apply_pat_rules(uint8_t *buf, size_t sz, const PatRule *rules, size_t nrules) {
    size_t hits[64];
    uint8_t ok[64];
    int applied = 0;
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

static int validate_elf64_aarch64(const uint8_t *buf, size_t sz) {
    if (sz < 64) return 0;
    if (memcmp(buf, "\x7f""ELF", 4) != 0) return 0;
    if (buf[4] != 2 || buf[5] != 1) return 0; /* ELF64, little-endian */
    if (rd16(buf + 18) != 183) return 0;      /* EM_AARCH64 */
    uint64_t phoff = rd64(buf + 32);
    uint16_t phent = rd16(buf + 54);
    uint16_t phnum = rd16(buf + 56);
    if (phent < 56 || phnum == 0 || phoff > sz || (uint64_t)phnum * phent > sz - phoff) return 0;
    return 1;
}

static int elf_has_str(const uint8_t *buf, size_t sz, const char *s) {
    return find_bytes(buf, sz, (const uint8_t *)s, xstrlen(s), 0) != (size_t)-1;
}

static int is_in_rx_seg(const uint8_t *buf, size_t sz, uint64_t off, uint64_t len) {
    uint64_t phoff = rd64(buf + 32);
    uint16_t phent = rd16(buf + 54);
    uint16_t phnum = rd16(buf + 56);
    for (uint16_t i = 0; i < phnum; i++) {
        uint64_t po = phoff + (uint64_t)i * phent;
        uint32_t ptype = rd32(buf + po);
        uint32_t pflags = rd32(buf + po + 4);
        if (ptype == 1 && (pflags & 1)) {
            uint64_t p_off = rd64(buf + po + 8);
            uint64_t p_filesz = rd64(buf + po + 32);
            if (off >= p_off && off + len <= p_off + p_filesz) return 1;
        }
    }
    return 0;
}

static int alloc_rx_cave_for(uint8_t *buf, size_t sz, uint64_t hook_off, uint64_t align, uint64_t need, uint64_t *out_cave_off) {
    if (!validate_elf64_aarch64(buf, sz)) return -1;
    uint64_t phoff = rd64(buf + 32);
    uint16_t phent = rd16(buf + 54);
    uint16_t phnum = rd16(buf + 56);
    for (uint16_t i = 0; i < phnum; i++) {
        uint64_t po = phoff + (uint64_t)i * phent;
        uint32_t ptype = rd32(buf + po);
        uint32_t pflags = rd32(buf + po + 4);
        if (ptype == 1 && (pflags & 1)) {
            uint64_t p_off    = rd64(buf + po + 8);
            uint64_t p_vaddr  = rd64(buf + po + 16);
            uint64_t p_filesz = rd64(buf + po + 32);
            uint64_t p_memsz  = rd64(buf + po + 40);
            if (hook_off < p_off || hook_off >= p_off + p_filesz) continue;
            if (p_filesz != p_memsz) return -1;
            uint64_t end = p_off + p_filesz;
            uint64_t cave = (end + align - 1) & ~(align - 1);
            uint64_t new_end = cave + need;
            if (new_end > sz) return -1;
            /* Verify ARM64 B/BL +/-128MB branch reach within same PT_LOAD segment */
            uint64_t dist = (cave > hook_off) ? (cave - hook_off) : (hook_off - cave);
            if (dist >= 0x07F00000ULL) return -1;
            uint64_t v_cave = p_vaddr + (cave - p_off);
            uint64_t v_new_end = p_vaddr + (new_end - p_off);
            /* Ensure cave does not collide with any other PT_LOAD segment in file or vaddr */
            for (uint16_t j = 0; j < phnum; j++) {
                if (j == i) continue;
                uint64_t q = phoff + (uint64_t)j * phent;
                if (rd32(buf + q) != 1) continue;
                uint64_t qo = rd64(buf + q + 8);
                uint64_t qv = rd64(buf + q + 16);
                uint64_t qfs = rd64(buf + q + 32);
                uint64_t qms = rd64(buf + q + 40);
                if (cave < qo + qfs && new_end > qo) return -1;
                uint64_t qv_page = qv & ~0xFFFULL;
                if (v_cave < qv + qms && v_new_end > qv_page) return -1;
            }
            for (uint64_t k = cave; k < new_end; k++) if (buf[k] != 0) return -1;
            uint64_t grow = new_end - end;
            wr64(buf + po + 32, p_filesz + grow);
            wr64(buf + po + 40, p_memsz + grow);
            *out_cave_off = cave;
            return 0;
        }
    }
    return -1;
}

static uint64_t elf_file_to_vaddr(const uint8_t *buf, size_t sz, uint64_t foff) {
    uint64_t phoff = rd64(buf + 32);
    uint16_t phent = rd16(buf + 54);
    uint16_t phnum = rd16(buf + 56);
    for (uint16_t i = 0; i < phnum; i++) {
        uint64_t po = phoff + (uint64_t)i * phent;
        if (rd32(buf + po) != 1) continue;
        uint64_t p_off = rd64(buf + po + 8);
        uint64_t p_vaddr = rd64(buf + po + 16);
        uint64_t p_filesz = rd64(buf + po + 32);
        if (foff >= p_off && foff < p_off + p_filesz) return p_vaddr + (foff - p_off);
    }
    return (uint64_t)-1;
}

static uint64_t elf_vaddr_to_file(const uint8_t *buf, size_t sz, uint64_t vaddr) {
    uint64_t phoff = rd64(buf + 32);
    uint16_t phent = rd16(buf + 54);
    uint16_t phnum = rd16(buf + 56);
    for (uint16_t i = 0; i < phnum; i++) {
        uint64_t po = phoff + (uint64_t)i * phent;
        if (rd32(buf + po) != 1) continue;
        uint64_t p_off = rd64(buf + po + 8);
        uint64_t p_vaddr = rd64(buf + po + 16);
        uint64_t p_filesz = rd64(buf + po + 32);
        if (vaddr >= p_vaddr && vaddr < p_vaddr + p_filesz) return p_off + (vaddr - p_vaddr);
    }
    return (uint64_t)-1;
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

static int elf_cstr_eq(const uint8_t *buf, size_t sz, uint64_t vaddr, const char *s) {
    uint64_t foff = elf_vaddr_to_file(buf, sz, vaddr);
    if (foff == (uint64_t)-1) return 0;
    size_t slen = xstrlen(s);
    if (foff + slen + 1 > sz) return 0;
    return memcmp(buf + foff, s, slen + 1) == 0;
}

static int elf_is_bss_vaddr(const uint8_t *buf, size_t sz, uint64_t vaddr) {
    uint64_t phoff = rd64(buf + 32);
    uint16_t phent = rd16(buf + 54);
    uint16_t phnum = rd16(buf + 56);
    for (uint16_t i = 0; i < phnum; i++) {
        uint64_t po = phoff + (uint64_t)i * phent;
        if (rd32(buf + po) != 1) continue;
        uint64_t p_vaddr  = rd64(buf + po + 16);
        uint64_t p_filesz = rd64(buf + po + 32);
        uint64_t p_memsz  = rd64(buf + po + 40);
        if (p_memsz > p_filesz && vaddr >= p_vaddr + p_filesz && vaddr < p_vaddr + p_memsz) return 1;
    }
    return 0;
}

static int is_cam_entry(const uint8_t *buf, size_t sz, uint64_t foff, const char *name, uint32_t c1, uint32_t c2, uint32_t c3) {
    if (foff + 32 > sz) return 0;
    uint64_t ptr = rd64(buf + foff);
    if (rd32(buf + foff + 8) != c1 || rd32(buf + foff + 12) != c2 || rd32(buf + foff + 16) != c3) return 0;
    return elf_cstr_eq(buf, sz, ptr, name);
}

static int find_masked_unique_rx(const uint8_t *buf, size_t sz, const uint32_t *words, const uint32_t *masks, size_t nwords, uint64_t *out_foff) {
    uint64_t phoff = rd64(buf + 32);
    uint16_t phent = rd16(buf + 54);
    uint16_t phnum = rd16(buf + 56);
    uint64_t hit = 0;
    int count = 0;
    size_t nbytes = nwords * 4;
    for (uint16_t i = 0; i < phnum; i++) {
        uint64_t po = phoff + (uint64_t)i * phent;
        if (rd32(buf + po) != 1 || !(rd32(buf + po + 4) & 1)) continue;
        uint64_t p_off = rd64(buf + po + 8);
        uint64_t p_fsz = rd64(buf + po + 32);
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
                count++;
                if (count > 1) return 2;
            }
        }
    }
    if (count == 1) *out_foff = hit;
    return count;
}

typedef struct {
    uint64_t pc_strd;
    uint32_t orig_strd;
    uint64_t pc_tbl_adrp;
    uint64_t pc_tbl_add;
    uint32_t tbl_reg;
    uint64_t t_vt_va;
    uint64_t pc_mov8;
    uint64_t pc_b0;
    uint64_t pc_b1;
    uint64_t pc_b2;
    uint64_t pc_str8;
} ChiMcBlock;

static int find_garnet_mc_block(const uint8_t *buf, size_t sz, ChiMcBlock *out) {
    uint64_t phoff = rd64(buf + 32);
    uint16_t phent = rd16(buf + 54);
    uint16_t phnum = rd16(buf + 56);
    int matches = 0;
    ChiMcBlock found;
    memset(&found, 0, sizeof(found));

    for (uint16_t i = 0; i < phnum; i++) {
        uint64_t po = phoff + (uint64_t)i * phent;
        if (rd32(buf + po) != 1 || !(rd32(buf + po + 4) & 1)) continue;
        uint64_t p_off = rd64(buf + po + 8);
        uint64_t p_va  = rd64(buf + po + 16);
        uint64_t p_fsz = rd64(buf + po + 32);
        if (p_fsz < 256 || p_off + p_fsz > sz) continue;
        uint64_t end = p_off + p_fsz - 256;

        for (uint64_t f = p_off; f <= end; f += 4) {
            uint64_t va = p_va + (f - p_off);
            uint64_t page = 0; uint32_t rd = 0;
            if (!arm64_dec_adrp(va, rd32(buf + f), &page, &rd)) continue;
            /* Check next 1..2 insns for add rd, rd, #imm12 -> "MultiCamera" */
            int is_mc = 0;
            for (uint64_t k = 1; k <= 2; k++) {
                uint32_t wk = rd32(buf + f + k * 4);
                if ((wk & 0xFFC003FFu) == (0x91000000u | (rd << 5) | rd)) {
                    uint64_t target = page + ((wk >> 10) & 0xFFFu);
                    if (elf_cstr_eq(buf, sz, target, "MultiCamera")) { is_mc = 1; break; }
                }
            }
            if (!is_mc) continue;

            /* 1. str dN, [x0, #8] within +8..+24 */
            uint64_t pc_strd = 0; uint32_t orig_strd = 0;
            for (uint64_t d = 8; d <= 24; d += 4) {
                uint32_t w = rd32(buf + f + d);
                if ((w & 0xFFFFFFE0u) == 0xFD000400u) { pc_strd = f + d; orig_strd = w; break; }
            }
            if (!pc_strd) continue;

            /* 2. adrp+add rTbl -> t_mc ([UW(2,1,2), Wide(0,1,0)]) with t_vt=t_mc+0x40 ([Wide(0,1,0), UW(2,1,2), Front(1,1,1)]) */
            uint64_t pc_ta = 0, pc_td = 0, t_vt_va = 0; uint32_t rtbl = 0;
            for (uint64_t d = 24; d <= 80; d += 4) {
                uint64_t tpage = 0; uint32_t trd = 0;
                if (!arm64_dec_adrp(va + d, rd32(buf + f + d), &tpage, &trd)) continue;
                for (uint64_t k = 1; k <= 3; k++) {
                    uint32_t wk = rd32(buf + f + d + k * 4);
                    if ((wk & 0xFFC003FFu) == (0x91000000u | (trd << 5) | trd)) {
                        uint64_t t_mc_va = tpage + ((wk >> 10) & 0xFFFu);
                        uint64_t t_mc_fo = elf_vaddr_to_file(buf, sz, t_mc_va);
                        if (t_mc_fo != (uint64_t)-1 &&
                            is_cam_entry(buf, sz, t_mc_fo + 0x00, "UltrawidePhysicalCam", 2, 1, 2) &&
                            is_cam_entry(buf, sz, t_mc_fo + 0x20, "WidePhysicalCam", 0, 1, 0) &&
                            is_cam_entry(buf, sz, t_mc_fo + 0x40, "WidePhysicalCam", 0, 1, 0) &&
                            is_cam_entry(buf, sz, t_mc_fo + 0x60, "UltrawidePhysicalCam", 2, 1, 2) &&
                            is_cam_entry(buf, sz, t_mc_fo + 0x80, "FrontPhysicalCam", 1, 1, 1)) {
                            pc_ta = f + d; pc_td = f + d + k * 4; rtbl = trd; t_vt_va = t_mc_va + 0x40;
                        }
                        break;
                    }
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

            /* 4. adrp x8, <bss>; add x8, x8, #imm1; add x8, x8, #imm2; ... mov w1,#0x40 ... str x8, [x0], #8 */
            uint64_t pc_b0 = 0, pc_b1 = 0, pc_b2 = 0, pc_str8 = 0;
            int has_mov64 = 0;
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
                /* Confirm str x8, [x0], #8 within +4..+32 after b2, and mov w1, #0x40 between q and str8 */
                for (uint64_t s = 4; s <= 32; s += 4) {
                    if (rd32(buf + b2 + s) == 0xF8008408u) { pc_str8 = b2 + s; break; }
                }
                if (!pc_str8) continue;
                for (uint64_t m = q; m < pc_str8; m += 4) {
                    if (rd32(buf + m) == 0x52800801u) { has_mov64 = 1; break; }
                }
                if (!has_mov64) { pc_str8 = 0; continue; }
                /* Confirm VTCamera string loaded near pc_b0..pc_str8+24 (HOS4 hoists it before pc_str8) */
                int has_vt = 0;
                uint64_t vt_end = pc_str8 + 24;
                if (vt_end > p_off + p_fsz - 16) vt_end = p_off + p_fsz - 16;
                for (uint64_t v = q; v <= vt_end; v += 4) {
                    uint64_t vpage = 0; uint32_t vrd = 0;
                    if (!arm64_dec_adrp(p_va + (v - p_off), rd32(buf + v), &vpage, &vrd)) continue;
                    for (uint64_t k = 1; k <= 3; k++) {
                        uint32_t wk = rd32(buf + v + k * 4);
                        if ((wk & 0xFFC003FFu) == (0x91000000u | (vrd << 5) | vrd)) {
                            if (elf_cstr_eq(buf, sz, vpage + ((wk >> 10) & 0xFFFu), "VTCamera")) { has_vt = 1; break; }
                        }
                    }
                    if (has_vt) break;
                }
                if (!has_vt) { pc_str8 = 0; continue; }
                pc_b0 = q; pc_b1 = b1; pc_b2 = b2;
                break;
            }
            if (!pc_b0) continue;

            found.pc_strd = pc_strd;
            found.orig_strd = orig_strd;
            found.pc_tbl_adrp = pc_ta;
            found.pc_tbl_add = pc_td;
            found.tbl_reg = rtbl;
            found.t_vt_va = t_vt_va;
            found.pc_mov8 = pc_mov8;
            found.pc_b0 = pc_b0;
            found.pc_b1 = pc_b1;
            found.pc_b2 = pc_b2;
            found.pc_str8 = pc_str8;
            matches++;
            if (matches > 1) return 2;
        }
    }
    if (matches == 1) *out = found;
    return matches;
}

static int find_pair_arr(const uint8_t *buf, size_t sz, uint64_t *out_va) {
    uint64_t phoff = rd64(buf + 32);
    uint16_t phent = rd16(buf + 54);
    uint16_t phnum = rd16(buf + 56);
    uint64_t strict_va = 0, base_va = 0;
    int strict_cnt = 0, base_cnt = 0;

    for (uint16_t i = 0; i < phnum; i++) {
        uint64_t po = phoff + (uint64_t)i * phent;
        if (rd32(buf + po) != 1 || rd32(buf + po + 4) != 6) continue; /* RW PT_LOAD (.data.rel.ro) */
        uint64_t p_off = rd64(buf + po + 8);
        uint64_t p_va  = rd64(buf + po + 16);
        uint64_t p_fsz = rd64(buf + po + 32);
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
            if (!is_cam_entry(buf, sz, a - 0x20, "WidePhysicalCam", 0, 1, 0)) continue;
            if (!is_cam_entry(buf, sz, a + 0x28, "WidePhysicalCam", 0, 1, 0)) continue;
            if (!is_cam_entry(buf, sz, a + 0x48, "UltrawidePhysicalCam", 2, 1, 2)) continue;
            if (!is_cam_entry(buf, sz, a + 0x68, "FrontPhysicalCam", 1, 1, 1)) continue;

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

/* NUL-delimited CamX override keys -> zero pointer relocations! */
static const char CAMX_KEYS_FLAT[] =
    "videoSATSupportedRange=771\0"
    "multiCameraVREnable=TRUE\0"
    "enableSW2DLUT=TRUE\0"
    "multiCameraSWFrameSyncInterval=5\0"
    "multiCameraHWSyncMask=0\0"
    "multiCameraFPSMatchMask=0x1\0"
    "multiCameraFrameSyncMask=0x1\0"
    "multiCameraExposureTimeMatch=TRUE\0"
    "enableExposureTimeMatch=TRUE\0"
    "multiCameraSWFrameSyncType=SOFSync\0"
    "isNCSFlushFenceJobEnabled=TRUE\0"
    "disableTFRefinement=FALSE\0"
    "enableLDC=TRUE\0";
#define NCAMX_KEYS 13

static const char *get_camx_key(int idx) {
    const char *p = CAMX_KEYS_FLAT;
    for (int i = 0; i < idx; i++) p += xstrlen(p) + 1;
    return p;
}

static int key_matches_line(const char *line, size_t llen, const char *kv) {
    size_t klen = 0;
    while (kv[klen] && kv[klen] != '=') klen++;
    if (llen <= klen) return 0;
    return memcmp(line, kv, klen) == 0 && line[klen] == '=';
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

    if (xstrcmp(kind, "camera_qcom") == 0) {
        if (!elf_has_str(buf, sz, "CamX") || !elf_has_str(buf, sz, "ConstructDefaultRequestSettings")) {
            free_buf(buf, sz + 4096);
            xputs(1, "[SKIP] camera_qcom: CamX marker not found (unsupported vendor)\n");
            return 3;
        }
        size_t ncam = sizeof(CAMQCOM_RULES) / sizeof(CAMQCOM_RULES[0]);
        for (size_t i = 0; i < ncam; i++) {
            size_t h = 0;
            if (find_unique(buf, sz, RULE_BLOB + CAMQCOM_RULES[i].ctx_off, CAMQCOM_RULES[i].ctx_len, &h) == 1 &&
                (h & 3) == 0 && is_in_rx_seg(buf, sz, h, CAMQCOM_RULES[i].ctx_len)) {
                memcpy(buf + h + CAMQCOM_RULES[i].rel, RULE_BLOB + CAMQCOM_RULES[i].rep_off, CAMQCOM_RULES[i].rep_len);
                edits++;
            }
        }
        /* TF20/TF201 Luma-Only temporal blend = 0 (keep Chroma temporal denoising 100% active) */
        size_t pos = 0;
        while ((pos = find_bytes(buf, sz, TF201_LUMA_CTX, sizeof(TF201_LUMA_CTX), pos)) != (size_t)-1) {
            if ((pos & 3) == 0 && is_in_rx_seg(buf, sz, pos, sizeof(TF201_LUMA_CTX))) {
                memcpy(buf + pos + 16, TF201_LUMA_REP, sizeof(TF201_LUMA_REP));
                edits++;
            }
            pos += sizeof(TF201_LUMA_CTX);
        }
        /* ANR101 Luma-Only spatial bypass RX cave hook (keep Chroma CLPF + CNR 100% active) */
        size_t hpos = 0;
        if (find_unique(buf, sz, ANR101_HOOK_CTX, sizeof(ANR101_HOOK_CTX), &hpos) == 1 &&
            (hpos & 3) == 0 && is_in_rx_seg(buf, sz, hpos, sizeof(ANR101_HOOK_CTX))) {
            uint64_t hook_pc = hpos + 12;
            uint64_t cave = 0;
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
    } else if (xstrcmp(kind, "mcreprocrt") == 0) {
        size_t h = 0;
        if (elf_has_str(buf, sz, "MultiCameraSATWithQSAT") &&
            find_unique(buf, sz, RULE_BLOB + MCREPROCRT_RULES[0].ctx_off, MCREPROCRT_RULES[0].ctx_len, &h) == 1 &&
            (h & 3) == 0 && is_in_rx_seg(buf, sz, h, MCREPROCRT_RULES[0].ctx_len)) {
            /* Decode ADRP+ADD at h+8 / h+12 and verify exact string targets in .rodata */
            uint64_t pc_vaddr = elf_file_to_vaddr(buf, sz, h + 8);
            uint32_t i_adrp = rd32(buf + h + 8);
            uint32_t i_add  = rd32(buf + h + 12);
            if (pc_vaddr != (uint64_t)-1 && (i_adrp & 0x9F000000u) == 0x90000000u && (i_add & 0xFFC00000u) == 0x91000000u) {
                int64_t imm = (int64_t)((((i_adrp >> 5) & 0x7FFFFu) << 2) | ((i_adrp >> 29) & 3u));
                if (imm & 0x100000) imm -= 0x200000;
                uint64_t page = (pc_vaddr & ~0xFFFULL) + (uint64_t)(imm << 12);
                uint32_t old_imm12 = (i_add >> 10) & 0xFFFu;
                uint32_t new_add = (i_add & ~(0xFFu << 8)) | ((uint32_t)RULE_BLOB[MCREPROCRT_RULES[0].rep_off] << 8);
                uint32_t new_imm12 = (new_add >> 10) & 0xFFFu;
                uint64_t old_foff = elf_vaddr_to_file(buf, sz, page + old_imm12);
                uint64_t new_foff = elf_vaddr_to_file(buf, sz, page + new_imm12);
                if (old_foff != (uint64_t)-1 && new_foff != (uint64_t)-1 &&
                    old_foff + 25 <= sz && new_foff + 23 <= sz &&
                    memcmp(buf + old_foff, "MultiCameraVideoSATNoGPU", 25) == 0 &&
                    memcmp(buf + new_foff, "MultiCameraSATWithQSAT", 23) == 0) {
                    memcpy(buf + h + MCREPROCRT_RULES[0].rel, RULE_BLOB + MCREPROCRT_RULES[0].rep_off, MCREPROCRT_RULES[0].rep_len);
                    edits++;
                }
            }
        }
    } else if (xstrcmp(kind, "feature2_generic") == 0) {
        size_t hpos = 0;
        if (elf_has_str(buf, sz, "Feature2Generic") &&
            find_unique(buf, sz, GEN_HOOK_CTX, sizeof(GEN_HOOK_CTX), &hpos) == 1 &&
            (hpos & 3) == 0 && is_in_rx_seg(buf, sz, hpos, sizeof(GEN_HOOK_CTX))) {
            uint64_t hook_pc = hpos + 12;
            uint64_t cave = 0;
            if (alloc_rx_cave_for(buf, sz, hook_pc, 16, 0x20, &cave) == 0) {
                static const uint8_t cbody[16] = {
                    0xea,0x03,0x08,0xaa, 0x28,0x01,0x40,0xb9,
                    0xaa,0x1e,0xc3,0xb4, 0x56,0x21,0x5e,0xf9
                };
                memcpy(buf + cave, cbody, 16);
                wr32(buf + cave + 16, arm64_b(cave + 16, hook_pc + 8, 0));
                wr32(buf + hook_pc, arm64_b(hook_pc, cave, 0));
                edits++;
            }
        }
    } else if (xstrcmp(kind, "chi_override") == 0) {
        /* Universal atomic 15/15 chi_override patcher (HyperOS 1/2/3, OOS16, Lunaris/AOSP).
         * Every single rule, dynamic ADRP+ADD target, transition pair, and RX cave hook must
         * resolve uniquely before a single byte is modified. */
        if (elf_has_str(buf, sz, "chi_hal_override_entry")) {
            size_t z65_hit = 0;
            size_t fixed_hits[4];
            int all_ok = (find_unique(buf, sz, CHI_ZOOM65_CTX, sizeof(CHI_ZOOM65_CTX), &z65_hit) == 1);
            for (size_t i = 0; all_ok && i < 4; i++) {
                if (find_unique(buf, sz, RULE_BLOB + CHI_FIXED_RULES[i].ctx_off, CHI_FIXED_RULES[i].ctx_len, &fixed_hits[i]) != 1)
                    all_ok = 0;
            }

            /* HOOK1 + RULE12: CameraIdXiaomi logical camera list (BL immediates masked) */
            static const uint32_t h1_words[13] = {
                0x52800801u, 0xf9000a9fu, 0x94000000u, 0x528007f6u,
                0xaa0003f5u, 0x52800081u, 0xb9002016u, 0x52800020u,
                0x94000000u, 0xf90016a0u, 0xaa1403e8u, 0xb8004416u,
                0xa90302a0u
            };
            static const uint32_t h1_masks[13] = {
                0xffffffffu, 0xffffffffu, 0xfc000000u, 0xffffffffu,
                0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu,
                0xfc000000u, 0xffffffffu, 0xffffffffu, 0xffffffffu,
                0xffffffffu
            };
            /* HOOK2: FinalizeFeatureInstances SWRemosaic guard (verifies [x29, #-0xe0], BL masked) */
            static const uint32_t h2_words[9] = {
                0xf85203a0u, 0xd10243a2u, 0xa9377fbfu, 0xf81803bfu,
                0x94000000u, 0x2a0003f6u, 0x34000120u, 0xa941d353u,
                0xeb13029fu
            };
            static const uint32_t h2_masks[9] = {
                0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu,
                0xfc000000u, 0xffffffffu, 0xffffffffu, 0xffffffffu,
                0xffffffffu
            };

            uint64_t h1_blk = 0, h2_blk = 0, pair_arr_va = 0;
            ChiMcBlock mc;
            memset(&mc, 0, sizeof(mc));
            if (all_ok &&
                find_masked_unique_rx(buf, sz, h1_words, h1_masks, 13, &h1_blk) == 1 &&
                find_masked_unique_rx(buf, sz, h2_words, h2_masks, 9, &h2_blk) == 1 &&
                find_garnet_mc_block(buf, sz, &mc) == 1 &&
                find_pair_arr(buf, sz, &pair_arr_va) == 1) {
                uint64_t pc_tbl_adrp_va = elf_file_to_vaddr(buf, sz, mc.pc_tbl_adrp);
                uint64_t pc_b0_va       = elf_file_to_vaddr(buf, sz, mc.pc_b0);
                uint32_t insn_tbl_adrp = 0, insn_b0_adrp = 0;
                uint64_t h1_pc = h1_blk + 44;
                uint64_t h2_pc = h2_blk + 20;
                uint64_t cave_base = 0;
                if (pc_tbl_adrp_va != (uint64_t)-1 && pc_b0_va != (uint64_t)-1 &&
                    arm64_enc_adrp(pc_tbl_adrp_va, mc.t_vt_va, mc.tbl_reg, &insn_tbl_adrp) &&
                    arm64_enc_adrp(pc_b0_va, pair_arr_va, 8, &insn_b0_adrp) &&
                    alloc_rx_cave_for(buf, sz, h1_pc, 32, 0x40, &cave_base) == 0) {
                    uint64_t cave2 = cave_base + 0x10;
                    uint64_t d2 = (cave2 > h2_pc) ? (cave2 - h2_pc) : (h2_pc - cave2);
                    if (d2 < 0x07F00000ULL) {
                        /* 1. 0.65f -> 0.6001f zoom threshold */
                        memcpy(buf + z65_hit + 8, CHI_ZOOM65_REP, 3);
                        edits++;
                        /* 2..5. Fixed-pattern unique rules */
                        for (size_t i = 0; i < 4; i++) {
                            memcpy(buf + fixed_hits[i] + CHI_FIXED_RULES[i].rel,
                                   RULE_BLOB + CHI_FIXED_RULES[i].rep_off, CHI_FIXED_RULES[i].rep_len);
                            edits++;
                        }
                        /* 6..12. Dynamic garnet MultiCamera logical config (7 edits) */
                        wr32(buf + mc.pc_strd,     0xBD000800u | (mc.orig_strd & 0x1Fu));
                        wr32(buf + mc.pc_tbl_adrp, insn_tbl_adrp);
                        wr32(buf + mc.pc_tbl_add,  arm64_enc_add12(mc.tbl_reg, mc.tbl_reg, (uint32_t)(mc.t_vt_va & 0xFFFu)));
                        wr32(buf + mc.pc_mov8,     0x52800201u); /* mov w1, #0x10 */
                        wr32(buf + mc.pc_b0,       insn_b0_adrp);
                        wr32(buf + mc.pc_b1,       arm64_enc_add12(8, 8, (uint32_t)(pair_arr_va & 0xFFFu)));
                        wr32(buf + mc.pc_b2,       0x3DC00100u); /* ldr q0, [x8] */
                        wr32(buf + mc.pc_str8,     0x3C810400u); /* str q0, [x0], #16 */
                        edits += 7;
                        /* 13. CameraIdXiaomi roleId 0x3f -> 0x3e */
                        wr32(buf + h1_blk + 12, 0x528007d6u); /* mov w22, #0x3e */
                        edits++;
                        /* 14. HOOK1 RX cave: write 0x3f into logical camera array */
                        static const uint8_t c1[12] = {
                            0xe9,0x07,0x80,0x52, 0x09,0x44,0x00,0xb8, 0xc0,0x03,0x5f,0xd6
                        };
                        memcpy(buf + cave_base, c1, 12);
                        wr32(buf + h1_pc, arm64_b(h1_pc, cave_base, 1));
                        edits++;
                        /* 15. HOOK2 RX cave: FinalizeFeatureInstances SWRemosaic guard */
                        static const uint8_t c2[28] = {
                            0xc0,0x00,0x00,0x34, 0xa8,0x03,0x52,0xf8,
                            0x08,0x49,0x41,0xf9, 0x08,0x01,0x40,0xb9,
                            0x1f,0x89,0x00,0x71, 0xe0,0x03,0x80,0x1a,
                            0xf6,0x03,0x00,0x2a
                        };
                        memcpy(buf + cave2, c2, 28);
                        wr32(buf + cave2 + 28, arm64_b(cave2 + 28, h2_pc + 4, 0));
                        wr32(buf + h2_pc, arm64_b(h2_pc, cave2, 0));
                        edits++;
                    }
                }
            }
        }
    } else {
        free_buf(buf, sz + 4096);
        xputs(2, "ERR: unknown HAL kind\n");
        return 2;
    }

    if (edits == 0) {
        free_buf(buf, sz + 4096);
        xputs(1, "[SKIP] "); xputs(1, kind); xputs(1, ": 0 safe matches (keeping stock)\n");
        return 3;
    }

    int rc = write_file_all(dst, buf, sz);
    free_buf(buf, sz + 4096);
    if (rc != 0) { xputs(2, "ERR: failed to write HAL output\n"); return 1; }
    xputs(1, "[OK] "); xputs(1, kind); xputs(1, ": applied "); xput_u32(1, (uint32_t)edits); xputs(1, " edits\n");
    return 0;
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

static const uint8_t *dex_get_str(const uint8_t *dex, size_t dlen, uint32_t str_off, uint32_t idx, size_t *out_len) {
    size_t so = (size_t)str_off + (size_t)idx * 4;
    if (so + 4 > dlen) return (const uint8_t *)0;
    size_t off = rd32(dex + so);
    if (off >= dlen) return (const uint8_t *)0;
    read_uleb128(dex, dlen, &off);
    size_t p = off;
    while (p < dlen && dex[p] != 0) p++;
    *out_len = p - off;
    return dex + off;
}

static int apply_dex_meth_rules(uint8_t *dex, size_t dlen) {
    if (dlen < 112 || memcmp(dex, "dex\n", 4) != 0) return 0;
    uint32_t str_off  = rd32(dex + 60);
    uint32_t type_off = rd32(dex + 68);
    uint32_t meth_off = rd32(dex + 92);
    uint32_t cls_sz   = rd32(dex + 96);
    uint32_t cls_off  = rd32(dex + 100);
    int edits = 0;
    size_t nrules = sizeof(DEX_METH_RULES) / sizeof(DEX_METH_RULES[0]);

    for (uint32_t i = 0; i < cls_sz; i++) {
        size_t co = (size_t)cls_off + (size_t)i * 32;
        if (co + 32 > dlen) break;
        uint32_t cidx = rd32(dex + co);
        uint32_t cdata = rd32(dex + co + 24);
        if (cdata == 0 || (size_t)type_off + (size_t)cidx * 4 + 4 > dlen) continue;
        uint32_t csidx = rd32(dex + type_off + (size_t)cidx * 4);
        size_t clen = 0;
        const uint8_t *cstr = dex_get_str(dex, dlen, str_off, csidx, &clen);
        if (!cstr) continue;

        int match_cls = 0;
        for (size_t r = 0; r < nrules; r++) {
            if (clen == DEX_METH_RULES[r].cls_len &&
                memcmp(cstr, RULE_BLOB + DEX_METH_RULES[r].cls_off, clen) == 0) {
                match_cls = 1; break;
            }
        }
        if (!match_cls) continue;

        size_t p = cdata;
        uint32_t sf = read_uleb128(dex, dlen, &p);
        uint32_t if_ = read_uleb128(dex, dlen, &p);
        uint32_t dm = read_uleb128(dex, dlen, &p);
        uint32_t vm = read_uleb128(dex, dlen, &p);
        for (uint32_t k = 0; k < sf + if_; k++) { read_uleb128(dex, dlen, &p); read_uleb128(dex, dlen, &p); }
        for (int grp = 0; grp < 2; grp++) {
            uint32_t cnt = (grp == 0) ? dm : vm;
            uint32_t midx = 0;
            for (uint32_t k = 0; k < cnt; k++) {
                midx += read_uleb128(dex, dlen, &p);
                read_uleb128(dex, dlen, &p);
                uint32_t code_off = read_uleb128(dex, dlen, &p);
                if (!code_off || (size_t)meth_off + (size_t)midx * 8 + 8 > dlen) continue;
                uint32_t mnidx = rd32(dex + meth_off + (size_t)midx * 8 + 4);
                size_t mlen = 0;
                const uint8_t *mstr = dex_get_str(dex, dlen, str_off, mnidx, &mlen);
                if (!mstr) continue;
                for (size_t r = 0; r < nrules; r++) {
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

static int is_classes_dex(const uint8_t *nm, uint16_t nlen) {
    if (nlen < 11 || nlen > 14) return 0;
    if (memcmp(nm, "classes", 7) != 0) return 0;
    if (memcmp(nm + nlen - 4, ".dex", 4) != 0) return 0;
    return 1;
}

static int parse_classes_dex_num(const uint8_t *nm, uint16_t nlen) {
    if (!is_classes_dex(nm, nlen)) return 0;
    if (nlen == 11) return 1; /* "classes.dex" */
    int v = 0;
    for (uint16_t i = 7; i < nlen - 4; i++) {
        if (nm[i] < '0' || nm[i] > '9') return 0;
        v = v * 10 + (nm[i] - '0');
    }
    return v;
}

/* Dalvik instruction widths in 16-bit code units (zero relocations!) */
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

static int dex_str_eq(const uint8_t *dex, size_t dlen, uint32_t str_off, uint32_t str_cnt, uint32_t idx, const char *s) {
    if (idx >= str_cnt) return 0;
    size_t len = 0;
    const uint8_t *p = dex_get_str(dex, dlen, str_off, idx, &len);
    size_t slen = xstrlen(s);
    return (p && len == slen && memcmp(p, s, slen) == 0);
}

static uint32_t dex_find_str_id(const uint8_t *dex, size_t dlen, const char *s) {
    uint32_t str_cnt = rd32(dex + 56), str_off = rd32(dex + 60);
    size_t slen = xstrlen(s);
    for (uint32_t i = 0; i < str_cnt; i++) {
        size_t len = 0;
        const uint8_t *p = dex_get_str(dex, dlen, str_off, i, &len);
        if (p && len == slen && memcmp(p, s, slen) == 0) return i;
    }
    return 0xFFFFFFFFu;
}

static uint32_t dex_find_meth_id(const uint8_t *dex, size_t dlen, const char *cls, const char *mth) {
    uint32_t str_cnt = rd32(dex + 56), str_off = rd32(dex + 60);
    uint32_t type_cnt = rd32(dex + 64), type_off = rd32(dex + 68);
    uint32_t meth_cnt = rd32(dex + 88), meth_off = rd32(dex + 92);
    uint32_t cls_sid = dex_find_str_id(dex, dlen, cls);
    uint32_t mth_sid = dex_find_str_id(dex, dlen, mth);
    if (cls_sid == 0xFFFFFFFFu || mth_sid == 0xFFFFFFFFu) return 0xFFFFFFFFu;
    uint32_t cls_tid = 0xFFFFFFFFu;
    for (uint32_t t = 0; t < type_cnt; t++) {
        if ((size_t)type_off + (size_t)t * 4 + 4 > dlen) break;
        if (rd32(dex + type_off + (size_t)t * 4) == cls_sid) { cls_tid = t; break; }
    }
    if (cls_tid == 0xFFFFFFFFu) return 0xFFFFFFFFu;
    for (uint32_t m = 0; m < meth_cnt; m++) {
        size_t mo = (size_t)meth_off + (size_t)m * 8;
        if (mo + 8 > dlen) break;
        if (rd16(dex + mo) == cls_tid && rd32(dex + mo + 4) == mth_sid) return m;
    }
    return 0xFFFFFFFFu;
}

static int dex_code_has_str(const uint8_t *code, size_t clen, uint32_t sid, size_t *out_pos) {
    if (sid == 0xFFFFFFFFu) return 0;
    size_t i = 0;
    while (i < clen) {
        size_t st = dex_insn_step(code, clen, i);
        if (st == 0 || i + st > clen) break;
        if (code[i] == 0x1a && st == 4 && rd16(code + i + 2) == sid) { if (out_pos) *out_pos = i; return 1; }
        if (code[i] == 0x1b && st == 6 && rd32(code + i + 2) == sid) { if (out_pos) *out_pos = i; return 1; }
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

/* ProGuard-independent dynamic DEX patcher (HyperOS 4 & universal MiuiCamera support).
 * Preserves native AlgoUp / LocalParallel / Garnet device features while fixing SAT roleId,
 * VideoSAT/PhotoSAT enablement, anti-oversharpening, 0.6x UW LDC, and video bitrates. */
static int apply_dex_dynamic(uint8_t *dex, size_t dlen, int *out_sat_anchors) {
    if (dlen < 112 || memcmp(dex, "dex\n", 4) != 0) return 0;
    uint32_t str_cnt  = rd32(dex + 56), str_off  = rd32(dex + 60);
    uint32_t type_off = rd32(dex + 68);
    uint32_t meth_off = rd32(dex + 92);
    uint32_t cls_sz   = rd32(dex + 96), cls_off  = rd32(dex + 100);
    int edits = 0;

    uint32_t sid_getsat  = dex_find_str_id(dex, dlen, "Warning: getSATCameraId(): #init() failed.");
    uint32_t sid_hassat  = dex_find_str_id(dex, dlen, "Warning: hasSATCamera(): #init() failed.");
    uint32_t sid_vsatdbg = dex_find_str_id(dex, dlen, "supportVideoSAT: debug prop force true");
    uint32_t sid_satldc  = dex_find_str_id(dex, dlen, "setSATUltraWideLDC: ");
    uint32_t sid_h264    = dex_find_str_id(dex, dlen, "setupRecorder: H264 bitrate = ");
    uint32_t sid_hevc    = dex_find_str_id(dex, dlen, "HEVC: no pre-defined bitrate for ");
    uint32_t mid_sharp   = dex_find_meth_id(dex, dlen, "Lcom/android/camera2/compat/MiCameraCompatBaseImpl;", "applySharpness");
    uint32_t mid_uwldc   = dex_find_meth_id(dex, dlen, "Lcom/android/camera2/compat/MiCameraCompatBaseImpl;", "applyUltraWideLDC");

    uint32_t sharp_def_mid = 0xFFFFFFFFu;
    uint32_t sat_cls_idx = 0xFFFFFFFFu;
    uint16_t sat_dbg_fld = 0;
    uint8_t  have_sat_dbg = 0;
    uint32_t u0_coff = 0, h0_coff = 0;
    int u0_cnt = 0, h0_cnt = 0;

    /* Pass 1: single-method string/API patches & capture cross-method anchors */
    for (uint32_t i = 0; i < cls_sz; i++) {
        size_t co = (size_t)cls_off + (size_t)i * 32;
        if (co + 32 > dlen) break;
        uint32_t cidx = rd32(dex + co), cdata = rd32(dex + co + 24);
        if (!cdata || (size_t)type_off + (size_t)cidx * 4 + 4 > dlen) continue;
        uint32_t csidx = rd32(dex + type_off + (size_t)cidx * 4);
        int is_cam2mod = dex_str_eq(dex, dlen, str_off, str_cnt, csidx, "Lcom/android/camera/module/Camera2Module;");
        int is_basemod = dex_str_eq(dex, dlen, str_off, str_cnt, csidx, "Lcom/android/camera/module/BaseModule;");

        size_t p = cdata;
        uint32_t sf = read_uleb128(dex, dlen, &p), if_ = read_uleb128(dex, dlen, &p);
        uint32_t dm = read_uleb128(dex, dlen, &p), vm  = read_uleb128(dex, dlen, &p);
        for (uint32_t k = 0; k < sf + if_; k++) { read_uleb128(dex, dlen, &p); read_uleb128(dex, dlen, &p); }
        for (int grp = 0; grp < 2; grp++) {
            uint32_t cnt = (grp == 0) ? dm : vm, midx = 0;
            for (uint32_t k = 0; k < cnt; k++) {
                midx += read_uleb128(dex, dlen, &p);
                read_uleb128(dex, dlen, &p);
                uint32_t coff = read_uleb128(dex, dlen, &p);
                if (!coff || (size_t)coff + 16 > dlen) continue;
                uint16_t tries = rd16(dex + coff + 6);
                uint32_t insns_sz = rd32(dex + coff + 12);
                size_t clen = (size_t)insns_sz * 2;
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
                if (is_cam2mod && clen >= 24 && clen <= 80 && (size_t)meth_off + (size_t)midx * 8 + 8 <= dlen) {
                    uint32_t mnidx = rd32(dex + meth_off + (size_t)midx * 8 + 4);
                    if (dex_str_eq(dex, dlen, str_off, str_cnt, mnidx, "updateSharpness")) {
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
                if (is_basemod && sid_satldc != 0xFFFFFFFFu && (size_t)meth_off + (size_t)midx * 8 + 8 <= dlen) {
                    uint32_t mnidx = rd32(dex + meth_off + (size_t)midx * 8 + 4);
                    size_t spos = 0;
                    if (dex_str_eq(dex, dlen, str_off, str_cnt, mnidx, "updateUltraWideLDC") &&
                        dex_code_has_str(code, clen, sid_satldc, &spos) && spos >= 24) {
                        size_t lo = (spos > 64) ? (spos - 64) : 0;
                        for (size_t j = spos - 6; j >= lo; j -= 2) {
                            if (code[j] == 0x0a && code[j + 1] == 0x00 &&
                                code[j + 2] == 0x38 && code[j + 3] == 0x00 && code[j + 5] == 0x00) {
                                code[j + 2] = 0; code[j + 3] = 0; code[j + 4] = 0; code[j + 5] = 0;
                                edits++; break;
                            }
                            if (j < lo + 2) break;
                        }
                    }
                }
                /* 11. H.264 bitrate boost (20M->40M, 50M->75M) */
                size_t hpos = 0;
                if (tries == 0 && sid_h264 != 0xFFFFFFFFu && clen >= 1024 &&
                    dex_code_has_str(code, clen, sid_h264, &hpos) && hpos >= 80 &&
                    code[hpos - 4] == 0x02 && code[hpos - 3] == 0x02 && code[hpos - 1] == 0x00) {
                    uint8_t vsrc = code[hpos - 2];
                    size_t lo = (hpos > 240) ? (hpos - 240) : 0;
                    size_t start = 0;
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
                                0xda, 0x02, 0x02, 0x02,             /* mul-int/lit8 v2, v2, #2 */
                                0x28, (uint8_t)d1,                  /* goto hpos */
                                0x14, 0x03, 0x00, 0x87, 0x93, 0x03, /* const v3, 60000000 */
                                0x35, 0x32, (uint8_t)(d2 & 0xFF), (uint8_t)(d2 >> 8), /* if-ge v2, v3, hpos */
                                0xda, 0x02, 0x02, 0x03,             /* mul-int/lit8 v2, v2, #3 */
                                0xdb, 0x02, 0x02, 0x02,             /* div-int/lit8 v2, v2, #2 */
                                0x28, (uint8_t)d3,                  /* goto hpos */
                                0x00, 0x00, 0x00, 0x00
                            };
                            memcpy(code + start, patch, 44);
                            memset(code + start + 44, 0, span - 44);
                            edits++;
                        }
                    }
                }
                /* 12. HEVC bitrate 2x boost (15.4M->30.8M, 38.5M->77M) */
                if (tries == 0 && sid_hevc != 0xFFFFFFFFu && clen >= 64 && clen <= 600 &&
                    dex_code_has_str(code, clen, sid_hevc, 0)) {
                    uint8_t ret_op = code[clen - 2], r = code[clen - 1];
                    uint8_t rr = (uint8_t)((r << 4) | r);
                    if (ret_op == 0x0f && r < 16 &&
                        code[clen - 6] == 0x52 && code[clen - 5] == rr &&
                        code[clen - 26] == 0x28 && code[clen - 25] == 0x0c) {
                        uint8_t fid0 = code[clen - 4], fid1 = code[clen - 3];
                        /* Retarget all goto (0x28) pointing at clen-2 -> clen-6 (mul-int/lit8 *2) */
                        size_t j = 0;
                        while (j < clen - 24) {
                            size_t st = dex_insn_step(code, clen, j);
                            if (st == 0 || j + st > clen) break;
                            if (code[j] == 0x28 && st == 2) {
                                int8_t rel = (int8_t)code[j + 1];
                                if ((int)j + (int)rel * 2 == (int)(clen - 2) && rel >= 4) {
                                    code[j + 1] = (uint8_t)(rel - 2);
                                }
                            }
                            j += st;
                        }
                        memset(code + clen - 24, 0, 14);
                        code[clen - 10] = 0x52; code[clen - 9] = rr; code[clen - 8] = fid0; code[clen - 7] = fid1;
                        code[clen - 6]  = 0xda; code[clen - 5] = r;  code[clen - 4] = r;    code[clen - 3] = 0x02;
                        edits++;
                    }
                }
            }
        }
    }

    /* Commit unique single-candidate wrappers (8 & 9) */
    if (u0_cnt == 1) {
        static const uint8_t rep_u0[6] = {0x3b, 0x02, 0x03, 0x00, 0x12, 0x02};
        memcpy(dex + u0_coff + 16 + 10, rep_u0, 6);
        edits++;
    }
    if (h0_cnt == 1) {
        memset(dex + h0_coff + 16 + 30, 0, 4);
        edits++;
    }

    /* Pass 2: cross-method patches (4. isSupportSAT & 7b. default sharpness callee) */
    if (have_sat_dbg && sat_cls_idx < cls_sz) {
        size_t co = (size_t)cls_off + (size_t)sat_cls_idx * 32;
        uint32_t cdata = rd32(dex + co + 24);
        size_t p = cdata;
        uint32_t sf = read_uleb128(dex, dlen, &p), if_ = read_uleb128(dex, dlen, &p);
        uint32_t dm = read_uleb128(dex, dlen, &p), vm  = read_uleb128(dex, dlen, &p);
        for (uint32_t k = 0; k < sf + if_; k++) { read_uleb128(dex, dlen, &p); read_uleb128(dex, dlen, &p); }
        uint32_t sat_coff = 0; int sat_cnt = 0;
        for (int grp = 0; grp < 2; grp++) {
            uint32_t cnt = (grp == 0) ? dm : vm, midx = 0;
            for (uint32_t k = 0; k < cnt; k++) {
                midx += read_uleb128(dex, dlen, &p);
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
        for (uint32_t i = 0; i < cls_sz; i++) {
            size_t co = (size_t)cls_off + (size_t)i * 32;
            if (co + 32 > dlen) break;
            uint32_t cdata = rd32(dex + co + 24);
            if (!cdata) continue;
            size_t p = cdata;
            uint32_t sf = read_uleb128(dex, dlen, &p), if_ = read_uleb128(dex, dlen, &p);
            uint32_t dm = read_uleb128(dex, dlen, &p), vm  = read_uleb128(dex, dlen, &p);
            for (uint32_t k = 0; k < sf + if_; k++) { read_uleb128(dex, dlen, &p); read_uleb128(dex, dlen, &p); }
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
                            memcpy(code, zret, 8);
                            edits++;
                        }
                    }
                }
            }
        }
    }

    /* 5 & 6. Universal VideoSAT maxZoom 12.0f->10.0f & ~335ms spline timing (Common) */
    edits += apply_pat_rules(dex, dlen, &DEX_PAT_RULES[26], 2);
    return edits;
}

static int cmd_patch_apk(const char *src, const char *dst) {
    size_t sz = 0;
    uint8_t *buf = read_file_all(src, &sz);
    if (!buf) { xputs(2, "ERR: cannot read APK\n"); return 1; }

    size_t search_start = (sz > 65557) ? (sz - 65557) : 0;
    size_t eocd = (size_t)-1;
    for (size_t i = sz - 22; i >= search_start; i--) {
        if (buf[i] == 0x50 && buf[i+1] == 0x4b && buf[i+2] == 0x05 && buf[i+3] == 0x06) {
            uint16_t clen = rd16(buf + i + 20);
            if (i + 22 + clen == sz) { eocd = i; break; }
        }
        if (i == search_start) break;
    }
    if (eocd == (size_t)-1) { free_buf(buf, sz + 4096); xputs(2, "ERR: invalid APK (no EOCD)\n"); return 1; }

    uint32_t cd_sz = rd32(buf + eocd + 12);
    uint32_t cd_off = rd32(buf + eocd + 16);
    if ((size_t)cd_off + cd_sz > sz) { free_buf(buf, sz + 4096); return 1; }

    int total_edits = 0, meth_edits = 0, dex_patched = 0;
    int max_dex_num = 1, has_smoother_dex = 0, smoother_hooked = 0;
    size_t pos = cd_off;
    while (pos + 46 <= (size_t)cd_off + cd_sz) {
        if (rd32(buf + pos) != 0x02014b50u) break;
        uint16_t comp = rd16(buf + pos + 10);
        uint32_t csz  = rd32(buf + pos + 20);
        uint16_t nlen = rd16(buf + pos + 28);
        uint16_t elen = rd16(buf + pos + 30);
        uint16_t clen = rd16(buf + pos + 32);
        uint32_t lh_off = rd32(buf + pos + 42);
        const uint8_t *nm = buf + pos + 46;

        if (comp == 0 && is_classes_dex(nm, nlen) && (size_t)lh_off + 30 <= sz) {
            int dnum = parse_classes_dex_num(nm, nlen);
            if (dnum > max_dex_num) max_dex_num = dnum;
            uint16_t lnlen = rd16(buf + lh_off + 26);
            uint16_t lelen = rd16(buf + lh_off + 28);
            size_t doff = (size_t)lh_off + 30 + lnlen + lelen;
            if (doff + csz <= sz) {
                uint8_t *dex = buf + doff;
                if (csz == sizeof(ZOOM_SMOOTHER_DEX) && memcmp(dex, ZOOM_SMOOTHER_DEX, 32) == 0) {
                    has_smoother_dex = 1;
                }
                int pe = apply_pat_rules(dex, csz, DEX_PAT_RULES, sizeof(DEX_PAT_RULES)/sizeof(DEX_PAT_RULES[0]));
                int me = apply_dex_meth_rules(dex, csz);
                int e = pe + me;
                if (find_bytes(dex, csz, RULE_BLOB + 2570, 20, 0) != (size_t)-1) {
                    smoother_hooked = 1;
                }
                if (e > 0) {
                    sha1_calc(dex + 32, csz - 32, dex + 12);
                    wr32(dex + 8, adler32_calc(dex + 12, csz - 12));
                    uint32_t ncrc = crc32_calc(dex, csz);
                    wr32(buf + lh_off + 14, ncrc);
                    wr32(buf + pos + 16, ncrc);
                    total_edits += e;
                    meth_edits += me;
                    dex_patched++;
                }
            }
        }
        pos += 46 + nlen + elen + clen;
    }

    /* Tier 1: Exact OOS16/OS3 ported MiuiCamera full patch (>=20 edits & >=2 exact method anchors).
     * Tier 2: ProGuard-independent dynamic string/API-anchored patch (HyperOS 4 & native MiuiCamera). */
    if (total_edits < 20 || meth_edits < 2) {
        free_buf(buf, sz + 4096);
        buf = read_file_all(src, &sz);
        if (!buf) { xputs(2, "ERR: cannot re-read APK\n"); return 1; }
        int dyn_edits = 0, sat_anchors = 0, dyn_dex = 0;
        pos = cd_off;
        while (pos + 46 <= (size_t)cd_off + cd_sz) {
            if (rd32(buf + pos) != 0x02014b50u) break;
            uint16_t comp = rd16(buf + pos + 10);
            uint32_t csz  = rd32(buf + pos + 20);
            uint16_t nlen = rd16(buf + pos + 28);
            uint16_t elen = rd16(buf + pos + 30);
            uint16_t clen = rd16(buf + pos + 32);
            uint32_t lh_off = rd32(buf + pos + 42);
            const uint8_t *nm = buf + pos + 46;
            if (comp == 0 && is_classes_dex(nm, nlen) && (size_t)lh_off + 30 <= sz) {
                uint16_t lnlen = rd16(buf + lh_off + 26);
                uint16_t lelen = rd16(buf + lh_off + 28);
                size_t doff = (size_t)lh_off + 30 + lnlen + lelen;
                if (doff + csz <= sz) {
                    uint8_t *dex = buf + doff;
                    int e = apply_dex_dynamic(dex, csz, &sat_anchors);
                    if (e > 0) {
                        sha1_calc(dex + 32, csz - 32, dex + 12);
                        wr32(dex + 8, adler32_calc(dex + 12, csz - 12));
                        uint32_t ncrc = crc32_calc(dex, csz);
                        wr32(buf + lh_off + 14, ncrc);
                        wr32(buf + pos + 16, ncrc);
                        dyn_edits += e;
                        dyn_dex++;
                    }
                }
            }
            pos += 46 + nlen + elen + clen;
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
        int nxt = max_dex_num + 1;
        size_t nlen = 7;
        memcpy(dname, "classes", 7);
        if (nxt >= 10) dname[nlen++] = (char)('0' + (nxt / 10));
        dname[nlen++] = (char)('0' + (nxt % 10));
        memcpy(dname + nlen, ".dex", 4);
        nlen += 4;
        dname[nlen] = '\0';

        uint32_t dex_sz = (uint32_t)sizeof(ZOOM_SMOOTHER_DEX);
        uint32_t dex_crc = crc32_calc(ZOOM_SMOOTHER_DEX, dex_sz);
        uint32_t pad = (4u - (((uint32_t)cd_off + 30u + (uint32_t)nlen) & 3u)) & 3u;
        uint32_t new_lfh_off = cd_off + pad;
        uint32_t new_dex_off = new_lfh_off + 30u + (uint32_t)nlen;
        uint32_t new_cd_off  = new_dex_off + dex_sz;
        uint32_t new_cd_sz   = cd_sz + 46u + (uint32_t)nlen;
        size_t   eocd_sz     = sz - eocd;
        size_t   new_sz      = (size_t)new_cd_off + (size_t)new_cd_sz + eocd_sz;

        uint8_t *out = alloc_buf(new_sz + 4096);
        if (!out) { free_buf(buf, sz + 4096); xputs(2, "ERR: OOM injecting smoother DEX\n"); return 1; }
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
        memcpy(lh + 30, dname, nlen);
        memcpy(out + new_dex_off, ZOOM_SMOOTHER_DEX, dex_sz);

        /* Central Directory + new entry */
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
        memcpy(ce + 46, dname, nlen);

        /* Updated EOCD */
        uint8_t *ne = out + new_cd_off + new_cd_sz;
        memcpy(ne, buf + eocd, eocd_sz);
        wr16(ne + 8,  (uint16_t)(rd16(ne + 8) + 1u));
        wr16(ne + 10, (uint16_t)(rd16(ne + 10) + 1u));
        wr32(ne + 12, new_cd_sz);
        wr32(ne + 16, new_cd_off);

        int rc = write_file_all(dst, out, new_sz);
        free_buf(out, new_sz + 4096);
        free_buf(buf, sz + 4096);
        if (rc != 0) { xputs(2, "ERR: failed to write patched APK\n"); return 1; }
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

static int main_c(int argc, char **argv) {
    if (argc < 4) {
        xputs(2, "Usage: garnet_patcher <patch-camx|patch-hal|patch-apk> ...\n");
        return 2;
    }
    if (xstrcmp(argv[1], "patch-camx") == 0 && argc == 4) return cmd_patch_camx(argv[2], argv[3]);
    if (xstrcmp(argv[1], "patch-hal") == 0 && argc == 5)  return cmd_patch_hal(argv[2], argv[3], argv[4]);
    if (xstrcmp(argv[1], "patch-apk") == 0 && argc == 4)  return cmd_patch_apk(argv[2], argv[3]);
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
