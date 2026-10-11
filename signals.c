#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "signals.h"

static volatile sig_atomic_t got_sigint = 0;

static void sigint_handler(int sig)
{
    (void)sig;
    got_sigint = 1;
}

int sigint_received(void)
{
    return got_sigint;
}

void setup_signals(void)
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    sigemptyset(&act.sa_mask);

    /* SIGPIPE: ignorar.*/
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1) {
        perror("sigaction SIGPIPE");
        exit(1);
    }

    act.sa_handler = sigint_handler;
    act.sa_flags = 0;
    if (sigaction(SIGINT, &act, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(1);
    }
}

void ignore_sigint(void)
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    sigemptyset(&act.sa_mask);
    act.sa_handler = SIG_IGN;
    sigaction(SIGINT, &act, NULL);
}