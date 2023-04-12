#include <stdlib.h>

#include "ticker.h"
#include "store.h"
#include "cli.h"
#include "debug.h"

#include <unistd.h>

typedef struct watcher {
	// int index;
	int wType;
	int pid;
	int fd1;
	int fd2;
	char** args;
} WATCHER;

WATCHER *cli_watcher_start(WATCHER_TYPE *type, char *args[]) {
	WATCHER *wp = (WATCHER *)malloc(sizeof(WATCHER));
	*wp = (WATCHER){.wType = CLI_WATCHER_TYPE, .pid = -1, .fd1 = 0, .fd2 = 1, .args = args};
	return wp;
}

int cli_watcher_stop(WATCHER *wp) {
    // TO BE IMPLEMENTED
	//Free WATCHER
    abort();
}

int cli_watcher_send(WATCHER *wp, void *arg) {
    // TO BE IMPLEMENTED
    abort();
}

int cli_watcher_recv(WATCHER *wp, char *txt) {
    // TO BE IMPLEMENTED
    abort();
}

int cli_watcher_trace(WATCHER *wp, int enable) {
    // TO BE IMPLEMENTED
    abort();
}
