#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "debug.h"

#include "ticker.h"

void sigIntHandler(int sig) {
	debug("sigInt terminated\n");
	exit(0);
}

void sigChildHandler(int sig) {
	debug("sigChild terminated\n");
	exit(0);
}

//becomes 1 if userinput is avaliable
volatile sig_atomic_t userInput = 0;

void sigIOHandler(int sig, siginfo_t *siginfo, void *context) {
	//execute first
	//gets stdin file descriptor
    int fd = siginfo->si_fd;

	if(fd == STDIN_FILENO) {
		//user input avaliable
		userInput = 1;
	}
}

typedef struct watcher {
	// int index;
	int wType;
	int pid;
	int fd1;
	int fd2;
	char **args;
} WATCHER;

//quit flag
int quit;
//keeps track of the max number of watchers I need to check
int maxWatcherIndex;
//watcher table pointer
WATCHER **watcher_table;

void handleCommand(char **command) {
	char *firstArg = command[0];
	if(!(strcmp(firstArg, "quit"))) {
		quit = 1;
	} else if(!(strcmp(firstArg, "watchers"))) {
		for(int i = 0; i <= maxWatcherIndex; i++) {
			if(watcher_table[i]) {
				int watcherInfoSize = snprintf(NULL, 0, "%d\t%s(%d,%d,%d)", i, watcher_types[watcher_table[i]->wType].name, watcher_table[i]->pid, watcher_table[i]->fd1, watcher_table[i]->fd2);
				// debug("watcherInfoSize: %d", watcherInfoSize);
				char watcherInfo[watcherInfoSize + 1];
				sprintf(watcherInfo, "%d\t%s(%d,%d,%d)", i, watcher_types[watcher_table[i]->wType].name, watcher_table[i]->pid, watcher_table[i]->fd1, watcher_table[i]->fd2);
				watcherInfo[watcherInfoSize] = '\0';
				// debug("watcherInfo: %s", watcherInfo);
				write(STDOUT_FILENO, watcherInfo, watcherInfoSize);
				while(watcher_types[watcher_table[i]->wType].argv) {
					//for bitstamp
					//write the arguments
				}
				while(watcher_table[i]->args) {
					//if theres additional args, write them
				}
				write(STDOUT_FILENO, "\n", 1);
			}
		}
	} else {
		write(STDOUT_FILENO, "???\n", 4);
	}
}

// void readFromInput(sigset_t *mask) {
void readFromInput() {
	//reads input
	static char *buf = NULL;
	static size_t buf_size = 0;
	static FILE *memstream = NULL;
	//used for sigsuspend

	if(!(memstream)) {
		if(!(memstream = open_memstream(&buf, &buf_size))) {
			perror("open_memstream failed");
			exit(1);
		}
	}

	int n;
	char input[100];
	while((n = read(STDIN_FILENO, input, sizeof(input))) > 0) {
		// debug("n: %d", n);
		// debug("input: %s", input);
		if ((fwrite(input, sizeof(char), n, memstream)) < 0) {
			perror("fwrite failed");
			exit(1);
		}
	}
	// debug("n after read: %d", n);

	// FILE *stream;
	// if((stream = fmemopen(buf, buf_size, "r")) == NULL) {
	// 	perror("fmemopen failed");
	// 	exit(1);
	// }
	if(n < 0) { 
		if(errno == EWOULDBLOCK) {
			userInput = 0;
			// sigsuspend(&mask);
		} else {
			// debug("%d", errno);
			perror("read failed");
			exit(1);
		}
	}

	char *command = NULL;
	size_t command_size = 0;
	// debug("word: %s", fgets(command, sizeof(command), memstream));
	// debug("command: %s", command);
	//dynamically allocates memory for reading command lines
	while(getline(&command, &command_size, memstream) != -1) {
		//set newline to null pointer
		command[strlen(command) - 1] = '\0';
		int spaceCount = 0;
		for(int i = 0; i < strlen(command); i++) {
			if(command[i] == ' ') {
				spaceCount++;
			}
		}
		char *commandArgs[spaceCount + 2];
		char *token = strtok(command, " ");
		int argvIndex = 0;
		while(token != NULL) {
			commandArgs[argvIndex++] = token;
			token = strtok(NULL, " ");
		}
		commandArgs[argvIndex] = NULL;
		// debug("command: %d", command[strlen(command)]);

		handleCommand(commandArgs);
		write(STDOUT_FILENO, "ticker> ", 8);

		free(command);
		command = NULL;
		command_size = 0;
	}

	if(n == 0) {
		quit = 1;
	}

	if(quit && memstream) {
		// debug("quit: %d", quit);
		// debug("n: %d", n);
		// debug("memstream: %p", memstream);
		if(fclose(memstream) == EOF) {
			perror("fclose memstream failed");
			exit(1);
		}
		free(buf);
		buf = NULL;
		memstream = NULL;
		buf_size = 0;
	}
}

int ticker(void) {
	struct sigaction action1, action2, action3;
//initialize sigactions
	memset(&action1, 0, sizeof(action1));
	action1.sa_handler = sigIntHandler;
	sigemptyset(&action1.sa_mask);
	if(sigaction(SIGINT, &action1, NULL) < 0) {
		perror("sigaction SIGINT");
		exit(1);
	}

	memset(&action2, 0, sizeof(action2));
	action2.sa_handler = sigChildHandler;
	sigemptyset(&action2.sa_mask);
	if(sigaction(SIGCHLD, &action2, NULL) < 0) {
		perror("sigaction SIGCHLD");
		exit(1);
	}

	memset(&action3, 0, sizeof(action3));
	action3.sa_sigaction = sigIOHandler;
	action3.sa_flags = SA_SIGINFO;
	sigemptyset(&action3.sa_mask);
	if(sigaction(SIGIO, &action3, NULL) < 0) {
		perror("sigaction SIGIO");
		exit(1);
	}
//test handlers
	// kill(getpid(), SIGCHLD);	
	// kill(getpid(), SIGIO);



// To enable asynchronous I/O notification on a file descriptor

// (1) Asynchronous I/O notification needs to be set on the file descriptor,
// using the fcntl() system call with command F_SETFL and argument
// O_ASYNC
	if(fcntl(STDIN_FILENO, F_SETFL, O_ASYNC | O_NONBLOCK) < 0) {
		perror("fcntl F_SETFL");
		exit(1);
	}

// The fcntl() system call has to be used on that file descriptor with
// command F_SETOWN to identity the process to which I/O notifications for
// that file descriptor should be sent;
	if(fcntl(STDIN_FILENO, F_SETOWN, getpid()) < 0) {
		perror("fcntl F_SETOWN");
		exit(1);
	}

// The fcntl() system call has to be used
// on that file descriptor with command F_SETSIG to designate
// SIGIO as the signal to be sent as the asynchronous I/O notification
	if(fcntl(STDIN_FILENO, F_SETSIG, SIGIO) < 0) {
		perror("fcntl F_SETSIG");
		exit(1);
	}

	//start CLI and table
	WATCHER *CLI = watcher_types[CLI_WATCHER_TYPE].start(&watcher_types[CLI_WATCHER_TYPE], watcher_types[CLI_WATCHER_TYPE].argv);
	watcher_table = malloc(sizeof(WATCHER *) * 100);
	watcher_table[0] = CLI;
	maxWatcherIndex = 0;
	watcher_table[1] = NULL;

	sigset_t mask;
	if(sigemptyset(&mask) == -1) {
		perror("sigemptyset failed");
		exit(1);
	}

	write(STDOUT_FILENO, "ticker> ", 8);
	// printf("ticker> ");
	readFromInput();

	while(!quit) {
		if(!userInput) {
			sigsuspend(&mask);
		}
		if(userInput) {
			readFromInput(&mask);
		}

		//execute second
		// write(STDOUT_FILENO, "ticker> ", 8);
		// printf("ticker> ");

		//reads input
		// char input[100];
		// ssize_t n;
		// while((n = read(fd, input, sizeof(input))) > 0) {
		// 	debug("input size: %zu", sizeof(input));
		// 	debug( "read %zu bytes", n);
		// 	debug( "%s", input);
		// 	fwrite(input, sizeof(char), n, memstream);
		// 	nRead += n;
		// 	for(int i = 0; i < n; i++) {
		// 		if(input[i] == '\n') {
		// 			// nRead = n - i - 1;
		// 		}
		// 	}
		// }
	}

	//quit happens
	//call CLI.stop?
	free(CLI);
	free(watcher_table);

	return 0;
}
