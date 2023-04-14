#include "ticker.h"
#include <string.h>

#ifndef WATCHER_H
#define WATCHER_H

typedef struct watcher{
	WATCHER_TYPE *wType;//0 for bitstamp, 1 for bitfinex
	int pid;//process id
	int fd1;//file descriptor 1
	int fd2;//file descriptor 2
	char **args;//additional arguments, allocated need free
	// int maxWatcherIndex;
	WATCHER **watcher_table;
	int watcher_table_size;
	int terminating;
} WATCHER;

#endif /* WATCHER_H */