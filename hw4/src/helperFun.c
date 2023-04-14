#include "helperFun.h"
// #include "watchers.h"
#include <stdio.h>
#include "debug.h"
#include <stdlib.h>

//handles commands
//0 if other command, 1 if quit
int handleCommand(char **command, WATCHER *CLI) {
	// static WATCHER **wt = NULL; 
	static int invalidCommand = 0;
	static int watcherCreated = 0;

	// debug("handleCommand: %s", command[0]);
	// debug("handleCommand: %s", command[1]);
	// debug("handleCommand: %s", command[2]);

	// if(!wt) wt = CLI->watcher_table;
	//check first arg
	char *firstArg = command[0];
	char *secondArg = command[1];
	if(!(strcmp(firstArg, "quit"))) {
		//set quit flag
		//dont exit right away to free things
		// quit = 1;
		return 1;
	} else if(!(strcmp(firstArg, "watchers"))) {
		//print out all the watchers
		// WATCHER **wt = CLI->watcher_table;
		// debug("tSize: %d", CLI->watcher_table_size);
		for(int i = 0; i < CLI->watcher_table_size; i++) {
			// debug("%d", i);
			if(CLI->watcher_table[i] && !(CLI->watcher_table[i]->terminating)) {
				//get expected size of string
				int watcherInfoSize;
				if((watcherInfoSize = snprintf(NULL, 0, "%d\t%s(%d,%d,%d)", 
				i, 
				CLI->watcher_table[i]->wType->name, 
				CLI->watcher_table[i]->pid, 
				CLI->watcher_table[i]->fd1, 
				CLI->watcher_table[i]->fd2)) < 0) {
					perror("snprintf failed");
					return -1;
				}
				char watcherInfo[watcherInfoSize + 1];
				// debug("watcherInfoSize: %d", watcherInfoSize);
				// debug("sizeof(watcherInfo): %zu", sizeof(watcherInfo));
				//get the string
				if((sprintf(watcherInfo, "%d\t%s(%d,%d,%d)", i, 
				CLI->watcher_table[i]->wType->name, 
				CLI->watcher_table[i]->pid, 
				CLI->watcher_table[i]->fd1, 
				CLI->watcher_table[i]->fd2)) < 0) {
					perror("sprintf failed");
					return -1;
				}
				watcherInfo[watcherInfoSize] = '\0';
				// debug("watcherInfo: %s", watcherInfo);
				// debug("sizeof(watcherInfo): %zu", sizeof(watcherInfo));
				//write to stdout
				// write(STDOUT_FILENO, watcherInfo, watcherInfoSize);
				if(CLI->wType->send(CLI, watcherInfo) < 0) {
					perror("send failed");
					return -1;
				}
				//write the arguments of the watcher type
				if(CLI->watcher_table[i]->wType->argv) {
					//for bitstamp
					//write the arguments
					int argvIndex = 0;
					while(CLI->watcher_table[i]->wType->argv[argvIndex]){
						if(CLI->wType->send(CLI, " ") < 0) {
							perror("send failed");
							return -1;
						}
						if(CLI->wType->send(CLI, CLI->watcher_table[i]->wType->argv[argvIndex]) < 0) {
							perror("send failed");
							return -1;
						}
						argvIndex++;
					}
				}
				//write the additional arguments
				if(CLI->watcher_table[i]->args) {
					// debug("wt[i]->args: %s", wt[i]->wType->name);
					//if theres additional args, write them
					// debug("i: %d", i);
					int argsIndex = 0;
					while(CLI->watcher_table[i]->args[argsIndex]) {
						// debug("argsIndex: %d", argsIndex);
						// debug("wt[i]->args[argsIndex]: %s", wt[i]->args[argsIndex]);
						if(CLI->wType->send(CLI, " [") < 0) {
							perror("send failed");
							return -1;
						}
						if(CLI->wType->send(CLI, CLI->watcher_table[i]->args[argsIndex]) < 0) {
							perror("send failed");
							return -1;
						}
						if(CLI->wType->send(CLI, "]") < 0) {
							perror("send failed");
							return -1;
						}
						argsIndex++;
					}
				}
				//write a newline
				// write(STDOUT_FILENO, "\n", 1);
				if(CLI->wType->send(CLI, "\n") < 0) {
					perror("send failed");
					return -1;
				}
			}
		}
	} else if(!(strcmp(firstArg, "start"))) {
		//start a watcher
		// char *secondArg;
		if(!(secondArg)) {
			invalidCommand = 1;
		} else {
			//if there is a name
			// debug("secondArg: %s", secondArg);
			int i = 0;
			char *wTypeName;
			WATCHER *w;
			//search type by name and must not be CLI
			// debug("strcmp(secondArg, \"CLI\"): %d", strcmp(secondArg, "CLI"));
			if(!strcmp(secondArg, "CLI")) {
				invalidCommand = 1;
			} else {
				while((wTypeName = watcher_types[i].name)) {
					if(!strcmp(secondArg, wTypeName)) {
						//start the watcher
						if((w = watcher_types[i].start(&watcher_types[i], command + 2)) < 0) {
							perror("start failed");
							return -1;
						}
						watcherCreated = 1;
						break;
					}
					i++;
				}
				if(!watcherCreated) {
					invalidCommand = 1;
				} else {
					watcherCreated = 0;
					//put watcher in table
					int j = 0;
					while(CLI->watcher_table[j]) {
						j++;
						if(j == CLI->watcher_table_size) {
							//realloc the table
							if(!(CLI->watcher_table = realloc(CLI->watcher_table, 
							sizeof(WATCHER *) * (CLI->watcher_table_size + 1)))) {
								perror("realloc failed");
								return -1;
							}
							CLI->watcher_table_size++;
							CLI->watcher_table[j] = NULL;
						}
					}
					CLI->watcher_table[j] = w;
					// debug("w->args[0]: %s", w->args[0]);
					// debug("j: %d", j);
					// if(j < CLI->watcher_table_size - 1) {
					// 	CLI->watcher_table[j + 1] = NULL;
					// }
				}
			}
		}
	} else if(!(strcmp(firstArg, "stop"))) {
		if(!secondArg) {
			invalidCommand = 1;
		} else {
			//stop the watcher
			int index = atoi(secondArg);
			if(index < 1 || index >= CLI->watcher_table_size || !(CLI->watcher_table[index])) {
				invalidCommand = 1;
			} else {
				if(CLI->watcher_table[index]->wType->stop(CLI->watcher_table[index]) < 0) {
					perror("stop failed");
					return -1;
				}
				// debug("hi");
				// debug("pid: %d", CLI->watcher_table[2]->pid);
			}
		}
	} else {
		invalidCommand = 1;
	}

	if(invalidCommand) {
		//invalid command
		invalidCommand = 0;
		if(CLI->wType->send(CLI, "???\n") < 0) {
			perror("send failed");
			return -1;
		}
	}
	return 0;
}


//handles commands
// void handleCommand(char **command) {
// 	//check first arg
// 	char *firstArg = command[0];
// 	if(!(strcmp(firstArg, "quit"))) {
// 		//set quit flag
// 		//dont exit right away to free things
// 		quit = 1;
// 	} else if(!(strcmp(firstArg, "watchers"))) {
// 		//print out all the watchers
// 		for(int i = 0; i <= maxWatcherIndex; i++) {
// 			if(watcher_table[i]) {
// 				//get expected size of string
// 				int watcherInfoSize = snprintf(NULL, 0, "%d\t%s(%d,%d,%d)", i, watcher_table[i]->wType->name, watcher_table[i]->pid, watcher_table[i]->fd1, watcher_table[i]->fd2);
// 				// debug("watcherInfoSize: %d", watcherInfoSize);
// 				char watcherInfo[watcherInfoSize + 1];
// 				//get the string
// 				sprintf(watcherInfo, "%d\t%s(%d,%d,%d)", i, watcher_table[i]->wType->name, watcher_table[i]->pid, watcher_table[i]->fd1, watcher_table[i]->fd2);
// 				watcherInfo[watcherInfoSize] = '\0';
// 				// debug("watcherInfo: %s", watcherInfo);
// 				//write to stdout
// 				write(STDOUT_FILENO, watcherInfo, watcherInfoSize);
// 				//write the arguments of the watcher type
// 				while(watcher_table[i]->wType->argv) {
// 					//for bitstamp
// 					//write the arguments
// 				}
// 				//write the additional arguments
// 				while(watcher_table[i]->args) {
// 					//if theres additional args, write them
// 				}
// 				//write a newline
// 				write(STDOUT_FILENO, "\n", 1);
// 			}
// 		}
// 	} else {
// 		//invalid command
// 		write(STDOUT_FILENO, "???\n", 4);
// 	}
// }