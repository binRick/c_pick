#include "config.h"

#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define tty_putp(capability, fatal)    do {                 \
    if (tputs((capability), 1, tty_putc) == ERR && (fatal)) \
    errx(1, #capability ": unknown terminfo capability");   \
} while (0)
