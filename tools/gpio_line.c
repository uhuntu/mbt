/*
 * Show or drive one GPIO line through the GPIO character device (v2 uapi).
 * Usage:
 *   gpio_line <chip> <line> info
 *   gpio_line <chip> <line> set <0|1>              drive the line, then release it
 *   gpio_line <chip> <line> pulse <0|1> <ms>       drive to the value for ms, then
 *                                                  back to the other value, release
 * The line is requested as an output that starts at the value it ends with
 * (for pulse: the opposite of the pulse value), so requesting it does not
 * glitch a line that is already at that level.
 */
#include <fcntl.h>
#include <linux/gpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

static double now_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

static int info(int fd, unsigned line)
{
    struct gpio_v2_line_info li;

    memset(&li, 0, sizeof(li));
    li.offset = line;
    if (ioctl(fd, GPIO_V2_GET_LINEINFO_IOCTL, &li) < 0) {
        perror("GPIO_V2_GET_LINEINFO");
        return -1;
    }
    printf("line %u: name \"%s\" consumer \"%s\" %s%s%s\n", line, li.name,
           li.consumer, (li.flags & GPIO_V2_LINE_FLAG_USED) ? "used " : "free ",
           (li.flags & GPIO_V2_LINE_FLAG_OUTPUT) ? "output" : "",
           (li.flags & GPIO_V2_LINE_FLAG_INPUT) ? "input" : "");
    return 0;
}

static int request_out(int fd, unsigned line, int value)
{
    struct gpio_v2_line_request req;

    memset(&req, 0, sizeof(req));
    req.offsets[0] = line;
    req.num_lines = 1;
    strcpy(req.consumer, "gpio_line");
    req.config.flags = GPIO_V2_LINE_FLAG_OUTPUT;
    req.config.num_attrs = 1;
    req.config.attrs[0].attr.id = GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES;
    req.config.attrs[0].attr.values = value ? 1 : 0;
    req.config.attrs[0].mask = 1;
    if (ioctl(fd, GPIO_V2_GET_LINE_IOCTL, &req) < 0) {
        perror("GPIO_V2_GET_LINE");
        return -1;
    }
    return req.fd;
}

static int set(int lfd, int value)
{
    struct gpio_v2_line_values v = { .bits = value ? 1 : 0, .mask = 1 };

    if (ioctl(lfd, GPIO_V2_LINE_SET_VALUES_IOCTL, &v) < 0) {
        perror("GPIO_V2_LINE_SET_VALUES");
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int fd, lfd, value;
    unsigned line;
    double t0;

    if (argc < 4) {
        fprintf(stderr, "usage: %s chip line info | set <0|1> | pulse <0|1> <ms>\n", argv[0]);
        return 1;
    }
    setvbuf(stdout, NULL, _IONBF, 0);
    line = atoi(argv[2]);
    if ((fd = open(argv[1], O_RDWR)) < 0) {
        perror(argv[1]);
        return 1;
    }
    if (info(fd, line) < 0)
        return 1;

    if (!strcmp(argv[3], "set") && argc > 4) {
        value = atoi(argv[4]);
        if ((lfd = request_out(fd, line, value)) < 0)
            return 1;
        printf("line %u set to %d\n", line, value);
        close(lfd);
    } else if (!strcmp(argv[3], "pulse") && argc > 5) {
        value = atoi(argv[4]) ? 1 : 0;
        if ((lfd = request_out(fd, line, !value)) < 0)
            return 1;
        t0 = now_ms();
        set(lfd, value);
        printf("%8.1f line %u -> %d\n", 0.0, line, value);
        usleep(atoi(argv[5]) * 1000);
        set(lfd, !value);
        printf("%8.1f line %u -> %d\n", now_ms() - t0, line, !value);
        close(lfd);
    } else if (strcmp(argv[3], "info")) {
        fprintf(stderr, "unknown command\n");
        return 1;
    }
    close(fd);
    return 0;
}
