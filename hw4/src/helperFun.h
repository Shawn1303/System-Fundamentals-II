#include "watchers.h"

#ifndef HELPERFUN_H
#define HELPERFUN_H

int handleCommand(char **command, WATCHER *wp);
// extern int sigChildTermFlag;
int outputTracing(WATCHER *wp, char *traceMsg);

#endif // HELPERFUN_H