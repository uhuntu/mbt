/* Experiment helper, runs next to a downloader on the same tty.
 * 1. At pulse_s, deassert RTS for 0.5 s (to check that RTS control works:
 *    the controller should pause its replies, so the download pauses).
 * 2. When CTS then stays low for more than 100 ms (Launch RAM), deassert
 *    RTS for hold_s seconds, then assert it again.
 * Logs CTS/RTS changes with the time since start.
 * Usage: rts_ctl <tty> <run_s> <pulse_s> <hold_s> */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

static struct timespec t0;
static double now(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (t.tv_sec - t0.tv_sec) + (t.tv_nsec - t0.tv_nsec) / 1e9;
}

int main(int argc, char **argv)
{
	int fd, m, rts = TIOCM_RTS, last = -1, pulsed = 0, held = 0;
	double run, pulse, hold, low_since = -1;

	if (argc < 5) { fprintf(stderr, "usage: %s tty run_s pulse_s hold_s\n", argv[0]); return 1; }
	run = atof(argv[2]); pulse = atof(argv[3]); hold = atof(argv[4]);
	setvbuf(stdout, NULL, _IONBF, 0);
	clock_gettime(CLOCK_MONOTONIC, &t0);
	fd = open(argv[1], O_RDONLY | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) { perror("open"); return 1; }
	while (now() < run) {
		if (ioctl(fd, TIOCMGET, &m) == 0) {
			int cts = !!(m & TIOCM_CTS);
			if (cts != last) {
				printf("%8.3f CTS=%d RTS=%d\n", now(), cts, !!(m & TIOCM_RTS));
				last = cts;
				low_since = cts ? -1 : now();
			}
			if (!pulsed && now() >= pulse) {
				ioctl(fd, TIOCMBIC, &rts);
				printf("%8.3f RTS off (pulse)\n", now());
				usleep(500000);
				ioctl(fd, TIOCMBIS, &rts);
				printf("%8.3f RTS on\n", now());
				pulsed = 1;
				low_since = -1;
			}
			if (pulsed && !held && !cts && low_since >= 0 && now() - low_since > 0.1) {
				ioctl(fd, TIOCMBIC, &rts);
				ioctl(fd, TIOCMGET, &m);
				printf("%8.3f CTS low > 100 ms: RTS off for %.1f s (RTS bit now %d)\n", now(), hold, !!(m & TIOCM_RTS));
				usleep((useconds_t)(hold * 1e6));
				ioctl(fd, TIOCMBIS, &rts);
				printf("%8.3f RTS on\n", now());
				held = 1;
				last = -1;
			}
		}
		usleep(5000);
	}
	return 0;
}
