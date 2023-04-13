#include "helperFun.h"
// #include "watchers.h"
#include <stdio.h>
#include "debug.h"

//handles commands
//0 if other command, 1 if quit
int handleCommand(char **command, WATCHER *wp) {
	//check first arg
	char *firstArg = command[0];
	if(!(strcmp(firstArg, "quit"))) {
		//set quit flag
		//dont exit right away to free things
		// quit = 1;
		return 1;
	} else if(!(strcmp(firstArg, "watchers"))) {
		//print out all the watchers
		WATCHER **wt = wp->watcher_table;
		for(int i = 0; i <= wp->maxWatcherIndex; i++) {
			if(wt[i]) {
				//get expected size of string
				int watcherInfoSize = snprintf(NULL, 0, "%d\t%s(%d,%d,%d)", i, wt[i]->wType->name, wt[i]->pid, wt[i]->fd1, wt[i]->fd2);
				char watcherInfo[watcherInfoSize + 1];
				// debug("watcherInfoSize: %d", watcherInfoSize);
				// debug("sizeof(watcherInfo): %zu", sizeof(watcherInfo));
				//get the string
				sprintf(watcherInfo, "%d\t%s(%d,%d,%d)", i, wt[i]->wType->name, wt[i]->pid, wt[i]->fd1, wt[i]->fd2);
				watcherInfo[watcherInfoSize] = '\0';
				// debug("watcherInfo: %s", watcherInfo);
				// debug("sizeof(watcherInfo): %zu", sizeof(watcherInfo));
				//write to stdout
				// write(STDOUT_FILENO, watcherInfo, watcherInfoSize);
				wp->wType->send(wp, watcherInfo);
				//write the arguments of the watcher type
				while(wt[i]->wType->argv) {
					//for bitstamp
					//write the arguments
				}
				//write the additional arguments
				while(wt[i]->args) {
					//if theres additional args, write them
				}
				//write a newline
				// write(STDOUT_FILENO, "\n", 1);
				wp->wType->send(wp, "\n");
			}
		}
	} else {
		//invalid command
		// write(STDOUT_FILENO, "???\n", 4);
		wp->wType->send(wp, "???\n");
	}
	return 0;
}