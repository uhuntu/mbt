/*
 * Power-cycle BT via rfkill and log every byte the controller sends at 115200,
 * then send HCI Reset / Read Local Name / Read Local Version and log replies.
 * Usage: uart_watch <tty> <rfkill state> <off_ms> <listen_ms> [baud]
 *   off_ms < 0: do not power-cycle, just listen and send commands.
 * Environment:
 *   FLOW=1    turn RTS/CTS flow control on (default off)
 *   RTSOFF=1  keep RTS deasserted while BT_REG_ON is cycled
 *   ORDER=... commands to send afterwards, in order: r = HCI Reset,
 *             n = Read Local Name, v = Read Local Version,
 *             m = Download Minidriver (default "rnv")
 */
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>

static int fd;
static struct timespec t0;

static double now_ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (t.tv_sec - t0.tv_sec) * 1000.0 + (t.tv_nsec - t0.tv_nsec) / 1e6;
}

static void rfkill(const char *p, char v) {
    int f = open(p, O_WRONLY);
    if (f < 0 || write(f, &v, 1) != 1) perror("rfkill");
    if (f >= 0) close(f);
    printf("%9.1f rfkill=%c\n", now_ms(), v);
}

/* Print everything received for ms milliseconds, grouped by bursts. */
static void listen_for(int ms) {
    double end = now_ms() + ms;
    unsigned char b[512];
    while (now_ms() < end) {
        struct pollfd p = { fd, POLLIN, 0 };
        if (poll(&p, 1, (int)(end - now_ms()) + 1) <= 0) break;
        usleep(3000); /* let a burst complete */
        int n = read(fd, b, sizeof b);
        if (n <= 0) break;
        printf("%9.1f rx %3d:", now_ms(), n);
        for (int i = 0; i < n && i < 48; i++) printf(" %02x", b[i]);
        if (n > 48) printf(" ...");
        int s = 0; /* printable tail, for names */
        for (int i = 0; i < n; i++) if (b[i] >= 0x20 && b[i] < 0x7f) s++;
        if (s > 6) { printf("  \""); for (int i = 0; i < n; i++) if (b[i] >= 0x20 && b[i] < 0x7f) putchar(b[i]); printf("\""); }
        printf("\n");
    }
}

static void lines(const char *tag) {
    int m = 0;
    ioctl(fd, TIOCMGET, &m);
    printf("%9.1f %s: CTS=%d RTS=%d\n", now_ms(), tag, !!(m & TIOCM_CTS), !!(m & TIOCM_RTS));
}

static void send(const char *tag, const unsigned char *c, int n) {
    lines(tag);
    printf("%9.1f tx %s\n", now_ms(), tag);
    if (write(fd, c, n) != n) perror("write");
    listen_for(500);
}

int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "usage: %s tty rfkill off_ms listen_ms [baud]\n", argv[0]); return 1; }
    int off = atoi(argv[3]), lis = atoi(argv[4]);
    speed_t baud = (argc > 5 && atoi(argv[5]) == 3000000) ? B3000000 : B115200;
    setvbuf(stdout, NULL, _IONBF, 0);
    clock_gettime(CLOCK_MONOTONIC, &t0);

    fd = open(argv[1], O_RDWR | O_NOCTTY);
    if (fd < 0) { perror("open"); return 1; }
    struct termios tio;
    tcgetattr(fd, &tio);
    cfmakeraw(&tio);
    if (getenv("FLOW")) tio.c_cflag |= CRTSCTS; else tio.c_cflag &= ~CRTSCTS;
    tio.c_cflag |= CLOCAL | CREAD;
    cfsetospeed(&tio, baud);
    cfsetispeed(&tio, baud);
    tcsetattr(fd, TCSANOW, &tio);
    tcflush(fd, TCIOFLUSH);

    lines("start");
    int rts = TIOCM_RTS;
    if (getenv("RTSOFF")) { ioctl(fd, TIOCMBIC, &rts); lines("RTS deasserted"); }
    if (off >= 0) {
        rfkill(argv[2], '0');
        usleep(50000); lines("BT_REG_ON low");
        usleep(off * 1000);
        rfkill(argv[2], '1');
        for (int k = 0; k < 6; k++) { usleep(50000); lines("after power on"); }
    }
    if (getenv("RTSOFF")) { ioctl(fd, TIOCMBIS, &rts); lines("RTS asserted again"); }
    listen_for(lis);
    lines("before commands");

    static const unsigned char reset[] = { 0x01, 0x03, 0x0c, 0x00 };
    static const unsigned char name[] = { 0x01, 0x14, 0x0c, 0x00 };
    static const unsigned char ver[] = { 0x01, 0x01, 0x10, 0x00 };
    static const unsigned char mini[] = { 0x01, 0x2e, 0xfc, 0x00 };
    const char *order = getenv("ORDER");  /* e.g. "nrnv": n=name r=reset v=version */
    if (!order) order = "rnv";
    for (const char *o = order; *o; o++) {
        if (*o == 'r') send("HCI_Reset", reset, sizeof reset);
        if (*o == 'n') send("Read_Local_Name", name, sizeof name);
        if (*o == 'v') send("Read_Local_Version", ver, sizeof ver);
        if (*o == 'm') send("Download_Minidriver", mini, sizeof mini);
    }
    close(fd);
    return 0;
}
