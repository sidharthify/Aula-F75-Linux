/*
 * f75cfg - inspect and manipulate the AULA F75 on-board "LED profile".
 *
 * The Windows driver stores its lighting/effect configuration as a 128-byte
 * block that is read with opcode 0x44 and written with opcode 0x04 over the
 * 20-byte vendor HID protocol.  This tool exposes that block so the effect
 * fields can be isolated.
 *
 *   f75cfg read  [dev]        print the 128-byte profile as hex
 *   f75cfg diff  [dev]        read, wait for Enter, read again and print the
 *                             bytes that changed -- use it by changing the
 *                             on-board effect (keyboard Fn shortcut) between
 *                             the two reads
 *   f75cfg write <hex> [dev]  write a 128-byte profile given as a hex string
 *
 * Build:  gcc -O2 -o f75cfg f75cfg.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

#define PKT       20
#define PROFILE   128
#define CHUNK     14

static unsigned char crc(const unsigned char *p)
{
    unsigned char c = 0;
    for (int i = 0; i < 19; i++) c += p[i];
    return c;
}

static int open_dev(const char *path)
{
    int fd = open(path, O_RDWR | O_NONBLOCK);
    if (fd < 0) perror("open");
    return fd;
}

static void drain(int fd)
{
    unsigned char r[PKT];
    struct pollfd pf = { fd, POLLIN, 0 };
    while (poll(&pf, 1, 50) > 0) { if (read(fd, r, PKT) <= 0) break; }
}

static void wake(int fd)
{
    unsigned char p[PKT];
    memset(p, 0, PKT); p[0] = 0x13; p[1] = 0x05; p[2] = 1; p[19] = crc(p);
    write(fd, p, PKT);
    drain(fd);
}

/* Read the 128-byte LED profile (opcode 0x44). */
static int read_profile(int fd, unsigned char *out)
{
    unsigned char p[PKT], r[PKT];
    memset(out, 0, PROFILE);
    for (int attempt = 0; attempt < 8; attempt++)
    {
        drain(fd);
        wake(fd);
        memset(p, 0, PKT); p[0] = 0x13; p[1] = 0x44; p[2] = 1; p[19] = crc(p);
        write(fd, p, PKT);

        int have = 0, total = -1;
        struct pollfd pf = { fd, POLLIN, 0 };
        for (int i = 0; i < 1200; i++)
        {
            if (poll(&pf, 1, 1) > 0)
            {
                int n = read(fd, r, PKT);
                if (n >= 5 && r[0] == 0x13 && (r[1] & 0x7f) == 0x44)
                {
                    int idx = r[3] & 0x7f, len = r[4] & 0x0f;
                    total = r[2] & 0x7f;
                    if (idx * CHUNK + len <= PROFILE) { memcpy(out + idx * CHUNK, r + 5, len); have = 1; }
                    if (total > 0 && idx == total - 1) return 1;
                }
            }
        }
        if (have && total > 0) return 1;
        usleep(20000);
    }
    return 0;
}

/* Write a 128-byte LED profile (opcode 0x04). */
static int write_profile(int fd, const unsigned char *in)
{
    unsigned char p[PKT];
    int chunks = (PROFILE + CHUNK - 1) / CHUNK;
    for (int c = 0; c < chunks; c++)
    {
        int off = c * CHUNK, n = PROFILE - off;
        if (n > CHUNK) n = CHUNK;
        memset(p, 0, PKT);
        p[0] = 0x13; p[1] = 0x04; p[2] = (unsigned char)chunks; p[3] = (unsigned char)c;
        p[4] = (unsigned char)n;
        memcpy(p + 5, in + off, n);
        p[19] = crc(p);
        if (write(fd, p, PKT) < 0) { perror("write"); return 0; }
        usleep(5000);
    }
    return 1;
}

static void hexdump(const unsigned char *b, int n)
{
    for (int i = 0; i < n; i++)
    {
        printf("%02x ", b[i]);
        if (i % 14 == 13) printf("\n");
    }
    printf("\n");
}

static int hex2bytes(const char *s, unsigned char *out, int max)
{
    int n = 0;
    while (*s && n < max)
    {
        while (*s == ' ' || *s == '\n' || *s == '\t') s++;
        if (!*s) break;
        unsigned v;
        if (sscanf(s, "%2x", &v) != 1) break;
        out[n++] = (unsigned char)v;
        s += 2;
    }
    return n;
}

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: f75cfg read|diff|write ...\n"); return 1; }
    const char *cmd = argv[1];
    const char *dev = "/dev/hidraw4";
    unsigned char prof[PROFILE], prof2[PROFILE];

    if (!strcmp(cmd, "read"))
    {
        if (argc >= 3) dev = argv[2];
        int fd = open_dev(dev); if (fd < 0) return 1;
        if (!read_profile(fd, prof)) { fprintf(stderr, "no reply (keyboard asleep?)\n"); return 1; }
        hexdump(prof, PROFILE);
    }
    else if (!strcmp(cmd, "diff"))
    {
        if (argc >= 3) dev = argv[2];
        int fd = open_dev(dev); if (fd < 0) return 1;
        if (!read_profile(fd, prof)) { fprintf(stderr, "first read failed\n"); return 1; }
        printf("Read the profile. Now CHANGE THE ON-BOARD EFFECT on the keyboard\n");
        printf("(its Fn effect shortcut, or the Windows driver), then press Enter...\n");
        fflush(stdout);
        char line[64];
        if (!fgets(line, sizeof line, stdin)) return 1;
        if (!read_profile(fd, prof2)) { fprintf(stderr, "second read failed\n"); return 1; }
        printf("Changed bytes (offset: before -> after):\n");
        int changed = 0;
        for (int i = 0; i < PROFILE; i++)
            if (prof[i] != prof2[i]) { printf("  [%3d] %02x -> %02x\n", i, prof[i], prof2[i]); changed++; }
        if (!changed) printf("  (none — the board may be in direct mode, or the effect is not in this block)\n");
    }
    else if (!strcmp(cmd, "write"))
    {
        if (argc < 3) { fprintf(stderr, "usage: f75cfg write <hex> [dev]\n"); return 1; }
        if (argc >= 4) dev = argv[3];
        int n = hex2bytes(argv[2], prof, PROFILE);
        if (n != PROFILE) { fprintf(stderr, "need %d bytes, got %d\n", PROFILE, n); return 1; }
        int fd = open_dev(dev); if (fd < 0) return 1;
        if (!write_profile(fd, prof)) return 1;
        printf("wrote %d bytes\n", PROFILE);
    }
    else { fprintf(stderr, "unknown command\n"); return 1; }

    return 0;
}
