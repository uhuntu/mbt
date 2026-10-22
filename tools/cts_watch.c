/* Log every change of CTS on a tty for a given time. Opens the port without
 * changing its settings, so it can run next to another user of the port.
 * Usage: cts_watch <tty> <seconds> */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	struct timespec t0, t;
	int fd, m, last = -1;
	double end;

	if (argc < 3) { fprintf(stderr, "usage: %s tty seconds\n", argv[0]); return 1; }
	fd = open(argv[1], O_RDONLY | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) { perror("open"); return 1; }
	setvbuf(stdout, NULL, _IONBF, 0);
	clock_gettime(CLOCK_MONOTONIC, &t0);
	end = atof(argv[2]);
	for (;;) {
		clock_gettime(CLOCK_MONOTONIC, &t);
		double s = (t.tv_sec - t0.tv_sec) + (t.tv_nsec - t0.tv_nsec) / 1e9;
		if (s > end) break;
		if (ioctl(fd, TIOCMGET, &m) == 0 && !!(m & TIOCM_CTS) != last) {
			last = !!(m & TIOCM_CTS);
			printf("%8.3f CTS=%d RTS=%d\n", s, last, !!(m & TIOCM_RTS));
		}
		usleep(20000);
	}
	return 0;
}
