/* signals.h */
#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

void setup_signals(void);
void ignore_sigint(void);
int  sigint_received(void);

#endif
