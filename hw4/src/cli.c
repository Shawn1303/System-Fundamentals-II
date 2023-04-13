#include <stdlib.h>

#include "ticker.h"
#include "store.h"
#include "cli.h"
#include "debug.h"

#include <unistd.h>
#include "watchers.h"
#include <string.h>
// #include "ticker.c"
#include "helperFun.h"

WATCHER *cli_watcher_start(WATCHER_TYPE *type, char *args[]) {
	WATCHER *wp;
	if(!(wp = (WATCHER *)malloc(sizeof(WATCHER)))) {
		perror("malloc failed");
		return NULL;
	}
	*wp = (WATCHER){.wType = type, .pid = -1, .fd1 = 0, .fd2 = 1, .args = args};
	return wp;
}

//free WATCHER and table
int cli_watcher_stop(WATCHER *wp) {
	// if(!(strcmp(wp->wType->name, "CLI"))) {
		// debug("watcher_stop: %s", wp->wType.name);
	free(wp);
	free(wp->watcher_table);
	return 0;
	// } 
	// return -1;
}

//writes to stdout file descriptor
int cli_watcher_send(WATCHER *wp, void *arg) {
    // if(!(strcmp(wp->wType->name, "CLI"))) {
		// debug("size of arg: %zu", sizeof((char *)arg));
		// debug("arg: %s", (char *)arg);
	return write(STDOUT_FILENO, arg, strlen(arg));
	// 	return 0;
	// }
    // return -1;
}

//handle user inputs
int cli_watcher_recv(WATCHER *wp, char *txt) {
    int spaceCount = 0;
	for(int i = 0; i < strlen(txt); i++) {
		if(txt[i] == ' ') {
			spaceCount++;
		}
	}
	char *commandArgs[spaceCount + 2];
	//use tokens to parse command line
	char *token = strtok(txt, " ");
	int argvIndex = 0;
	while(token != NULL) {
		commandArgs[argvIndex++] = token;
		token = strtok(NULL, " ");
	}
	//last argv is null
	commandArgs[argvIndex] = NULL;
	// debug("command: %d", command[strlen(command)]);

	// //handle command
	return handleCommand(commandArgs, wp);
}

int cli_watcher_trace(WATCHER *wp, int enable) {
    // TO BE IMPLEMENTED
    abort();
}
